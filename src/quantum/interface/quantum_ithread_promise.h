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
#ifndef QUANTUM_ITHREAD_PROMISE_H
#define QUANTUM_ITHREAD_PROMISE_H

#include <quantum/interface/quantum_ipromise_base.h>
#include <quantum/interface/quantum_ifuture.h>
#include <quantum/quantum_util.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                interface IThreadPromise
//==============================================================================================
/// @interface IThreadPromise
/// @brief Exposes methods to access and manipulate a non-coroutine promise (i.e. used in a thread)
/// @tparam PROMISE The derived promise (concrete type).
/// @tparam T The type of value contained in this promise.
template <template<class> class PROMISE, class T>
struct IThreadPromise : public Traits::DerivedFrom<PROMISE<T>,
                                                   IThreadPromise<PROMISE, T>,
                                                   IPromiseBase>
{
    using ptr = std::shared_ptr<IThreadPromise<PROMISE, T>>;
    using impl = PROMISE<T>;
    
    /// @brief Constructor.
    /// @param[in] derived A pointer to the concrete implementation of this class.
    /// @note The constructor parameter is only used for template deduction at compile time.
    IThreadPromise(impl* derived){ UNUSED(derived); }
    
    /// @brief Get the associated thread future.
    /// @return An interface to the associated future object sharing a common state.
    virtual typename IThreadFuture<T>::ptr getIThreadFuture() const = 0;
    
    /// @brief Set the promised value.
    /// @tparam V The type of the value. Must be implicitly deduced by the compiler and should always be == T.
    /// @param[in] value A reference to the value (l-value or r-value).
    /// @return 0 on success
    template <class V = T>
    int set(V&& value);
    
    /// @brief Push a single value into the promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[in] value Value to push at the end of the buffer.
    /// @note Method available for buffered futures only. Once the buffer is closed, no more Push
    ///       operations are allowed.
    template <class BUF = T, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    void push(V &&value);
    
    /// @brief Close a promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @note Once closed no more Pushes can be made into the buffer. The corresponding future can still Pull values until
    ///       the buffer is empty.
    /// @return 0 on success.
    template <class BUF = T, class = std::enable_if_t<Traits::IsBuffer<BUF>::value>>
    int closeBuffer();
};

template <class T> class Promise;

template <class T = int>
using ThreadPromise = IThreadPromise<Promise, T>;

}}

#endif //QUANTUM_ITHREAD_PROMISE_H
