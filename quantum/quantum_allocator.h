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
#ifndef BLOOMBERG_QUANTUM_ALLOCATOR_H
#define BLOOMBERG_QUANTUM_ALLOCATOR_H

#include <quantum/impl/quantum_stl_impl.h>
#include <quantum/quantum_allocator_traits.h>
#include <quantum/quantum_stack_allocator.h>
#include <quantum/quantum_heap_allocator.h>
#include <quantum/quantum_coroutine_pool_allocator.h>
#include <boost/context/stack_traits.hpp>
#include <boost/coroutine2/pooled_fixedsize_stack.hpp>
#include <boost/coroutine2/fixedsize_stack.hpp>
#include <memory>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 struct StlAllocator
//==============================================================================================
template <typename T>
struct StlAllocator : public std::allocator<T>
{
    typedef std::true_type default_constructor;
};

//==============================================================================================
//                                 struct BoostAllocator
//==============================================================================================
template <typename Traits>
struct BoostAllocator : public boost::context::basic_fixedsize_stack<Traits>
{
    typedef std::true_type default_constructor;
};

//==============================================================================================
//                               struct Allocator (singleton)
//==============================================================================================
template <typename AllocType>
struct Allocator {
    template <typename A = AllocType>
    static AllocType& instance(std::enable_if_t<!A::default_constructor::value, uint16_t> size) {
       static AllocType allocator(size);
       return allocator;
    }
    template <typename A = AllocType>
    static AllocType& instance(std::enable_if_t<A::default_constructor::value, uint16_t> = 0) {
       static AllocType allocator;
       return allocator;
    }
};

}
}

#endif //BLOOMBERG_QUANTUM_ALLOCATOR_H
