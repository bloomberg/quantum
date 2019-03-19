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
#ifndef BLOOMBERG_QUANTUM_ITHREAD_FUTURE_BASE_H
#define BLOOMBERG_QUANTUM_ITHREAD_FUTURE_BASE_H

#include <memory>
#include <future>
#include <chrono>
#include <quantum/interface/quantum_icoro_sync.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                interface IThreadFutureBase
//==============================================================================================
/// @interface ICoroFutureBase
/// @brief Exposes methods to access a non-coroutine future (i.e. accessed from a thread)
struct IThreadFutureBase
{
    using Ptr = std::shared_ptr<IThreadFutureBase>;
    
    /// @brief Virtual destructor
    virtual ~IThreadFutureBase() = default;
    
    /// @brief Determines if this future still has a shared state with the promise object.
    /// @return True if valid, false otherwise.
    virtual bool valid() const = 0;
    
    /// @brief Waits for the future value.
    /// @note This method blocks until the future is ready or until an exception is thrown.
    virtual void wait() const = 0;
    
    /// @brief Waits for the future value up to a maximum 'timeMs' milliseconds.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitFor(std::chrono::milliseconds timeMs) const = 0;
};

using IThreadFutureBasePtr = IThreadFutureBase::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ITHREAD_FUTURE_BASE_H
