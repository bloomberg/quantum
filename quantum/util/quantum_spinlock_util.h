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
#ifndef QUANTUM_QUANTUM_SPINLOCK_UTIL_H
#define QUANTUM_QUANTUM_SPINLOCK_UTIL_H

namespace Bloomberg {
namespace quantum {

//Adapted from https://geidav.wordpress.com/tag/test-and-test-and-set/
struct SpinLockUtil {
    static void lockExclusive(std::atomic_int &flag,
                              int exclusiveValue,
                              int unlockedValue);
    static void lockShared(std::atomic_int &flag,
                           int exclusiveValue,
                           int unlockedValue,
                           int sharedValue);
    static void pauseCPU();
private:
    static void reset();
    static void yieldOrSleep();
    static void backoff();
    static void spinWait(std::atomic_int &flag,
                         int spinValue);
    static size_t& numYields();
    static size_t& numSpins();
};

}}

#include <quantum/util/impl/quantum_spinlock_util_impl.h>

#endif //QUANTUM_QUANTUM_SPINLOCK_UTIL_H
