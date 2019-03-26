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
#ifndef BLOOMBERG_QUANTUM_TRAITS_H
#define BLOOMBERG_QUANTUM_TRAITS_H

#include <quantum/quantum_stack_traits.h>
#include <quantum/quantum_allocator.h>
#include <boost/coroutine2/all.hpp>
#include <iterator>
#include <type_traits>

namespace Bloomberg {
namespace quantum {

#define UNUSED(x) (void)(x) //remove compiler warnings

template <class T, class ALLOCATOR = std::allocator<T>>
class Buffer; //fwd declaration

//==============================================================================================
//                                struct StackTraitsProxy
//==============================================================================================
struct StackTraitsProxy {
    static bool is_unbounded() { return StackTraits::isUnbounded(); }
    static std::size_t page_size() { return StackTraits::pageSize(); }
    static std::size_t default_size() { return StackTraits::defaultSize(); }
    static std::size_t minimum_size() { return StackTraits::minimumSize(); }
    static std::size_t maximum_size() { return StackTraits::maximumSize(); }
};

#ifndef __QUANTUM_USE_DEFAULT_CORO_ALLOCATOR
    using CoroStackAllocator = CoroutinePoolAllocatorProxy<StackTraitsProxy>;
#else
    using CoroStackAllocator = BoostAllocator<StackTraitsProxy>;
#endif

//==============================================================================================
//                                    struct Traits
//==============================================================================================
/// @struct Traits.
/// @brief Contains definitions for various traits used by this library. For internal use only.
struct Traits
{
    using BoostCoro = boost::coroutines2::coroutine<int&>;
    using Yield     = typename BoostCoro::pull_type;
    using Coroutine = typename BoostCoro::push_type;
    
    template <class IT>
    using IsInputIterator = std::enable_if_t<std::is_convertible<typename std::iterator_traits<IT>::iterator_category, std::input_iterator_tag>::value>;
    
    //FUTURE BUFFER TRAIT
    template <class T>
    struct IsBuffer : std::false_type
    {
        using Type = T;
    };
    
    template <class T>
    struct IsBuffer<Buffer<T>> : std::true_type
    {
        using Type = typename Buffer<T>::ValueType;
    };
    
    template <class D, class THIS, class B>
    struct DerivedFrom
    {
        virtual ~DerivedFrom() = default;
        operator B*() { return static_cast<D*>(static_cast<THIS*>(this)); }
        operator B&() { return static_cast<D&>(static_cast<THIS&>(*this)); }
    };
};

template <class T, class V>
using BufferType = std::enable_if_t<Traits::IsBuffer<T>::value &&
                                    !std::is_same<std::decay_t<V>,T>::value &&
                                    std::is_convertible<std::decay_t<V>, typename Traits::IsBuffer<T>::Type>::value>;
template <class T, class V>
using NonBufferType = std::enable_if_t<!Traits::IsBuffer<T>::value && std::is_convertible<std::decay_t<V>,T>::value>;
template <class T>
using BufferRetType = std::enable_if_t<Traits::IsBuffer<T>::value, typename Traits::IsBuffer<T>::Type>;
template <class T>
using NonBufferRetType = std::enable_if_t<!Traits::IsBuffer<T>::value, typename Traits::IsBuffer<T>::Type>;

}}

#include <quantum/impl/quantum_stl_impl.h>

#endif //BLOOMBERG_QUANTUM_TRAITS_H
