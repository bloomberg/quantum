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
#ifndef BLOOMBERG_QUANTUM_ITHREAD_FUTURE_H
#define BLOOMBERG_QUANTUM_ITHREAD_FUTURE_H

#include <quantum/interface/quantum_icontext_base.h>
#include <quantum/interface/quantum_icoro_future_base.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

template <class T>
class Future;

//==============================================================================================
//                                 interface IThreadFuture
//==============================================================================================
/// @interface ICoroFuture
/// @brief Exposes methods to access a non-coroutine future (i.e. accessed from a thread)
template <class T>
struct IThreadFuture : public IThreadFutureBase
{
    using ContextTag = ThreadContextTag;
    using Ptr = std::shared_ptr<IThreadFuture<T>>;
    using Impl = Future<T>;
    
    /// @brief Get the future value.
    /// @return The future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       becomes invalidated (i.e. cannot be read again)
    template <class V = T>
    NonBufferRetType<V> get();
    
    /// @brief Get a reference the future value.
    /// @return A reference to the future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to get(), this function does
    ///       not invalidate the future and as such may be read again.
    template <class V = T>
    const NonBufferRetType<V>& getRef() const;
    
    /// @brief Pull a single value from the future buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[out] isBufferClosed Indicates if this buffer is closed and no more Pull operations are allowed on it.
    /// @return The next value pulled out from the front of the buffer.
    /// @note Method available for buffered futures only. Blocks until one value is retrieved from the buffer.
    template <class V = T>
    BufferRetType<V> pull(bool& isBufferClosed);
};

template <class T>
using ThreadFuture = IThreadFuture<T>;

template <class T>
using ThreadFuturePtr = typename IThreadFuture<T>::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ITHREAD_FUTURE_H
