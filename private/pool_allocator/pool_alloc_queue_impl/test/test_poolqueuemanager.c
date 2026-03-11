#include <stddef.h>
#include "pool_queue.h"
#include "unity.h"
#include "esp_log.h"

static const char* TAG="test pool queue";


typedef struct some_struct{
    int a;
    char b;
    float c;

}some_struct_t;

#define         TOTAL_ELEMENTS          5
#define         SIZE_OF_VOID            4
#define         QUEUE_SIZE              3


static some_struct_t array[TOTAL_ELEMENTS]={
                            {.a=1,
                            .b='h',
                            .c=4.5},
                            {.a=2,
                            .b='i',
                            .c=5.5},
                            {.a=3,
                            .b='j',
                            .c=6.5},
                            {.a=4,
                            .b='k',
                            .c=7.5},
                            {.a=5,
                            .b='l',
                            .c=8.5},
};


static pool_queue_t* manager;
static pool_alloc_interface_t* interface;


//Setup will run before any test. The state is returned to inital after each test
//Thus state is not ,maintained, so new object must be created before each test

static some_struct_t* object1;
static some_struct_t* object2;
static some_struct_t* object3;
static some_struct_t* object4;


void setUp() {
    ESP_LOGI(TAG, "Setting up test environment...");
    size_t size=poolQueueGetSize(3);
    ESP_LOGI(TAG,"size %d",size);
    

    manager=(pool_queue_t*) malloc(size);
    object1=&array[0];
    object2=&array[1];
    object3=&array[2];
    object4=&array[3];

    interface=poolQueueCreate(manager,QUEUE_SIZE,sizeof(void*));
    interface->poolFill(interface,(void**)&object1);
    interface->poolFill(interface,(void**)&object2);
    interface->poolFill(interface,(void**)&object3);
    
        
}






static some_struct_t* allocateObject(){

    return interface->poolDrain(interface);
}

static int deallocateObject(some_struct_t** object){

    return interface->poolFill(interface,(void**)object);
}

static void printObject(some_struct_t* object){

    ESP_LOGI(TAG,"a %d  b %c   c%f",object->a,object->b,object->c);
}


//All allocation

TEST_CASE("PoolQueue Allocate 4 , Deallocate 1","[Unit Test: PoolQueue]"){
    
    
    object1=allocateObject();
    if (object1==NULL)
        ESP_LOGI(TAG,"failed 1");
    else
        ESP_LOGI(TAG,"true 1");
    object2=allocateObject();
    if (object2==NULL)
        ESP_LOGI(TAG,"failed 2");
    else
        ESP_LOGI(TAG,"true 2");
    object3=allocateObject();
    if (object3==NULL)
        ESP_LOGI(TAG,"failed 3");
    else
        ESP_LOGI(TAG,"true 3");
    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed 4");
    else
        ESP_LOGI(TAG,"true 4");

    int ret= deallocateObject(&object2);
    ESP_LOGI(TAG,"free ret %d",ret);

    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed after  free");
    else{
        ESP_LOGI(TAG,"true after free");
        printObject(object4);
    
    }

}



TEST_CASE("PoolQueue Allocate 4 , Deallocate 2, Alloc2","[Unit Test: PoolQueue]"){
    
    object1=allocateObject();
    if (object1==NULL)
        ESP_LOGI(TAG,"failed 1");
    else
        ESP_LOGI(TAG,"true 1");
    object2=allocateObject();
    if (object2==NULL)
        ESP_LOGI(TAG,"failed 2");
    else
        ESP_LOGI(TAG,"true 2");
    object3=allocateObject();
    if (object3==NULL)
        ESP_LOGI(TAG,"failed 3");
    else
        ESP_LOGI(TAG,"true 3");
    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed 4");
    else
        ESP_LOGI(TAG,"true 4");

    int ret= deallocateObject(&object2);
    ESP_LOGI(TAG,"free ret %d",ret);

    ret= deallocateObject(&object3);
    ESP_LOGI(TAG,"free ret %d",ret);

    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed after  free");
    else{
        ESP_LOGI(TAG,"true after free");
        printObject(object4);
    }
  
    object3=allocateObject();
    if (object3==NULL)
        ESP_LOGI(TAG,"failed after  free");
    else{
        ESP_LOGI(TAG,"true after free");
        printObject(object3);
    }

}


TEST_CASE("PoolQueue Deallocate same object twice","[Unit Test: PoolQueue]"){
    
    object1=allocateObject();
    if (object1==NULL)
        ESP_LOGI(TAG,"failed 1");
    else
        ESP_LOGI(TAG,"true 1");
    object2=allocateObject();
    if (object2==NULL)
        ESP_LOGI(TAG,"failed 2");
    else
        ESP_LOGI(TAG,"true 2");
    object3=allocateObject();
    if (object3==NULL)
        ESP_LOGI(TAG,"failed 3");
    else
        ESP_LOGI(TAG,"true 3");
    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed 4");
    else
        ESP_LOGI(TAG,"true 4");

    int ret= deallocateObject(&object2);
    ESP_LOGI(TAG,"free ret %d",ret);

  
    ret= deallocateObject(&object2);
    ESP_LOGI(TAG,"free ret %d",ret);

    object4=allocateObject();
    if (object4==NULL)
        ESP_LOGI(TAG,"failed after  free 1");

    else{
        ESP_LOGI(TAG,"true after free 1");
        printObject(object4);
    }
  
    object3=allocateObject();
    if (object3==NULL)
        ESP_LOGI(TAG,"failed after  free 2");
    else{
        ESP_LOGI(TAG,"true after free 2");
        printObject(object3);
    }

}

