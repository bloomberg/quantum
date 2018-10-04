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
#ifndef QUANTUM_ALLOCATOR_TRAITS_H
#define QUANTUM_ALLOCATOR_TRAITS_H

#include <cstdint>

namespace Bloomberg {
namespace quantum {

#ifndef __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
    #ifdef __QUANTUM_DEFAULT_STACK_ALLOC_SIZE
        #warning Deprecated : use __QUANTUM_DEFAULT_POOL_ALLOC_SIZE instead.
        #define __QUANTUM_DEFAULT_POOL_ALLOC_SIZE __QUANTUM_DEFAULT_STACK_ALLOC_SIZE
    #else
        #define __QUANTUM_DEFAULT_POOL_ALLOC_SIZE 1000
    #endif
#endif

#ifndef __QUANTUM_DEFAULT_CORO_POOL_ALLOC_SIZE
    #define __QUANTUM_DEFAULT_CORO_POOL_ALLOC_SIZE 200
#endif

//==============================================================================================
//                                 struct AllocatorTraits
//==============================================================================================
struct AllocatorTraits {
    using size_type = uint16_t;
    
    /**
     * @brief Get/set if the system allocator should be used for internal objects (other than coroutine stacks).
     * @return A modifiable reference to the value.
     * @remark For future use.
     */
    static bool& useDefaultAllocator() {
#ifdef __QUANTUM_USE_DEFAULT_ALLOCATOR
        static bool value = true;
#else
        static bool value = false;
#endif
        return value;
    }
    
    /**
     * @brief Get/set if the system allocator should be used for coroutine stacks.
     * @return A modifiable reference to the value.
     * @remark For future use.
     */
    static bool& useDefaultCoroAllocator() {
#ifdef __QUANTUM_USE_DEFAULT_CORO_ALLOCATOR
        static bool value = true;
#else
        static bool value = false;
#endif
        return value;
    }
    
    /**
     * @brief Get/set if the allocator pool for internal objects should use the heap or the application stack.
     * @return A modifiable reference to the value.
     * @remark For future use. If set to 'false', object pools will be allocated on the stack.
     */
    static bool& allocatePoolFromHeap() {
#ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        static bool value = true;
#else
        static bool value = false;
#endif
        return value;
    }
    
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
        static size_type size = defaultPoolAllocSize();
        return size;
    }
    
    /**
     * @brief Get/set if the default size for future object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& futureAllocSize() {
        static size_type size = defaultPoolAllocSize();
        return size;
    }
    
    /**
     * @brief Get/set if the default size for context object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& contextAllocSize() {
        static size_type size = defaultPoolAllocSize();
        return size;
    }
    
    /**
     * @brief Get/set if the default size for task object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& taskAllocSize() {
        static size_type size = defaultPoolAllocSize();
        return size;
    }
    
    /**
     * @brief Get/set if the default size for IO task object pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& ioTaskAllocSize() {
        static size_type size = defaultPoolAllocSize();
        return size;
    }
    
    /**
     * @brief Get/set if the default size for task queue pools.
     * @return A modifiable reference to the value.
     * @remark Normally this should not be modified unless very specific tuning is needed.
     */
    static size_type& queueListAllocSize() {
        static size_type size = defaultPoolAllocSize();
        return size;
    }
};

}}

#endif //QUANTUM_ALLOCATOR_TRAITS_H
