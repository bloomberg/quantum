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
#ifndef BLOOMBERG_QUANTUM_ICORO_SYNC_H
#define BLOOMBERG_QUANTUM_ICORO_SYNC_H

#include <memory>
#include <atomic>
#include <chrono>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  interface ICoroSync
//==============================================================================================
/// @interface ICoroSync
/// @brief Provides an interface to facilitate 'implicit' coroutine yielding within other primitives such
///        as mutexes and condition variables or to allow 'explicit' cooperative yielding by the user.
/// @note This class is used internally and as such should not be accessed directly.
struct ICoroSync
{
    using Ptr = std::shared_ptr<ICoroSync>;
    
    /// @brief Default virtual destructor.
    virtual ~ICoroSync() = default;
    
    /// @brief Sets the underlying boost::coroutine object so that it can be yielded on.
    /// @param[in] yield Reference to the boost::coroutine object.
    virtual void setYieldHandle(Traits::Yield& yield) = 0;
    
    /// @brief Retrieve the underlying boost::coroutine object.
    /// @return The associated boost::coroutine object held by this class.
    /// @throws If the underlying boost::coroutine is not set.
    virtual Traits::Yield& getYieldHandle() = 0;
    
    /// @brief Explicitly yields this coroutine context.
    virtual void yield() = 0;
    
    /// @brief Accessor to the underlying synchronization variable.
    /// @return An atomic integer used to synchronize with other primitive types.
    virtual std::atomic_int& signal() = 0;
    
    /// @brief Sleeps the coroutine associated with this context for *at least* 'timeMs' milliseconds or
    ///        'timeUs' microseconds depending on the overload chosen.
    /// @param[in] timeMs/timeUs Time to sleep.
    /// @note This method yields the coroutine until the timer has expired. Depending on the
    ///       coroutine load on the particular running thread, this method may sleep longer.
    virtual void sleep(const std::chrono::milliseconds& timeMs) = 0;
    virtual void sleep(const std::chrono::microseconds& timeUs) = 0;
};

using ICoroSyncPtr = ICoroSync::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICORO_SYNC_H
