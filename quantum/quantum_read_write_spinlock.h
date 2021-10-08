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
#ifndef BLOOMBERG_QUANTUM_READ_WRITE_SPINLOCK_H
#define BLOOMBERG_QUANTUM_READ_WRITE_SPINLOCK_H

#include <quantum/quantum_spinlock_traits.h>
#include <atomic>
#include <mutex>

namespace Bloomberg {
namespace quantum {

class ReadWriteSpinLock
{
public:
    /// @brief Spinlock is in unlocked state
    ReadWriteSpinLock() = default;
    
    /// @brief Copy constructor.
    ReadWriteSpinLock(const ReadWriteSpinLock&) = delete;
    ReadWriteSpinLock& operator=(const ReadWriteSpinLock&) = delete;
    
    /// @brief Lock this object as a reader (shared with other readers)
    void lockRead();
    
    /// @brief Lock this object as a writer (exclusive)
    void lockWrite();
    
    /// @brief Attempts to lock this object as a reader (shared with other readers). This operation never blocks.
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockRead();
    
    /// @brief Attempts to lock this object as a writer (exclusive). This operation never blocks.
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockWrite();
    
    /// @brief Unlocks the reader lock.
    /// @warning Locking this object as a writer and incorrectly unlocking it as a reader results in undefined behavior.
    void unlockRead();
    
    /// @brief Unlocks the writer lock.
    /// @warning Locking this object as a reader and incorrectly unlocking it as a writer results in undefined behavior.
    void unlockWrite();
    
    /// @brief Atomically upgrades a Reader to a Writer.
    /// @warning Calling then while not owning the read lock results in undefined behavior.
    void upgradeToWrite();
    
    /// @brief Attempts to upgrade a reader lock to a writer lock. This operation never blocks.
    /// @param pendingUpdate Use this overload when calling this function in a loop.
    ///                      PendingUpdate must be initialized to 'false'.
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryUpgradeToWrite();
    bool tryUpgradeToWrite(bool& pendingUpdate);
    
    /// @bried Determines if this object is either read or write locked.
    /// @return True if locked, false otherwise.
    bool isLocked() const;
    
    /// @bried Determines if this object is read locked.
    /// @return True if locked, false otherwise.
    bool isReadLocked() const;
    
    /// @bried Determines if this object is write locked.
    /// @return True if locked, false otherwise.
    bool isWriteLocked() const;
    
    /// @brief Returns the number of readers holding the lock.
    /// @return The number of readers.
    /// @note: This is not an atomic operation
    int numReaders() const;
    
    /// @brief Returns the number of pending upgraded writers.
    /// @return The number of writers.
    int numPendingWriters() const;
    
    class Guard
    {
    public:
        /// @brief Construct this object and lock the passed-in spinlock as a reader.
        /// @param[in] lock ReadWriteSpinLock which protects a scope during the lifetime of the Guard.
        /// @param[in] acquire Determines the type of ownership.
        /// @note Blocks the current thread until the spinlock is acquired.
        Guard(ReadWriteSpinLock& lock,
              LockTraits::AcquireRead acquire);
        Guard(ReadWriteSpinLock& lock,
              LockTraits::AcquireWrite acquire);
        
        /// @brief Construct this object and tries to lock the passed-in spinlock as a reader.
        /// @param[in] lock ReadWriteSpinLock which protects a scope during the lifetime of the Guard.
        /// @param[in] acquire Determines the type of ownership.
        /// @param[in] tryLock Tag. Not used.
        /// @note Attempts to lock the spinlock. Does not block.
        Guard(ReadWriteSpinLock& lock,
              LockTraits::AcquireRead acquire,
              LockTraits::TryToLock tryLock);
        Guard(ReadWriteSpinLock& lock,
              LockTraits::AcquireWrite acquire,
              LockTraits::TryToLock tryLock);
        
        /// @brief Construct this object and depending on the flag may assume ownership.
        /// @param[in] lock ReadWriteSpinLock which protects a scope during the lifetime of the Guard.
        /// @param[in] adoptLock If supplied, assumes the current 'locked' state of the lock.
        /// @param[in] deferLock If supplied, assumes the lock is 'unlocked' and does not lock it.
        Guard(ReadWriteSpinLock& lock,
              LockTraits::AdoptLock adoptLock);
        Guard(ReadWriteSpinLock& lock,
              LockTraits::DeferLock deferLock);
        
        /// @brief Destroy this object and unlock the underlying spinlock.
        ~Guard();
        
        /// @brief Acquire the underlying spinlock.
        /// @note Blocks.
        void lockRead();
        void lockWrite();
        
        /// @brief Try to acquire the underlying spinlock.
        /// @return True if spinlock is locked, false otherwise.
        /// @note Does not block.
        bool tryLockRead();
        bool tryLockWrite();
        
        /// @brief Upgrade this reader to a writer atomically.
        /// @note Blocks until upgrade is performed.
        /// @note The lock must already be owned prior to invoking this function.
        void upgradeToWrite();
        
        /// @brief Try to upgrade this reader to a writer.
        /// @return True is succeeded.
        /// @note Does not block.
        /// @note The lock must already be owned prior to invoking this function.
        bool tryUpgradeToWrite();
        
        /// @brief Indicates if this object owns the underlying spinlock.
        /// @return True if ownership is acquired.
        bool ownsLock() const;
        bool ownsReadLock() const;
        bool ownsWriteLock() const;
        
        /// @brief Unlocks the underlying spinlock
        void unlock();
    private:
        ReadWriteSpinLock&	_spinlock;
        bool                _ownsLock{false};
        bool                _isUpgraded{false};
    };
    
private:
    alignas(128) std::atomic_uint32_t _count{0};
};

}
}

#include <quantum/impl/quantum_read_write_spinlock_impl.h>

#endif //BLOOMBERG_QUANTUM_READ_WRITE_SPINLOCK_H
