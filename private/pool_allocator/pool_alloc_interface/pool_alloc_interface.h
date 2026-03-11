#ifndef POOL_ALLOC_INTERFACE_H
#define POOL_ALLOC_INTERFACE_H




struct pool_alloc_interface{


    /// @brief It return the pointer to the object being allocated
    void* (*poolDrain)(struct pool_alloc_interface* self);
    /*The single pointer is not used  because the parameter then just becomes local copy of the pointer
    whereas we need to modify the actual pointer (set it to NULL after copying)*/

    /// @brie The double pointer is changed to single pointer, since double poointer cannot
    //// be used to fill the pool initally from an array of objects as it is just one  pointer
    int (*poolFill)(struct pool_alloc_interface* self,void* object);
};


typedef struct pool_alloc_interface pool_alloc_interface_t;















#endif