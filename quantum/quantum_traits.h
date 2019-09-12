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

#include <quantum/quantum_allocator.h>
#include <iterator>
#include <type_traits>
#include <vector>
#include <tuple>
#include <memory>

namespace Bloomberg {
namespace quantum {

#define UNUSED(x) (void)(x) //remove compiler warnings

//fwd declarations
template <class T, class ALLOCATOR = std::allocator<T>>
class Buffer;
class Deprecated;
struct Void{};
template <typename T>
struct ICoroContext;
using VoidCoroContextPtr = std::shared_ptr<ICoroContext<Void>>;
using VoidContextPtr = VoidCoroContextPtr; //shorthand version

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
    
    //Partially specialize this trait to access inner types
    template <typename T>
    struct InnerType
    {
        using Type = T;
    };
    
    template <typename T>
    struct IsVoidContext : std::false_type
    {};
    
    template <typename T>
    struct IsThreadPromise : std::false_type
    {};
};

template <class T>
struct Traits::InnerType<std::vector<T>> { using Type = T; };
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
template <typename FUNC>
auto firstArgOf(FUNC&& func)->typename FunctionArguments<decltype(Callable::ref(func))>::template ArgType<0>;
template <typename FUNC>
auto resultOf(FUNC&& func)->typename Traits::InnerType<decltype(firstArgOf(func))>::Type;
template <typename FUNC>
auto resultOf2(FUNC&& func)->typename FunctionArguments<decltype(Callable::ref(func))>::RetType;
template <typename FUNC>
auto coroResult(FUNC&& func)->typename std::conditional<Traits::IsVoidContext<decltype(firstArgOf(func))>::value,
                              decltype(resultOf2(func)), decltype(resultOf(func))>::type;
template <typename FUNC>
auto ioResult(FUNC&& func)->typename std::conditional<Traits::IsThreadPromise<decltype(firstArgOf(func))>::value,
                            decltype(resultOf(func)), decltype(resultOf2(func))>::type;
template <typename FUNC>
auto mappedKeyOf(FUNC&& func)->typename std::tuple_element<0, typename Traits::InnerType<decltype(resultOf2(func))>::Type>::type;
template <typename FUNC>
auto mappedTypeOf(FUNC&& func)->typename std::tuple_element<1, typename Traits::InnerType<decltype(resultOf2(func))>::Type>::type;
template <typename FUNC>
auto reducedTypeOf(FUNC&& func)->typename std::tuple_element<1, decltype(resultOf2(func))>::type;

}}

#include <quantum/impl/quantum_stl_impl.h>

#endif //BLOOMBERG_QUANTUM_TRAITS_H
