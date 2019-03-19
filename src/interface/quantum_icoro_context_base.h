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
#ifndef BLOOMBERG_QUANTUM_ICORO_CONTEXT_BASE_H
#define BLOOMBERG_QUANTUM_ICORO_CONTEXT_BASE_H

#include <future>
#include <chrono>
#include <quantum/interface/quantum_icontext_base.h>
#include <quantum/interface/quantum_icoro_sync.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 interface ICoroContextBase
//==============================================================================================
/// @interface ICoroContextBase.
/// @brief Exposes methods to manipulate the coroutine context, especially future wait methods.
struct ICoroContextBase : public virtual IContextBase,
                          public ICoroSync
{
    using Ptr = std::shared_ptr<ICoroContextBase>;
    
    /// @brief Waits for the future associated with this context to be ready.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @note Blocks until the future is ready or until an exception is thrown.
    virtual void wait(ICoroSync::Ptr sync) const = 0;
    
    /// @brief Waits for the future associated with this context to be ready for a maximum of 'timeMs' milliseconds.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitFor(ICoroSync::Ptr sync,
                                       std::chrono::milliseconds timeMs) const = 0;
    
    /// @brief Waits for the future in the 'num-th' continuation context to be ready.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling wait() or
    ///          waitAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @param[in] num The number indicating which future to wait on.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @note Blocks until the value is ready or an exception is thrown.
    virtual void waitAt(int num,
                        ICoroSync::Ptr sync) const = 0;
    
    /// @brief Waits for the future in the 'num-th' continuation context to be ready for a maximum of 'timeMs' milliseconds.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling wait() or
    ///          waitAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @param[in] num The number indicating which future to wait on.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitForAt(int num,
                                         ICoroSync::Ptr sync,
                                         std::chrono::milliseconds timeMs) const = 0;
    
    /// @brief Wait for all the futures in the continuation chain to be ready.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @note Blocks until all future values are ready. If any future throws, the exception is swallowed.
    virtual void waitAll(ICoroSync::Ptr sync) const = 0;
};

using ICoroContextBasePtr = ICoroContextBase::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICORO_CONTEXT_BASE_H
