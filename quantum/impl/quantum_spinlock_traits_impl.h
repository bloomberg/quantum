/*
** Copyright 2020 Bloomberg Finance L.P.
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
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################

namespace Bloomberg {
namespace quantum {

inline size_t&
SpinLockTraits::minSpins() {
    static size_t minSpins = QUANTUM_SPINLOCK_MIN_SPINS;
    return minSpins;
}

inline size_t&
SpinLockTraits::maxSpins() {
    static size_t maxSpins = QUANTUM_SPINLOCK_MAX_SPINS;
    return maxSpins;
}

inline size_t&
SpinLockTraits::numYieldsBeforeSleep() {
    static size_t numYieldsBeforeSleep = QUANTUM_SPINLOCK_NUM_YIELDS_BEFORE_SLEEP;
    return numYieldsBeforeSleep;
}

inline std::chrono::microseconds&
SpinLockTraits::sleepDuration() {
    static std::chrono::microseconds sleepDuration{QUANTUM_SPINLOCK_SLEEP_DURATION_US};
    return sleepDuration;
}

inline SpinLockTraits::BackoffPolicy&
SpinLockTraits::backoffPolicy() {
    static BackoffPolicy backoffPolicy = static_cast<BackoffPolicy>(QUANTUM_SPINLOCK_BACKOFF_POLICY);
    return backoffPolicy;
}

}}

