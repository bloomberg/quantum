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
#ifndef BLOOMBERG_QUANTUM_SPINLOCK_H
#define BLOOMBERG_QUANTUM_SPINLOCK_H

#include <quantum/quantum_spinlock_traits.h>
#include <atomic>
#include <mutex>

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
    /// @brief Spinlock is in unlocked state
    SpinLock() = default;
    
    /// @brief Copy constructor.
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    
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
    
    /// @brief Checks if this spinlock is locked
    /// @return True if locked
    bool isLocked() const;
    
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
        
        /// @brief Construct this object and tries to lock the passed-in spinlock.
        /// @param[in] lock Spinlock which protects a scope during the lifetime of the Guard.
        /// @param[in] tryLock Tag. Not used.
        /// @note Attempts to lock the spinlock. Does not block.
        Guard(SpinLock& lock,
              LockTraits::TryToLock tryLock);
        
        /// @brief Construct this object and assumes the current state of the lock w/o modifying it.
        /// @param[in] lock Spinlock which protects a scope during the lifetime of the Guard.
        /// @param[in] adoptLock Tag. Not used.
        Guard(SpinLock& lock,
              LockTraits::AdoptLock adoptLock);
        
        /// @brief Construct this object and assumes the lock is 'unlocked'
        /// @param[in] lock Spinlock which protects a scope during the lifetime of the Guard.
        /// @param[in] deferLock Tag. Not used.
        /// @note ownsLock() will always return false.
        Guard(SpinLock& lock,
              LockTraits::DeferLock deferLock);
        
        /// @brief Destroy this object and unlock the underlying spinlock.
        ~Guard();
        
        /// @brief Acquire the underlying spinlock.
        /// @note Blocks.
        void lock();
        
        /// @brief Try to acquire the underlying spinlock.
        /// @return True if spinlock is locked, false otherwise.
        /// @note Does not block.
        bool tryLock();
        
        /// @brief Indicates if this object owns the underlying spinlock.
        /// @return True if ownership is acquired.
        bool ownsLock() const;
        
        /// @brief Unlocks the underlying spinlock
        void unlock();
    private:
        SpinLock&	_spinlock;
        bool        _ownsLock{false};
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
    alignas(128) std::atomic_uint32_t _flag{0};
};

}}

#include <quantum/impl/quantum_spinlock_impl.h>

#endif //BLOOMBERG_QUANTUM_SPINLOCK_H
