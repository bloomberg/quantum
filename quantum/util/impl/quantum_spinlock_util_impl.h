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

#include <quantum/quantum_spinlock_traits.h>
#include <random>
#include <cassert>
#include <thread>

//Arch macros: https://sourceforge.net/p/predef/wiki/Architectures
#if defined(_MSC_VER) && \
    (defined(_M_IX86) || \
     defined(_M_IA64) || \
     defined(_M_X64))
#include <emmintrin.h>
#pragma intrinsic(_mm_pause)
#endif

namespace Bloomberg {
namespace quantum {

inline
void SpinLockUtil::pauseCPU()
{
#if defined(_MSC_VER) && \
    (defined(_M_IX86) || \
     defined(_M_IA64) || \
     defined(_M_X64))
    ::_mm_pause();
#elif (defined(__GNUC__) || \
       defined(__llvm__)) && \
      (defined(__i386__) || \
       defined(__ia64__) || \
       defined(__x86_64__))
    __asm__ __volatile__( "pause" ::: "memory" );
#else
    __asm__ __volatile__( "rep;nop" ::: "memory" );
#endif
}

inline
bool SpinLockUtil::lockWrite(std::atomic_uint32_t &flag,
                             LockTraits::Attempt attempt)
{
    size_t numBackoffs = 0;
spin:
    if (attempt == LockTraits::Attempt::Unlimited) {
        spinWaitWriter(flag);
    }
    //Try acquiring the lock
    uint32_t oldValue = set(0, 0);
    uint32_t newValue = set(0, -1);
    while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        //lock is already taken
        if (owners(oldValue) != 0)
        {
            if (attempt == LockTraits::Attempt::Once)
            {
                return false;
            }
            backoff(numBackoffs);
            //spin wait again
            goto spin;
        }
        //maintain the value for the pending writers
        newValue = set(upgrades(oldValue), -1);
        pauseCPU();
    }
    return true;
}

inline
bool SpinLockUtil::upgradeToWrite(std::atomic_uint32_t &flag,
                                  LockTraits::Attempt attempt)
{
    bool pendingUpgrade = false;
    return upgradeToWriteImpl(flag, pendingUpgrade, attempt);
}

inline
bool SpinLockUtil::upgradeToWrite(std::atomic_uint32_t &flag,
                                  bool& pendingUpgrade,
                                  LockTraits::Attempt attempt)
{
    return upgradeToWriteImpl(flag, pendingUpgrade, attempt);
}

inline
bool SpinLockUtil::upgradeToWriteImpl(std::atomic_uint32_t &flag,
                                      bool& pendingUpgrade,
                                      LockTraits::Attempt attempt)
{
    size_t numBackoffs = 0;
spin:
    if (pendingUpgrade && (attempt == LockTraits::Attempt::Unlimited))
    {
        spinWaitWriter(flag);
    }
    //Try acquiring the lock
    uint32_t oldValue = set(0, 1);
    uint32_t newValue = set(0, -1);
    while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (!pendingUpgrade)
        {
            //We are attempting to upgrade.
            if (owners(oldValue) > 1)
            {
                if (attempt == LockTraits::Attempt::Once)
                {
                    return false; //cannot upgrade immediately
                }
                //Increment pending upgrades and decrement readers
                newValue = add(oldValue, 1, -1);
            }
            else //owners(oldValue) == 1
            {
                //One reader left so upgrade to writer directly.
                //There are other pending writers so we must preserve the value
                newValue = set(upgrades(oldValue), -1);
            }
        }
        else
        {
            //Upgrade pending. To acquire lock the low value must be 0.
            if (owners(oldValue) != 0)
            {
                //lock is already taken or there are still readers
                if (attempt != LockTraits::Attempt::Unlimited)
                {
                    return false;
                }
                backoff(numBackoffs);
                //spin wait until we can upgrade again
                goto spin;
            }
            //We can upgrade. Decrement pending writers and upgrade to write
            newValue = set(upgrades(oldValue)-1, -1);
        }
        pauseCPU();
    }
    if (owners(oldValue) > 1)
    {
        //We terminated the loop from H|L -> H+1|L-1 because there were multiple readers.
        //Therefore we are still pending until all readers terminate.
        pendingUpgrade = true;
        if (attempt == LockTraits::Attempt::Reentrant)
        {
            return false; //we will get called again
        }
        backoff(numBackoffs);
        //spin wait until we can upgrade again
        goto spin;
    }
    //We terminated the loop either from H|0->H-1|-1 OR H|1->H|-1 and obtained the lock
    assert((owners(oldValue) == 0) || (owners(oldValue) == 1));
    return true;
}

inline
bool SpinLockUtil::lockRead(std::atomic_uint32_t &flag,
                            LockTraits::Attempt attempt)
{
    size_t numBackoffs = 0;
spin:
    if (attempt == LockTraits::Attempt::Unlimited)
    {
        spinWaitReader(flag);
    }
    //Try acquiring the lock
    uint32_t oldValue = set(0, 0);
    uint32_t newValue = set(0, 1);
    while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if ((upgrades(oldValue) > 0) || (owners(oldValue) == -1))
        {
            //lock is already taken or we have pending write upgrades
            if (attempt == LockTraits::Attempt::Once)
            {
                return false;
            }
            backoff(numBackoffs);
            //spin wait again
            goto spin;
        }
        newValue = add(oldValue, 0, 1);
        pauseCPU();
    }
    //We obtained the lock so exit loop
    return true;
}

inline
void SpinLockUtil::unlockRead(std::atomic_uint32_t &flag)
{
    uint32_t oldValue = set(0, 1);
    uint32_t newValue = set(0, 0);
    while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (owners(oldValue) >= 1)
        {
            //preserve high order value
            newValue = add(oldValue, 0, -1);
            pauseCPU();
        }
        else if ((owners(oldValue) == 0) || ((owners(oldValue) == -1)))
        {
            //no-op if the lock is already unlocked or if it's write-locked
            break;
        }
        else
        {
            assert(false);
        }
    }
}

inline
void SpinLockUtil::unlockWrite(std::atomic_uint32_t &flag)
{
    uint32_t oldValue = set(0, -1);
    uint32_t newValue = set(0, 0);
    while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (owners(oldValue) == -1)
        {
            //preserve high order value
            newValue = set(upgrades(oldValue), 0);
            pauseCPU();
        }
        else if (owners(oldValue) >= 0)
        {
            //no-op if the lock is already unlocked or if it's read-locked
            break;
        }
        else
        {
            assert(false);
        }
    }
}

inline
void SpinLockUtil::yieldOrSleep(size_t& num)
{
    if (num < SpinLockTraits::numYieldsBeforeSleep())
    {
        ++num;
        std::this_thread::yield();
    }
    else
    {
        assert(SpinLockTraits::sleepDuration() >= std::chrono::microseconds::zero());
        std::this_thread::sleep_for(SpinLockTraits::sleepDuration());
    }
}

inline
size_t SpinLockUtil::generateBackoff()
{
    using Distribution = std::uniform_int_distribution<size_t>;
    static thread_local std::mt19937_64 gen(std::random_device{}());
    static thread_local Distribution distribution;

    assert(SpinLockTraits::minSpins() <= SpinLockTraits::maxSpins());
    if ((SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::EqualStep) ||
        (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Random))
    {
        //Generate a number from the entire range
        return distribution(gen, Distribution::param_type
                {SpinLockTraits::minSpins(), SpinLockTraits::maxSpins()});
    }
    else
    {
        //Generate a number below min and add it to the min.
        return SpinLockTraits::minSpins() +
                distribution(gen, Distribution::param_type
                {0, SpinLockTraits::minSpins()});
    }
}

inline
void SpinLockUtil::backoff(size_t& num)
{
    if (num == 0)
    {
        num = generateBackoff();
    }
    else if (num < SpinLockTraits::maxSpins())
    {
        if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Linear)
        {
            num += SpinLockTraits::minSpins();
        }
        else if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Exponential)
        {
            num *= 2;
        }
        else if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Random)
        {
            //Generate a new value each time
            num = generateBackoff();
        }
        //Check that we don't exceed max spins
        if (num > SpinLockTraits::maxSpins())
        {
            //Reset back to initial value
            num = generateBackoff();
        }
    }
    //Spin
    for (size_t i = 0; i < num; ++i)
    {
        pauseCPU();
    }
}

inline
void SpinLockUtil::spinWaitWriter(std::atomic_uint32_t& flag)
{
    size_t numIters = 0;
    size_t numYields = 0;
    while (owners(flag.load(std::memory_order_acquire)) != 0)
    {
        if (numIters < SpinLockTraits::maxSpins())
        {
            ++numIters;
            pauseCPU();
        }
        else
        {
            //Yield or sleep the thread instead of spinning
            yieldOrSleep(numYields);
        }
    }
}

inline
void SpinLockUtil::spinWaitReader(std::atomic_uint32_t& flag)
{
    size_t numIters = 0;
    size_t numYields = 0;
    while (true)
    {
        uint32_t v = flag.load(std::memory_order_acquire);
        if ((owners(v) == -1) || (upgrades(v) > 0))
        {
            if (numIters < SpinLockTraits::maxSpins())
            {
                ++numIters;
                pauseCPU();
            }
            else
            {
                //Yield or sleep the thread instead of spinning
                yieldOrSleep(numYields);
            }
        }
        else
        {
            break;
        }
    }
}

inline
bool SpinLockUtil::isLocked(const std::atomic_uint32_t& flag)
{
    return owners(flag.load(std::memory_order_acquire)) != 0;
}

inline
bool SpinLockUtil::isWriteLocked(const std::atomic_uint32_t& flag)
{
    return owners(flag.load(std::memory_order_acquire)) == -1;
}

inline
uint16_t SpinLockUtil::numReaders(const std::atomic_uint32_t& flag)
{
    uint32_t num = flag.load(std::memory_order_acquire);
    return owners(num) == -1 ? 0 : owners(num);
}

inline
uint16_t SpinLockUtil::numPendingWriters(const std::atomic_uint32_t &flag)
{
    return upgrades(flag.load(std::memory_order_acquire));
}

inline
uint32_t SpinLockUtil::set(int16_t upgrades, int16_t owners) {
    return ((uint32_t)upgrades<<16) | (owners&mask);
}

inline
int16_t SpinLockUtil::upgrades(uint32_t n) {
    return (n>>16)&mask;
}

inline
int16_t SpinLockUtil::owners(uint32_t n) {
    return n&mask;
}

inline
uint32_t SpinLockUtil::add(uint32_t n, int16_t upgrade, int16_t owner) {
    return set(upgrades(n)+upgrade, owners(n)+owner);
}

}}
