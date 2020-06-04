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
                             bool tryOnce)
{
    while (true)
    {
spin:
        if (!tryOnce) {
            spinWaitWriter(flag);
        }
        //Try acquiring the lock
        uint32_t oldValue = set(0, 0);
        uint32_t newValue = set(0, -1);
        while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acquire))
        {
            //lock is already taken
            if (owners(oldValue) != 0)
            {
                if (tryOnce)
                {
                    return false;
                }
                backoff();
                //spin wait again
                goto spin;
            }
            //maintain the value for the pending writers
            newValue = set(upgrades(oldValue), -1);
            pauseCPU();
        }
        reset();
        return true;
    }
}

inline
bool SpinLockUtil::upgradeToWrite(std::atomic_uint32_t &flag,
                                  bool tryOnce)
{
    bool pendingUpgrade = false;
    while (true) {
spin:
        if (pendingUpgrade && !tryOnce)
        {
            spinWaitUpgradedReader(flag);
        }
        //Try acquiring the lock
        uint32_t oldValue = set(0, 1);
        uint32_t newValue = set(0, -1);
        while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acquire))
        {
            if (!pendingUpgrade)
            {
                //We are attempting to upgrade.
                if (owners(oldValue) > 1)
                {
                    if (tryOnce)
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
                    if (tryOnce)
                    {
                        return false;
                    }
                    backoff();
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
            goto spin;
        }
        //We terminated the loop either from H|0->H-1|-1 OR H|1->H|-1 and obtained the lock
        assert((owners(oldValue) == 0) || (owners(oldValue) == 1));
        reset();
        return true;
    }
}

inline
bool SpinLockUtil::lockRead(std::atomic_uint32_t &flag,
                            bool tryOnce)
{
    while (true)
    {
spin:
        if (!tryOnce)
        {
            spinWaitReader(flag);
        }
        //Try acquiring the lock
        uint32_t oldValue = set(0, 0);
        uint32_t newValue = set(0, 1);
        while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acquire))
        {
            if ((upgrades(oldValue) > 0) || (owners(oldValue) == -1))
            {
                //lock is already taken or we have pending write upgrades
                if (tryOnce)
                {
                    return false;
                }
                backoff();
                //spin wait again
                goto spin;
            }
            newValue = add(oldValue, 0, 1);
            pauseCPU();
        }
        //We obtained the lock so exit loop
        reset();
        return true;
    }
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
        else
        {
            break;
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
            newValue = set(SpinLockUtil::upgrades(oldValue), 0);
            pauseCPU();
        }
        else
        {
            break;
        }
    }
}

inline
void SpinLockUtil::reset()
{
    numYields() = 0;
    numSpins() = 0;
}

inline
void SpinLockUtil::yieldOrSleep()
{
    if (numYields() < SpinLockTraits::numYieldsBeforeSleep())
    {
        ++numYields();
        std::this_thread::yield();
    }
    else
    {
        assert(SpinLockTraits::sleepDuration() >= std::chrono::microseconds::zero());
        std::this_thread::sleep_for(SpinLockTraits::sleepDuration());
    }
}

inline
void SpinLockUtil::backoff()
{
    using Distribution = std::uniform_int_distribution<size_t>;
    static thread_local std::mt19937_64 gen(std::random_device{}());
    static thread_local Distribution distribution;
    if (numSpins() == 0)
    {
        //Initialize for the first time
        assert(SpinLockTraits::minSpins() <= SpinLockTraits::maxSpins());
        if ((SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::EqualStep) ||
            (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Random))
        {
            //Generate a number from the entire range
            numSpins() = distribution(gen, Distribution::param_type
                    {SpinLockTraits::minSpins(), SpinLockTraits::maxSpins()});
        }
        else
        {
            //Generate a number below min and add it to the min.
            numSpins() = SpinLockTraits::minSpins() +
                    distribution(gen, Distribution::param_type
                    {0, SpinLockTraits::minSpins()});
        }
    }
    else if (numSpins() < SpinLockTraits::maxSpins())
    {
        if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Linear)
        {
            numSpins() += numSpins();
        }
        else if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Exponential)
        {
            numSpins() *= 2;
        }
        else if (SpinLockTraits::backoffPolicy() == SpinLockTraits::BackoffPolicy::Random)
        {
            //Generate a new value each time
            numSpins() = distribution(gen, Distribution::param_type
                    {SpinLockTraits::minSpins(), SpinLockTraits::maxSpins()});
        }
        //Check that we don't exceed max spins
        if (numSpins() > SpinLockTraits::maxSpins())
        {
            numSpins() = SpinLockTraits::maxSpins();
        }
    }
    //Spin
    for (size_t i = 0; i < numSpins(); ++i)
    {
        pauseCPU();
    }
}

inline
void SpinLockUtil::spinWaitWriter(std::atomic_uint32_t& flag)
{
    size_t numIters = 0;
    while (owners(flag.load(std::memory_order_relaxed) != 0))
    {
        if (numIters < SpinLockTraits::maxSpins())
        {
            ++numIters;
            pauseCPU();
        }
        else
        {
            //Yield or sleep the thread instead of spinning
            yieldOrSleep();
        }
    }
}

inline
void SpinLockUtil::spinWaitUpgradedReader(std::atomic_uint32_t& flag)
{
    size_t numIters = 0;
    while (true)
    {
        uint32_t v = flag.load(std::memory_order_relaxed);
        if (owners(v) != 0)
        {
            if (numIters < SpinLockTraits::maxSpins())
            {
                ++numIters;
                pauseCPU();
            }
            else
            {
                //Yield or sleep the thread instead of spinning
                yieldOrSleep();
            }
        }
        else
        {
            break;
        }
    }
}

inline
void SpinLockUtil::spinWaitReader(std::atomic_uint32_t& flag)
{
    size_t numIters = 0;
    while (true)
    {
        uint32_t v = flag.load(std::memory_order_relaxed);
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
                yieldOrSleep();
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
size_t& SpinLockUtil::numYields()
{
    static thread_local size_t _numYields = 0;
    return _numYields;
}

inline
size_t& SpinLockUtil::numSpins()
{
    static thread_local size_t _numSpins = 0;
    return _numSpins;
}

inline constexpr
uint32_t SpinLockUtil::set(int16_t upgrades, int16_t owners) {
    return ((uint32_t)upgrades<<16) | (owners&mask);
}

inline constexpr
int16_t SpinLockUtil::upgrades(uint32_t n) {
    return (n>>16)&mask;
}

inline constexpr
int16_t SpinLockUtil::owners(uint32_t n) {
    return n&mask;
}

inline constexpr
uint32_t SpinLockUtil::add(uint32_t n, int16_t upgrade, int16_t owner) {
    return set(upgrades(n)+upgrade, owners(n)+owner);
}

}}
