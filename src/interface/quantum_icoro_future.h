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
#ifndef BLOOMBERG_QUANTUM_ICORO_FUTURE_H
#define BLOOMBERG_QUANTUM_ICORO_FUTURE_H

#include <quantum/interface/quantum_icontext_base.h>
#include <quantum/interface/quantum_icoro_future_base.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

template <class T>
class Future;

//==============================================================================================
//                                interface ICoroFuture
//==============================================================================================
/// @interface ICoroFuture
/// @brief Exposes methods to access a coroutine-compatible future
template <class T>
struct ICoroFuture : public ICoroFutureBase
{
    using ContextTag = CoroContextTag;
    using Ptr = std::shared_ptr<ICoroFuture<T>>;
    using Impl = Future<T>;
    
    /// @brief Get the future value.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return The future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       becomes invalidated (i.e. cannot be read again)
    template <class V = T>
    NonBufferRetType<V> get(ICoroSync::Ptr sync);
    
    /// @brief Get a reference the future value.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return A reference to the future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to get(), this function does
    ///       not invalidate the future and as such may be read again.
    template <class V = T>
    const NonBufferRetType<V>& getRef(ICoroSync::Ptr sync) const;
    
    /// @brief Pull a single value from the future buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @param[out] isBufferClosed Indicates if this buffer is closed and no more Pull operations are allowed on it.
    /// @return The next value pulled out from the front of the buffer.
    /// @note Method available for buffered futures only. Blocks until one value is retrieved from the buffer.
    template <class V = T>
    BufferRetType<V> pull(ICoroSync::Ptr sync, bool& isBufferClosed);
};

template <class T>
using CoroFuture = ICoroFuture<T>;

template <class T>
using CoroFuturePtr = typename ICoroFuture<T>::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICORO_FUTURE_H
