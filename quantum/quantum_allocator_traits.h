/*
** Copyright 2018 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef BLOOMBERG_QUANTUM_ALLOCATOR_TRAITS_H
#define BLOOMBERG_QUANTUM_ALLOCATOR_TRAITS_H

#include <cstdint>

namespace Bloomberg {
namespace quantum {

#ifdef __QUANTUM_BOOST_USE_SEGMENTED_STACKS
    // For segmented stack support
    #define BOOST_USE_SEGMENTED_STACKS
    #define BOOST_USE_UCONTEXT
#endif

#ifndef __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
    #define __QUANTUM_DEFAULT_POOL_ALLOC_SIZE 1000
#endif

#ifndef __QUANTUM_DEFAULT_CORO_POOL_ALLOC_SIZE
    #define __QUANTUM_DEFAULT_CORO_POOL_ALLOC_SIZE 200
#endif

#ifndef __QUANTUM_FUNCTION_ALLOC_SIZE
    #define __QUANTUM_FUNCTION_ALLOC_SIZE 128
#endif

#ifndef __QUANTUM_PROMISE_ALLOC_SIZE
    #define __QUANTUM_PROMISE_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_FUTURE_ALLOC_SIZE
    #define __QUANTUM_FUTURE_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_CONTEXT_ALLOC_SIZE
    #define __QUANTUM_CONTEXT_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_TASK_ALLOC_SIZE
    #define __QUANTUM_TASK_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_IO_TASK_ALLOC_SIZE
    #define __QUANTUM_IO_TASK_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_QUEUE_LIST_ALLOC_SIZE
    #define __QUANTUM_QUEUE_LIST_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

#ifndef __QUANTUM_IO_QUEUE_LIST_ALLOC_SIZE
    #define __QUANTUM_IO_QUEUE_LIST_ALLOC_SIZE __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif

//==============================================================================================
//                                 struct AllocatorTraits
//==============================================================================================
/// @struct AllocatorTraits.
/// @brief Allows application-wide settings for the various allocators used by Quantum.
struct AllocatorTraits {
    using size_type = uint16_t;
    
    /**
     * @brief Get/set if the default size for internal object pools (other than coroutine stacks).
     * @return A modifiable reference to the value.
     */
    static size_type& defaultPoolAllocSize() {
        static size_type size = __QUANTUM_DEFAULT_POOL_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for coroutine stack pools.
     * @return A modifiable reference to the value.
     */
    static size_type& defaultCoroPoolAllocSize() {
        static size_type size = __QUANTUM_DEFAULT_CORO_POOL_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for promise object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& promiseAllocSize() {
        static size_type size = __QUANTUM_PROMISE_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for future object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& futureAllocSize() {
        static size_type size = __QUANTUM_FUTURE_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for context object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& contextAllocSize() {
        static size_type size = __QUANTUM_CONTEXT_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for task object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& taskAllocSize() {
        static size_type size = __QUANTUM_TASK_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for IO task object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& ioTaskAllocSize() {
        static size_type size = __QUANTUM_IO_TASK_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for coroutine queue pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& queueListAllocSize() {
        static size_type size = __QUANTUM_QUEUE_LIST_ALLOC_SIZE;
        return size;
    }
    
    /**
     * @brief Get/set if the default size for async IO queue pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& ioQueueListAllocSize() {
        static size_type size = __QUANTUM_IO_QUEUE_LIST_ALLOC_SIZE;
        return size;
    }
};

}}

#endif //BLOOMBERG_QUANTUM_ALLOCATOR_TRAITS_H
