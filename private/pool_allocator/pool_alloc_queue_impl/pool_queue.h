#ifndef POOL_QUEUE_H
#define POOL_QUEUE_H

#include "stddef.h"
#include "pool_alloc_interface.h"


#define         ERR_POOL_ALLOC_BASE             0
#define         ERR_POOL_ALLOC_INVALID_MEM      (ERR_POOL_ALLOC_BASE-1)
#define         ERR_POOL_ALLOC_POOL_EMPTY       (ERR_POOL_ALLOC_BASE-2)
#define         ERR_POOL_ALLOC_TIMEOUT          (ERR_POOL_ALLOC_BASE-3)

//Queue stores pointers






/// @brief No config struct because objecct size is fixed to be void pointers. and other parameters are set
///         through Kconfig
/// @return 
pool_alloc_interface_t* poolQueueCreate();



#endif