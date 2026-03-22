#include <stdio.h>
#include "unity.h"
#include "esp_log.h"
#include "matrix_keypad_nkro.h"

static const char *TAG = "test_keypad";

/* ──────────────────────────────────────────────
 * Shared test state
 * ────────────────────────────────────────────── */

static keypad_interface_t *s_keypad = NULL;

static volatile int s_event_count = 0;
static volatile key_event_t s_last_event;
static volatile uint8_t s_last_key;
static volatile uint32_t s_last_ts;

/* GPIO layout used in tests */

static uint8_t row_gpios[] = {12,13,14,15};
static uint8_t col_gpios[] = {18,19,22,23};

/* Default keypad configuration */

static keypad_config_t s_default_config =
{
    .cb = NULL,
    .col_gpios = col_gpios,
    .row_gpios = row_gpios,
    .total_cols = 4,
    .total_rows = 4,
    .keymap =
    {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    }
};

/* ──────────────────────────────────────────────
 * Test fixtures
 * ────────────────────────────────────────────── */

void setUp(void)
{
    s_event_count = 0;
    s_last_key = 0;
    s_last_event = KEY_PRESSED;
    s_last_ts = 0;
    s_keypad = NULL;
}

void tearDown(void)
{
    /* destroy keypad when API becomes available */

    /*
    if(s_keypad)
        keypadDestroy(s_keypad);
    */

    s_keypad = NULL;
}

/* ──────────────────────────────────────────────
 * Helper callback
 * ────────────────────────────────────────────── */

static void test_event_cb(key_event_t event, keypad_event_data_t *evt_data)
{
    s_event_count++;

    s_last_event = event;
    s_last_key = evt_data->key_id;
    s_last_ts = evt_data->time_stamp;

    ESP_LOGI(TAG,
             "event=%s key=%c ts=%lu count=%d",
             key_event_to_str(event),
             evt_data->key_id,
             (unsigned long)evt_data->time_stamp,
             s_event_count);
}

/* ══════════════════════════════════════════════
 * 1. API Creation Tests
 * ══════════════════════════════════════════════ */

TEST_CASE("keypadCreate succeeds with valid configuration", "[keypad][api]")
{
    keypad_config_t cfg = s_default_config;
    cfg.cb = test_event_cb;

    s_keypad = keypadCreate(&cfg);

    TEST_ASSERT_NOT_NULL(s_keypad);
}

TEST_CASE("keypadCreate returns NULL when config is NULL", "[keypad][api]")
{
    keypad_interface_t *kp = keypadCreate(NULL);

    TEST_ASSERT_NULL(kp);
}

/* ══════════════════════════════════════════════
 * 2. Configuration Validation
 * ══════════════════════════════════════════════ */

TEST_CASE("keypadCreate fails when callback is NULL", "[keypad][config]")
{
    keypad_config_t cfg = s_default_config;
    cfg.cb = NULL;

    keypad_interface_t *kp = keypadCreate(&cfg);

    TEST_ASSERT_NULL(kp);
}

TEST_CASE("keypadCreate fails when row gpio array is NULL", "[keypad][config]")
{
    keypad_config_t cfg = s_default_config;

    cfg.cb = test_event_cb;
    cfg.row_gpios = NULL;

    keypad_interface_t *kp = keypadCreate(&cfg);

    TEST_ASSERT_NULL(kp);
}

TEST_CASE("keypadCreate fails when column gpio array is NULL", "[keypad][config]")
{
    keypad_config_t cfg = s_default_config;

    cfg.cb = test_event_cb;
    cfg.col_gpios = NULL;

    keypad_interface_t *kp = keypadCreate(&cfg);

    TEST_ASSERT_NULL(kp);
}

TEST_CASE("keypadCreate fails when rows = 0", "[keypad][config]")
{
    keypad_config_t cfg = s_default_config;

    cfg.cb = test_event_cb;
    cfg.total_rows = 0;

    keypad_interface_t *kp = keypadCreate(&cfg);

    TEST_ASSERT_NULL(kp);
}

TEST_CASE("keypadCreate fails when cols = 0", "[keypad][config]")
{
    keypad_config_t cfg = s_default_config;

    cfg.cb = test_event_cb;
    cfg.total_cols = 0;

    keypad_interface_t *kp = keypadCreate(&cfg);

    TEST_ASSERT_NULL(kp);
}

/* ══════════════════════════════════════════════
 * 3. Event String Conversion
 * ══════════════════════════════════════════════ */

TEST_CASE("key_event_to_str returns correct string for KEY_PRESSED", "[keypad][event]")
{
    TEST_ASSERT_EQUAL_STRING("KEY_PRESSED",
                             key_event_to_str(KEY_PRESSED));
}

TEST_CASE("key_event_to_str returns correct string for KEY_RELEASED", "[keypad][event]")
{
    TEST_ASSERT_EQUAL_STRING("KEY_RELEASED",
                             key_event_to_str(KEY_RELEASED));
}

TEST_CASE("key_event_to_str returns correct string for KEY_LONG_PRESSED", "[keypad][event]")
{
    TEST_ASSERT_EQUAL_STRING("KEY_LONG_PRESSED",
                             key_event_to_str(KEY_LONG_PRESSED));
}

TEST_CASE("key_event_to_str returns correct string for KEY_REPEATED", "[keypad][event]")
{
    TEST_ASSERT_EQUAL_STRING("KEY_REPEATED",
                             key_event_to_str(KEY_REPEATED));
}

TEST_CASE("key_event_to_str returns UNKNOWN_EVENT for invalid event", "[keypad][event]")
{
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_EVENT",
                             key_event_to_str((key_event_t)255));
}

/* ══════════════════════════════════════════════
 * 4. Callback Data Routing
 * ══════════════════════════════════════════════ */

TEST_CASE("callback receives correct event data", "[keypad][callback]")
{
    keypad_event_data_t evt =
    {
        .event = KEY_PRESSED,
        .key_id = '5',
        .time_stamp = 100
    };

    test_event_cb(KEY_PRESSED, &evt);

    TEST_ASSERT_EQUAL(1, s_event_count);
    TEST_ASSERT_EQUAL(KEY_PRESSED, s_last_event);
    TEST_ASSERT_EQUAL_CHAR('5', s_last_key);
    TEST_ASSERT_EQUAL_UINT32(100, s_last_ts);
}

TEST_CASE("press hold repeat release sequence updates state correctly", "[keypad][callback]")
{
    const struct
    {
        key_event_t ev;
        uint8_t key;
        uint32_t ts;

    } sequence[] =
    {
        {KEY_PRESSED, '2', 100},
        {KEY_LONG_PRESSED, '2', 1000},
        {KEY_REPEATED, '2', 1500},
        {KEY_RELEASED, '2', 1600}
    };

    for(int i = 0; i < 4; i++)
    {
        keypad_event_data_t evt =
        {
            .event = sequence[i].ev,
            .key_id = sequence[i].key,
            .time_stamp = sequence[i].ts
        };

        test_event_cb(sequence[i].ev, &evt);
    }

    TEST_ASSERT_EQUAL(4, s_event_count);
    TEST_ASSERT_EQUAL(KEY_RELEASED, s_last_event);
    TEST_ASSERT_EQUAL_CHAR('2', s_last_key);
    TEST_ASSERT_EQUAL_UINT32(1600, s_last_ts);
}

/* ══════════════════════════════════════════════
 * 5. Robustness Tests
 * ══════════════════════════════════════════════ */

TEST_CASE("repeated keypadCreate calls succeed", "[keypad][stress]")
{
    for(int i=0;i<10;i++)
    {
        keypad_config_t cfg = s_default_config;

        cfg.cb = test_event_cb;

        keypad_interface_t *kp = keypadCreate(&cfg);

        TEST_ASSERT_NOT_NULL(kp);

        /* keypadDestroy(kp); */
    }
}

TEST_CASE("rapid event burst preserves last event state", "[keypad][stress]")
{
    const char keys[] =
    {
        '1','2','3','A',
        '4','5','6','B',
        '7','8','9','C',
        '*','0','#','D'
    };

    int len = sizeof(keys);

    for(int i=0;i<len;i++)
    {
        keypad_event_data_t evt =
        {
            .event = KEY_PRESSED,
            .key_id = keys[i],
            .time_stamp = i * 10
        };

        test_event_cb(KEY_PRESSED, &evt);
    }

    TEST_ASSERT_EQUAL(len, s_event_count);
    TEST_ASSERT_EQUAL_CHAR('D', s_last_key);
    TEST_ASSERT_EQUAL(KEY_PRESSED, s_last_event);
}