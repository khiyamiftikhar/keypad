/*implements the pool alloc interface using


*/


#include "pool_queue.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"



#define MAX_ELEMENTS    CONFIG_MAX_ELEMENTS     //Total elements a pool can host
                                                //Due to static allocation limitation, it is same for all pool objects
#define TIME_OUT        CONFIG_MAX_WAIT_TIME    //To be put in xQueueSend and xQueueReceive
#define MAX_POOLS       CONFIG_MAX_POOLS        //Total pool objects. The above max element limitation 
                                                //applies here. All pools will share the same max elements

#define configTICK_RATE_HZ  CONFIG_FREERTOS_HZ

#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})


static const char* TAG="queue pool";



typedef struct queue_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(void*)*MAX_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_handle_t;


typedef struct pool_queue{
    queue_handle_t queue;
    pool_alloc_interface_t interface;
    uint8_t issued;                     //count of how many objects already taken
    portMUX_TYPE spinlock;
}pool_queue_t;


/*This may seem confusing. Actuall this articraft is a pool of objects to get/return dynamically
 and here we create a pool of the pool_queue. It means multiple users can create their own pool
 objects from this array. 
*/
typedef struct pool_queue_pool{
    pool_queue_t pool_array[MAX_POOLS];     //An array of pools
    uint8_t count;                          //To keep track of how many allocated
}pool_queue_pool_t;


static pool_queue_pool_t pool_list;         //Memeory will be provided from here


typedef enum{decrement=0,increment}access_type;

static int issueCountAccess(uint8_t* shared_variable,uint8_t max_val, portMUX_TYPE* spinlock, access_type t){

    taskENTER_CRITICAL(spinlock);
    if(t==decrement){
        (*shared_variable)--;
        taskEXIT_CRITICAL(spinlock);
        return 0;
    }
    //Otherwise it is the increment operation
    else if(*shared_variable==max_val){
        taskEXIT_CRITICAL(spinlock);
        return ERR_POOL_ALLOC_POOL_EMPTY;                                  //Failure
    }

    (*shared_variable)++;                   //increment option, So counter not max yet, thus book next one also now
    taskEXIT_CRITICAL(spinlock);
    return 0;
}


static int poolQueueGiveBack(pool_alloc_interface_t * self,void* object){

    

    if(self==NULL ||  object==NULL)
        return ERR_POOL_ALLOC_INVALID_MEM;

    //ESP_LOGI(TAG,"address %p",object);  //Address which the pointer holds i.e value of the pointer
    struct pool_queue* pool_queue=container_of(self,struct pool_queue,interface);

    if(pool_queue->queue.handle==NULL)
        return ERR_POOL_ALLOC_INVALID_MEM;

    //Max count not required here , so set to 0;
    issueCountAccess(&pool_queue->issued,0,&pool_queue->spinlock,decrement);
        
    
    /*A queue accepts an address and copies what ever in in that address. 
        If a pointer is passeed which is essentially an address, it copies from that address
        if the address of the pointer ( and not the address which the pointer holds) is passed,
        it copies the address which the pointer has, i.e value of the pointer

    */
    if(xQueueSend(pool_queue->queue.handle,&object,pdMS_TO_TICKS(TIME_OUT))==pdTRUE){
        //So that the user
        
        //Below instruction removed , now it is th duty of the user to set the pointer to NULL
        
       /* 
        *object=NULL;    Setting the pointer passed by the user as NULL. there is a chance that
                            user might use this pointer between the xqueuesend and setting it to NULL
                            User will have then access to the object which is erroneous. but the 
                            assumption is that wont happen. although the user code will also set
                            its pointer to NULL afterwards as a redundant extra safety mechanism
                        */
        return 0;
    }

    
    return ERR_POOL_ALLOC_TIMEOUT;
}







static void* poolQueueAllocate(pool_alloc_interface_t* self){

    if(self==NULL)
        return NULL;

    
    

    struct pool_queue* pool_queue=container_of(self,struct pool_queue,interface);

    if(pool_queue->queue.handle==NULL)
        return NULL;

    //Check whether all the objeects are already allocated, if not then increment count
    if(issueCountAccess(&pool_queue->issued,MAX_ELEMENTS,&pool_queue->spinlock,increment)==ERR_POOL_ALLOC_POOL_EMPTY)
        return NULL;    //It means all elements already issued
    

    void * temp;
    if(xQueueReceive(pool_queue->queue.handle,&temp,pdMS_TO_TICKS(TIME_OUT))==pdTRUE){
        //ESP_LOGI(TAG,"read address %p",temp);
        return temp;
    }


    return NULL;
}




/*Not needed

size_t poolQueueGetSize(int total_elements){
    size_t size=sizeof(pool_queue_t);

    //The Queue will store the void pointers to the objects instead of actual object
      //  So a memory equal to total void pointers must be allocated because queue creates
    //    copy and does not reuse the pointers passed
    
    size+=sizeof(void*)*total_elements;

    return size;
}

*/

/// @brief Get one element(i.e a pool object out of pool array) of pool, and increment the count. Not thread safe
/// @return 
static pool_queue_t* poolGet(){
    
    if(pool_list.count==MAX_POOLS)
        return NULL;
    
    pool_queue_t* self=&pool_list.pool_array[pool_list.count];
    pool_list.count++;
    return self;

}

//So it assumes that return is in the same order as get, so very simplisitic, also  not thread safe
static void poolReturn(){

    pool_list.count--;

}





pool_alloc_interface_t* poolQueueCreate(){

    
    pool_queue_t* self=poolGet();

    if(self==NULL)
        return NULL;
    //self->queue.buff=object_array;

    self->queue.handle=xQueueCreateStatic(MAX_ELEMENTS,sizeof(void*),self->queue.buff,&self->queue.queue_meta_data);
    

    //Just decrements the count
    if(self->queue.handle==NULL){
        poolReturn();
        return NULL;
    }

    self->interface.poolDrain=poolQueueAllocate;
    self->interface.poolFill=poolQueueGiveBack;
    self->spinlock.owner=portMUX_FREE_VAL;
    self->spinlock.count=0;

    return &self->interface;
}

