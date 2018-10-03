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
#ifndef QUANTUM_SPINLOCK_H
#define QUANTUM_SPINLOCK_H

#include <atomic>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class SpinLock
//==============================================================================================
/// @class SpinLock
/// @brief Coroutine-compatible spinlock. Used internally for mutexes since threads running
///        coroutines cannot block.
class SpinLock
{
public:
    /// @brief Constructor. The object is in the unlocked state.
    SpinLock();
    
    /// @brief Copy constructor.
    SpinLock(const SpinLock&) = delete;
    
    /// @brief Move constructor.
    SpinLock(SpinLock&&) = default;
    
    /// @brief Copy assignment operator.
    SpinLock& operator=(const SpinLock&) = delete;
    
    /// @brief Move assignment operator.
    SpinLock& operator=(SpinLock&&) = default;
    
    /// @brief Locks this object.
    /// @note Blocks the current thread until the lock is acquired. Blocking is achieved
    ///       via a busy loop and the thread is not re-scheduled by the operating system.
    void lock();
    
    /// @brief Attempt to acquire the lock.
    /// @return True if successful, false otherwise.
    /// @note This function never blocks.
    bool tryLock();
    
    /// @brief Unlocks the current object.
    /// @note Never blocks.
    void unlock();
    
    //==============================================================================================
    //                                      class SpinLock::Guard
    //==============================================================================================
    /// @class SpinLock::Guard
    /// @brief RAII-style mechanism for SpinLock ownership.
    ///        Acquires a SpinLock on construction and releases it inside the destructor.
    class Guard
    {
    public:
        /// @brief Construct this object and lock the passed-in spinlock.
        /// @param[in] lock Spinlock which protects a scope during the lifetime of the Guard.
        /// @note Blocks the current thread until the spinlock is acquired.
        explicit Guard(SpinLock& lock);
        
        /// @brief Destroy this object and unlock the underlying spinlock.
        ~Guard();
    private:
        SpinLock&	_spinlock;
    };
    
    //==============================================================================================
    //                                 class SpinLock::ReverseGuard
    //==============================================================================================
    /// @class SpinLock::ReverseGuard
    /// @brief Opposite form of RAII-style mechanism for SpinLock ownership.
    ///        Releases a SpinLock on construction and acquires it inside the destructor.
    class ReverseGuard
    {
    public:
        /// @brief Release the passed-in spinlock.
        /// @param[in] lock Reference to the spinlock to release.
        explicit ReverseGuard(SpinLock& lock);
        
        /// @brief Acquire the underlying spinlock.
        /// @note This will block the current thread until the spinlock is acquired.
        ~ReverseGuard();
    private:
        SpinLock&	_spinlock;
    };
    
private:
    std::atomic_flag 	_flag;
};

}}

#include <quantum/impl/quantum_spinlock_impl.h>

#endif //QUANTUM_SPINLOCK_H
