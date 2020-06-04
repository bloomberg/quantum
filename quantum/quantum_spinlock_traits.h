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
#ifndef QUANTUM_QUANTUM_SPINLOCK_TRAITS_H
#define QUANTUM_QUANTUM_SPINLOCK_TRAITS_H

#include <quantum/quantum_macros.h>
#include <mutex>

namespace Bloomberg {
namespace quantum {

#ifndef QUANTUM_SPINLOCK_MIN_SPINS
#define QUANTUM_SPINLOCK_MIN_SPINS 100;
#endif

#ifndef QUANTUM_SPINLOCK_MAX_SPINS
#define QUANTUM_SPINLOCK_MAX_SPINS 5000
#endif

#ifndef QUANTUM_SPINLOCK_SLEEP_DURATION_US
#define QUANTUM_SPINLOCK_SLEEP_DURATION_US 200
#endif

#ifndef QUANTUM_SPINLOCK_NUM_YIELDS_BEFORE_SLEEP
#define QUANTUM_SPINLOCK_NUM_YIELDS_BEFORE_SLEEP 3
#endif

#ifndef QUANTUM_SPINLOCK_BACKOFF_POLICY
#define QUANTUM_SPINLOCK_BACKOFF_POLICY QUANTUM_BACKOFF_EXPONENTIAL
#endif

//==============================================================================
//                                SpinLockTraits
//==============================================================================
struct SpinLockTraits {
    enum class BackoffPolicy : int {
        Linear = QUANTUM_BACKOFF_LINEAR,           ///< Linear backoff
        Exponential = QUANTUM_BACKOFF_EXPONENTIAL, ///< Exponential backoff (doubles every time)
        EqualStep = QUANTUM_BACKOFF_EQUALSTEP,     ///< Identical backoff amount
        Random = QUANTUM_BACKOFF_RANDOM            ///< Random backoff amount
    };
    
    /// @brief Initial number of spins for the backoff
    static size_t &minSpins();
    
    /// @brief The maximum number of spins to wait before yielding,
    ///        as well as the backoff limit.
    static size_t &maxSpins();
    
    /// @brief The number of yields before sleeping the thread
    static size_t &numYieldsBeforeSleep();
    
    /// @brief The sleep duration (after there are no more yields)
    static std::chrono::microseconds &sleepDuration();
    
    /// @brief Backoff policy while spinning
    static BackoffPolicy &backoffPolicy();
};

//==============================================================================
//                                  LockTraits
//==============================================================================
struct LockTraits
{
    using TryToLock = std::try_to_lock_t;
    using AdoptLock = std::adopt_lock_t;
    struct AcquireRead{};
    struct AcquireWrite{};
};

namespace lock {
    static constexpr const LockTraits::TryToLock tryToLock;
    static constexpr const LockTraits::AdoptLock adoptLock;
    static constexpr const LockTraits::AcquireRead acquireRead;
    static constexpr const LockTraits::AcquireWrite acquireWrite;
}

}}

#include <quantum/impl/quantum_spinlock_traits_impl.h>

#endif //QUANTUM_QUANTUM_SPINLOCK_TRAITS_H
