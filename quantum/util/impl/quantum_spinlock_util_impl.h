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
void SpinLockUtil::lockExclusive(std::atomic_int &flag,
                                 int exclusiveValue,
                                 int unlockedValue)
{
    while (1)
    {
        spinWait(flag, exclusiveValue);
        //Try acquiring the lock
        int oldValue = unlockedValue;
        if (!flag.compare_exchange_strong(oldValue, exclusiveValue, std::memory_order_acquire))
        {
            backoff();
        }
        else
        {
            //We obtained the lock
            reset();
            break;
        }
    }
}

inline
void SpinLockUtil::lockShared(std::atomic_int &flag,
                              int exclusiveValue,
                              int unlockedValue,
                              int sharedValue)
{
    while (1)
    {
        spinWait(flag, exclusiveValue);
        //Try acquiring the lock
        int oldValue = unlockedValue;
        int newValue = sharedValue;
        while (!flag.compare_exchange_weak(oldValue, newValue, std::memory_order_acquire))
        {
            if (oldValue == exclusiveValue)
            {
                //lock is already taken
                backoff();
                break;
            }
            newValue = oldValue + 1;
            pauseCPU();
        }
        if (oldValue < newValue)
        {
            //We obtained the lock
            reset();
            break;
        }
    }
}

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
void SpinLockUtil::reset()
{
    numYields() = 0;
    numSpins() = 0;
}

inline
void SpinLockUtil::yieldOrSleep()
{
    if (numYields() < SpinLockTraits::numYieldsBeforeSleep()) {
        ++numYields();
        std::this_thread::yield();
    }
    else {
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
void SpinLockUtil::spinWait(std::atomic_int& flag,
                            int spinValue)
{
    size_t numIters = 0;
    while (flag.load(std::memory_order_relaxed) == spinValue)
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

}}
