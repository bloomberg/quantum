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
#ifndef BLOOMBERG_QUANTUM_ICORO_FUTURE_BASE_H
#define BLOOMBERG_QUANTUM_ICORO_FUTURE_BASE_H

#include <memory>
#include <future>
#include <chrono>
#include <quantum/interface/quantum_icoro_sync.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 interface ICoroFutureBase
//==============================================================================================
/// @interface ICoroFutureBase
/// @brief Exposes methods to access a coroutine-compatible future
struct ICoroFutureBase
{
    using Ptr = std::shared_ptr<ICoroFutureBase>;
    
    /// @brief Virtual destructor.
    virtual ~ICoroFutureBase() = default;
    
    /// @brief Determines if this future still has a shared state with the corresponding promise object.
    /// @return True if valid, false otherwise.
    virtual bool valid() const = 0;
    
    /// @brief Waits for the future value.
    /// @param[in] sync A pointer to a coroutine synchronization object.
    /// @note This method blocks until the future is ready or until an exception is thrown.
    virtual void wait(ICoroSync::Ptr sync) const = 0;
    
    /// @brief Waits for the future value up to a maximum 'timeMs' milliseconds.
    /// @param[in] sync A pointer to a coroutine synchronization object.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitFor(ICoroSync::Ptr sync,
                                       std::chrono::milliseconds timeMs) const = 0;
};

using ICoroFutureBasePtr = ICoroFutureBase::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICORO_FUTURE_BASE_H
