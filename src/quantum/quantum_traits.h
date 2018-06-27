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
#ifndef QUANTUM_TRAITS_H
#define QUANTUM_TRAITS_H

#include <boost/coroutine2/all.hpp>
#include <iterator>
#include <type_traits>

namespace Bloomberg {
namespace quantum {

#define UNUSED(x) (void)(x) //remove compiler warnings

template <class T>
class Buffer; //fwd declaration

//==============================================================================================
//                                    struct Traits
//==============================================================================================
/// @struct Traits.
/// @brief Contains definitions for various traits used by this library. For internal use only.
struct Traits
{
    using coro_t    = boost::coroutines2::coroutine<int &>;
    using yield_t   = typename coro_t::pull_type;
    using context_t = typename coro_t::push_type;
    
    //ITERATABLE TRAIT
    template <class T>
    static auto IsIteratableImpl(int)->decltype (
        std::begin(std::declval<T&>()) != std::end(std::declval<T&>()), // begin/end and operator!=
        ++std::declval<decltype(std::begin(std::declval<T&>()))&>(), // operator++
        void(*std::begin(std::declval<T&>())), // operator*
        std::true_type{}
    );
    
    template <class T>
    static std::false_type IsIteratableImpl(...);
    
    template <class T>
    struct IsReferenceWrapper : std::false_type
    {};
    
    template <class U>
    struct IsReferenceWrapper<std::reference_wrapper<U>> : std::true_type
    {};
    
    //FUTURE BUFFER TRAIT
    template <class T>
    struct IsBuffer : std::false_type
    {};
    
    template <class T>
    struct IsBuffer<Buffer<T>> : std::true_type
    {};
    
    template <class D, class THIS, class B>
    struct DerivedFrom
    {
        virtual ~DerivedFrom() = default;
        operator B*() { return static_cast<D*>(static_cast<THIS*>(this)); }
        operator B&() { return static_cast<D&>(static_cast<THIS&>(*this)); }
    };
};

template <typename T>
using IsIteratable = decltype(Traits::IsIteratableImpl<T>(0));

}}

#include <quantum/impl/quantum_stl_impl.h>

#endif //QUANTUM_TRAITS_H
