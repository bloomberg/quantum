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
#ifndef BLOOMBERG_QUANTUM_ICORO_PROMISE_H
#define BLOOMBERG_QUANTUM_ICORO_PROMISE_H

#include <quantum/interface/quantum_icontext_base.h>
#include <quantum/interface/quantum_ipromise_base.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                interface ICoroPromise
//==============================================================================================
/// @interface ICoroPromise
/// @brief Exposes methods to access and manipulate a coroutine-compatible promise.
/// @tparam PROMISE The derived promise (concrete type).
/// @tparam T The type of value contained in this promise.
template <template<class> class PROMISE, class T>
struct ICoroPromise : public Traits::DerivedFrom<PROMISE<T>,
                                                 ICoroPromise<PROMISE, T>,
                                                 IPromiseBase>
{
    using ContextTag = CoroContextTag;
    using Ptr = std::shared_ptr<ICoroPromise<PROMISE, T>>;
    using Impl = PROMISE<T>;
    
    /// @brief Constructor.
    /// @param[in] derived A pointer to the concrete implementation of this class.
    /// @note The constructor parameter is only used for template deduction at compile time.
    ICoroPromise(Impl* derived){ UNUSED(derived); }
    
    /// @brief Get the associated coroutine future.
    /// @return An interface to the associated future object sharing a common state.
    virtual CoroFuturePtr<T> getICoroFuture() const = 0;
    
    /// @brief Set the promised value.
    /// @tparam V The type of the value. Must be implicitly deduced by the compiler and should always be == T.
    /// @param[in] sync A pointer to a coroutine synchronization context.
    /// @param[in] value A reference to the value (l-value or r-value).
    /// @return 0 on success
    template <class V, class = NonBufferType<T,V>>
    int set(ICoroSync::Ptr sync, V&& value);
    
    /// @brief Push a single value into the promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[in] sync A pointer to a coroutine synchronization context.
    /// @param[in] value Value to push at the end of the buffer.
    /// @note Method available for buffered futures only. Once the buffer is closed, no more Push
    ///       operations are allowed.
    template <class V, class = BufferType<T,V>>
    void push(ICoroSync::Ptr sync, V&& value);
    
    /// @brief Close a promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @note Once closed no more Pushes can be made into the buffer. The corresponding future can still Pull values until
    ///       the buffer is empty.
    /// @return 0 on success.
    template <class V = T, class = BufferRetType<V>>
    int closeBuffer();
};

template <class T> class Promise;

template <class T>
using CoroPromise = ICoroPromise<Promise,T>;

template <class T>
using CoroPromisePtr = typename ICoroPromise<Promise,T>::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICORO_PROMISE_H
