/*
 * pulse_decoder.c
 *
 * Uses the ESP-IDF MCPWM Capture peripheral to measure PWM pulse widths and
 * identify which of N registered signals was received.
 *
 * Multi-chip resolution strategy
 * ──────────────────────────────
 * We request resolution_hz = 1,000,000 (1 tick = 1 µs) in the timer config.
 * On chips with SOC_MCPWM_CAPTURE_CLK_FROM_GROUP (ESP32-S3, C3, H2 ...) this
 * takes effect and cap_value is delivered in µs directly.
 * On the original ESP32 the hardware has no prescaler and ignores resolution_hz,
 * always running at APB (80 MHz).
 * After timer creation we call mcpwm_capture_timer_get_resolution() to read
 * back the ACTUAL resolution the hardware is using.  All tick↔µs conversions
 * use this queried value, so the same binary is correct on every target.
 *
 * Design notes
 * ────────────
 * • Up to 6 capture channels across 2 MCPWM groups (3 channels per group).
 * • A single FreeRTOS queue + task is shared by all instances.
 * • The ISR does only a tick-domain range check and enqueues raw tick values.
 * • The task converts ticks → µs and resolves the pulse ID.
 * • The interface is accessed through a pointer to the embedded member so that
 *   CONTAINER_OF always reconstructs the correct object address.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/mcpwm_cap.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"

#include "pulse_decoder.h"

/* ── Constants ───────────────────────────────────────────────────────────── */
static const char *TAG = "pulse-decoder";

#define MAX_CHANNELS            6
#define MAX_CHANNELS_PER_UNIT   3
#define TOTAL_CAPTURE_GROUPS    2
#define QUEUE_LENGTH            100

/* ── CONTAINER_OF ────────────────────────────────────────────────────────── */
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* ── Internal types ──────────────────────────────────────────────────────── */

typedef struct {
    mcpwm_cap_timer_handle_t      cap_timer;
    mcpwm_capture_timer_config_t  cap_conf;
    uint32_t                      resolution_hz;  /* actual hw resolution, queried after creation */
} cap_timer_t;

/*
 * Class-level data — one instance shared by all pulse_decoder objects.
 */
typedef struct {
    QueueHandle_t              queue;
    TaskHandle_t               capture_task;
    cap_timer_t                timer[TOTAL_CAPTURE_GROUPS];
    uint8_t                    object_count;
    pulseDecoderEventCallback  cb;
    void                      *context;
} pwm_capture_class_data_t;

static pwm_capture_class_data_t g_class = {0};

/*
 * Per-instance object.
 * pulse_widths[] is a Flexible Array Member sized to total_signals at malloc.
 */
typedef struct {
    uint8_t                    gpio_num;
    uint8_t                     line_num; //if created as a group like keypad, then col no,
    mcpwm_cap_timer_handle_t   cap_timer;
    mcpwm_cap_channel_handle_t cap_chan;
    pulse_decoder_interface_t  interface;       /* embedded vtable — always use &this */
    uint32_t                   time_stamp;      /* rising-edge tick captured in ISR   */
    uint8_t                    total_signals;
    uint32_t                   timer_resolution_hz; /* copied from cap_timer_t at create time */
    uint32_t                   min_width_ticks; /* ISR filter lower bound in ticks    */
    uint32_t                   max_width_ticks; /* ISR filter upper bound in ticks    */
    uint32_t                   min_width_us;    /* task filter lower bound in µs      */
    uint32_t                   tolerance;       /* ± µs tolerance for ID matching     */
    uint32_t                   pulse_widths[];  /* sorted expected widths in µs (FAM) */
} pulse_decoder_t;

/* Item placed on the queue by the ISR */
typedef struct {
    const pulse_decoder_t *cap_obj;
    uint32_t               pulse_width_ticks;
} capture_event_data_t;

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static int compare_uint32(const void *a, const void *b)
{
    uint32_t ua = *(const uint32_t *)a;
    uint32_t ub = *(const uint32_t *)b;
    return (ua > ub) - (ua < ub);
}

static void sort_uint32_array(const uint32_t *input, uint32_t *output, size_t n)
{
    memcpy(output, input, n * sizeof(uint32_t));
    qsort(output, n, sizeof(uint32_t), compare_uint32);
}

/* Convert µs → ticks using the object's actual timer resolution */
static uint32_t us_to_ticks(const pulse_decoder_t *obj, uint32_t us)
{
    return (uint32_t)((uint64_t)us * obj->timer_resolution_hz / 1000000ULL);
}

/* Convert ticks → µs using the object's actual timer resolution */
static uint32_t ticks_to_us(const pulse_decoder_t *obj, uint32_t ticks)
{
    return (uint32_t)((uint64_t)ticks * 1000000ULL / obj->timer_resolution_hz);
}

/* Resolve a received pulse width (µs) to an index in pulse_widths[]. */
static int match_pulse_id(const pulse_decoder_t *obj, uint32_t received_us)
{
    for (uint8_t i = 0; i < obj->total_signals; i++) {
        uint32_t diff = (received_us >= obj->pulse_widths[i])
                        ? received_us - obj->pulse_widths[i]
                        : obj->pulse_widths[i] - received_us;

        //ESP_LOGI(TAG,"%lu    %lu",received_us,obj->pulse_widths[i]);
        if (diff <= obj->tolerance)
            return (int)i;
    }
    return ERR_CAPTURE_UNREGISTERD_PULSE_WIDTH;
}

/* ── FreeRTOS task ───────────────────────────────────────────────────────── */

static void task_processCaptureQueue(void *args)
{
    pwm_capture_class_data_t *cd = (pwm_capture_class_data_t *)args;
    capture_event_data_t      evt;

    while (1) {
        if (xQueueReceive(cd->queue, &evt, portMAX_DELAY) != pdTRUE)
            continue;

        const pulse_decoder_t *obj = evt.cap_obj;

        uint32_t pulse_us = ticks_to_us(obj, evt.pulse_width_ticks);

        
        if (pulse_us < obj->min_width_us)
            continue;

        ESP_LOGI(TAG, "GPIO %d: measured pulse = %lu µs",
            obj->gpio_num, (unsigned long)pulse_us);



        int id = match_pulse_id(obj, pulse_us);
        if (id >= 0 && cd->cb) {
            pulse_decoder_event_data_t event = {
                .line_number   = obj->line_num,
                .source_number = (uint8_t)id,
                .gpio_number=obj->gpio_num,
                .pulse_width=pulse_us
            };
            cd->cb(&event, cd->context);
        }
    }
}

/* ── ISR callback ────────────────────────────────────────────────────────── */

static bool captureCallback(mcpwm_cap_channel_handle_t        cap_chan,
                             const mcpwm_capture_event_data_t *edata,
                             void                             *user_data)
{
    (void)cap_chan;
    pulse_decoder_t *self        = (pulse_decoder_t *)user_data;
    BaseType_t       higher_prio = pdFALSE;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        self->time_stamp = edata->cap_value;
    } else {
        uint32_t width = edata->cap_value - self->time_stamp;

        if (width < self->min_width_ticks || width > self->max_width_ticks)
            return false;

        capture_event_data_t evt = {
            .cap_obj           = self,
            .pulse_width_ticks = width,
        };
        xQueueSendFromISR(g_class.queue, &evt, &higher_prio);
    }

    return higher_prio == pdTRUE;
}

/* ── Interface implementations ───────────────────────────────────────────── */

static int startMonitoring(struct pulse_decoder_interface *self)
{
    pulse_decoder_t *obj = CONTAINER_OF(self, pulse_decoder_t, interface);
    esp_err_t        ret;

    ESP_LOGI(TAG, "Starting capture on GPIO %d", obj->gpio_num);

    ret = mcpwm_capture_channel_enable(obj->cap_chan);
    if (ret != ESP_OK) return ret;

    ret = mcpwm_capture_timer_enable(obj->cap_timer);
    if (ret != ESP_OK) return ret;

    return mcpwm_capture_timer_start(obj->cap_timer);
}

static int stopMonitoring(struct pulse_decoder_interface *self)
{
    pulse_decoder_t *obj = CONTAINER_OF(self, pulse_decoder_t, interface);
    esp_err_t        ret;

    ESP_LOGI(TAG, "Stopping capture on GPIO %d", obj->gpio_num);

    ret = mcpwm_capture_timer_stop(obj->cap_timer);
    if (ret != ESP_OK) return ret;

    return mcpwm_capture_channel_disable(obj->cap_chan);
}

static int destroyDecoder(struct pulse_decoder_interface *self)
{
    pulse_decoder_t *obj = CONTAINER_OF(self, pulse_decoder_t, interface);
    esp_err_t        ret = ESP_OK;

    mcpwm_capture_channel_disable(obj->cap_chan);
    ret = mcpwm_del_capture_channel(obj->cap_chan);
    if (ret != ESP_OK)
        ESP_LOGW(TAG, "del_capture_channel failed: %s", esp_err_to_name(ret));

    if (g_class.object_count > 0)
        g_class.object_count--;

    if (g_class.object_count == 0) {
        if (g_class.capture_task) {
            vTaskDelete(g_class.capture_task);
            g_class.capture_task = NULL;
        }
        if (g_class.queue) {
            vQueueDelete(g_class.queue);
            g_class.queue = NULL;
        }
        for (int g = 0; g < TOTAL_CAPTURE_GROUPS; g++) {
            if (g_class.timer[g].cap_timer) {
                mcpwm_capture_timer_stop(g_class.timer[g].cap_timer);
                mcpwm_capture_timer_disable(g_class.timer[g].cap_timer);
                mcpwm_del_capture_timer(g_class.timer[g].cap_timer);
                g_class.timer[g].cap_timer    = NULL;
                g_class.timer[g].resolution_hz = 0;
            }
        }
        ESP_LOGI(TAG, "All capture resources released");
    }

    free(obj);
    return (int)ret;
}

/* ── Class-level initialiser ─────────────────────────────────────────────── */

static esp_err_t pulseDecoderClassDataInit(void)
{
    
    if (g_class.object_count >= MAX_CHANNELS) {
        ESP_LOGE(TAG, "Maximum capture channels (%d) already in use", MAX_CHANNELS);
        return ERR_CAPTURE_CAP_UNIT_EXCEED;
    }
    

    /* Create queue + task once on first call */
    if (g_class.queue == NULL) {
        g_class.queue = xQueueCreate(QUEUE_LENGTH, sizeof(capture_event_data_t));
        if (!g_class.queue) {
            ESP_LOGE(TAG, "Failed to create capture queue");
            return ERR_CAPTURE_MEM_ALLOC;
        }

        
        if (xTaskCreate(task_processCaptureQueue,
                        "captureTask", 4096,
                        &g_class, 5,
                        &g_class.capture_task) != pdPASS) {
            vQueueDelete(g_class.queue);
            g_class.queue = NULL;
            ESP_LOGE(TAG, "Failed to create capture task");
            return ERR_CAPTURE_MEM_ALLOC;
        }
    }

    
    /* Create a new timer whenever we step into a new group (every 3 objects) */
    if (g_class.object_count % MAX_CHANNELS_PER_UNIT == 0) {
        uint8_t group = g_class.object_count / MAX_CHANNELS_PER_UNIT;

        
        mcpwm_capture_timer_config_t conf = {
            .clk_src       = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
            .group_id      = (int)group,
            /*
             * Request 1 MHz (1 tick = 1 µs).
             * Honoured on ESP32-S3 / C3 / H2 and any chip with
             * SOC_MCPWM_CAPTURE_CLK_FROM_GROUP.
             * Silently ignored on the original ESP32 (no prescaler).
             * We always query the actual value below.
             */
            .resolution_hz = 1000000,
        };

        
        esp_err_t ret=0;
        ret = mcpwm_new_capture_timer(&conf, &g_class.timer[group].cap_timer);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create capture timer for group %d: %s",
                     group, esp_err_to_name(ret));
            return ret;
        }

        
        /*
         * Query the resolution the hardware is ACTUALLY running at.
         * This is the only portable way to get the right value —
         * on ESP32 it returns APB (80 MHz), on newer chips it returns
         * whatever was accepted from resolution_hz.
         */
        uint32_t actual_hz = 0;
        ret = mcpwm_capture_timer_get_resolution(g_class.timer[group].cap_timer,
                                                  &actual_hz);
        if (ret != ESP_OK || actual_hz == 0) {
            /* Should never happen, but fall back to APB if it does */
            actual_hz = (uint32_t)esp_clk_apb_freq();
            ESP_LOGW(TAG, "get_resolution failed, falling back to APB (%lu Hz)",
                     (unsigned long)actual_hz);
        }
        
        g_class.timer[group].cap_conf      = conf;
        g_class.timer[group].resolution_hz = actual_hz;

        ESP_LOGI(TAG, "Created capture timer for group %d, actual resolution %lu Hz",
                 group, (unsigned long)actual_hz);
    }

    

    g_class.object_count++;
    return ESP_OK;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

esp_err_t pulseDecoderCreate(pulse_decoder_config_t    *config,
                              pulse_decoder_interface_t **out_if)
{
    if (!config || !out_if || !config->pulse_widths_us || config->total_signals == 0)
        return ESP_ERR_INVALID_ARG;

    

    esp_err_t ret = pulseDecoderClassDataInit();
    if (ret != ESP_OK)
        return ret;


    if (config->cb)
        g_class.cb = config->cb;
    if (config->context)
        g_class.context = config->context;

    pulse_decoder_t *obj =
        calloc(1, sizeof(pulse_decoder_t) +
                  config->total_signals * sizeof(uint32_t));
    
    if (!obj) {
        g_class.object_count--;
        return ESP_ERR_NO_MEM;
    }

    obj->gpio_num      = config->gpio_num;
    obj->line_num       = config->line_num;
    obj->total_signals = config->total_signals;
    obj->tolerance     = config->tolerance_us;

    sort_uint32_array(config->pulse_widths_us,
                      obj->pulse_widths,
                      config->total_signals);

    /* Assign timer — group is determined by how many objects already exist */
    uint8_t index  = g_class.object_count - 1;
    uint8_t group  = index / MAX_CHANNELS_PER_UNIT;
    obj->cap_timer             = g_class.timer[group].cap_timer;
    obj->timer_resolution_hz   = g_class.timer[group].resolution_hz;

    /*
     * Expand the filter window by tolerance on both sides so that a valid
     * pulse at ±tolerance from the narrowest/widest registered width is not
     * dropped before match_pulse_id() can evaluate it.
     */
    uint32_t min_us        = obj->pulse_widths[0];
    uint32_t max_us        = obj->pulse_widths[config->total_signals - 1];
    uint32_t filter_min_us = (min_us > config->tolerance_us)
                             ? min_us - config->tolerance_us : 0;
    uint32_t filter_max_us = max_us + config->tolerance_us;

    obj->min_width_us    = filter_min_us;
    obj->min_width_ticks = us_to_ticks(obj, filter_min_us);
    obj->max_width_ticks = us_to_ticks(obj, filter_max_us);

    ESP_LOGI(TAG, "GPIO %d: filter window %lu–%lu µs (%lu–%lu ticks @ %lu Hz)",
             obj->gpio_num,
             (unsigned long)filter_min_us, (unsigned long)filter_max_us,
             (unsigned long)obj->min_width_ticks, (unsigned long)obj->max_width_ticks,
             (unsigned long)obj->timer_resolution_hz);

    /* Create capture channel */
    mcpwm_capture_channel_config_t ch_conf = {
        .gpio_num       = obj->gpio_num,
        .prescale       = 1,
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        .flags.pull_down=true,
        .flags.pull_up=false
    };

    ret = mcpwm_new_capture_channel(obj->cap_timer, &ch_conf, &obj->cap_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create capture channel: %s", esp_err_to_name(ret));
        goto err_free;
    }

    mcpwm_capture_event_callbacks_t cbs = { .on_cap = captureCallback };
    ret = mcpwm_capture_channel_register_event_callbacks(obj->cap_chan, &cbs, obj);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callbacks: %s", esp_err_to_name(ret));
        mcpwm_del_capture_channel(obj->cap_chan);
        goto err_free;
    }

    obj->interface.startMonitoring = startMonitoring;
    obj->interface.stopMonitoring  = stopMonitoring;
    obj->interface.destroy         = destroyDecoder;

    *out_if = &obj->interface;

    ESP_LOGI(TAG, "Created decoder on GPIO %d (group %d, index %d)",
             obj->gpio_num, group, index);
    return ESP_OK;

err_free:
    free(obj);
    g_class.object_count--;
    return ret;
}