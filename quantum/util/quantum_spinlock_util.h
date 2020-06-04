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
    static bool lockWrite(std::atomic_uint32_t& flag, bool tryOnce = false);
    static bool lockRead(std::atomic_uint32_t& flag, bool tryOnce = false);
    static bool upgradeToWrite(std::atomic_uint32_t& flag, bool tryOnce = false);
    static void unlockRead(std::atomic_uint32_t& flag);
    static void unlockWrite(std::atomic_uint32_t& flag);
    static bool isLocked(const std::atomic_uint32_t& flag);
    static bool isWriteLocked(const std::atomic_uint32_t& flag);
    static uint16_t numReaders(const std::atomic_uint32_t& flag);
    static uint16_t numPendingWriters(const std::atomic_uint32_t& flag);
private:
    static void reset();
    static void yieldOrSleep();
    static void backoff();
    static void spinWaitWriter(std::atomic_uint32_t& flag);
    static void spinWaitReader(std::atomic_uint32_t& flag);
    static void spinWaitUpgradedReader(std::atomic_uint32_t& flag);
    static size_t& numYields();
    static size_t& numSpins();
    static void pauseCPU();
    //Bit manipulations
    static constexpr uint32_t set(int16_t upgrades, int16_t owners);
    static constexpr uint32_t add(uint32_t n, int16_t upgrade, int16_t owner);
    //The high 16 bits
    static constexpr int16_t upgrades(uint32_t n);
    //The low 16 bits. -1 indicates a single exclusive writer
    static constexpr int16_t owners(uint32_t n);
    static constexpr uint32_t mask = 0x0000FFFF;
};

}}

#include <quantum/util/impl/quantum_spinlock_util_impl.h>

#endif //QUANTUM_QUANTUM_SPINLOCK_UTIL_H
