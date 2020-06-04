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
#ifndef BLOOMBERG_QUANTUM_READ_WRITE_MUTEX_H
#define BLOOMBERG_QUANTUM_READ_WRITE_MUTEX_H

#include <quantum/quantum_read_write_spinlock.h>

namespace Bloomberg {
namespace quantum {

class ReadWriteMutex
{
public:
    /// @brief Constructor. The object is in the unlocked state.
    ReadWriteMutex() = default;

    /// @brief Copy constructor
    ReadWriteMutex(const ReadWriteMutex&) = delete;

    /// @brief Move constructor
    ReadWriteMutex(ReadWriteMutex&&) = default;

    /// @brief Copy assignment operator
    ReadWriteMutex& operator=(const ReadWriteMutex&) = delete;

    /// @brief Move assignment operator
    ReadWriteMutex& operator=(ReadWriteMutex&&) = default;

    /// @brief Lock this object as a reader (shared with other readers)
    /// @details The current context will be yielded until the lock is acquired.
    /// @note From a non-coroutine context, call the first. From a coroutine context,
    ///       call the second.
    /// @warning Wrongfully calling the first version from a coroutine context will
    ///          block all coroutines running on the same queue and result in noticeable
    ///          performance degradation.
    void lockRead();
    void lockRead(ICoroSync::Ptr sync);
    
    /// @brief Lock this object as a writer (exclusive)
    /// @details The current context will be yielded until the lock is acquired.
    /// @note From a non-coroutine context, call the first. From a coroutine context,
    ///       call the second.
    /// @warning Wrongfully calling the first version from a coroutine context will
    ///          block all coroutines running on the same queue and result in noticeable
    ///          performance degradation.
    void lockWrite();
    void lockWrite(ICoroSync::Ptr sync);
    
    /// @brief Attempts to lock this object as a reader (shared with other readers)
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockRead();
    
    /// @brief Attempts to lock this object as a writer (exclusive)
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockWrite();
    
    /// @brief Upgrade this reader to a writer
    /// @note Blocks until upgrade is performed.
    /// @note The lock must already be owned prior to invoking this function.
    void upgradeToWrite();
    void upgradeToWrite(ICoroSync::Ptr sync);
    
    /// @brief Try to upgrade this reader to a writer.
    /// @return True is succeeded.
    /// @note Does not block.
    /// @note The lock must already be owned prior to invoking this function.
    bool tryUpgradeToWrite();
    
    /// @brief Unlocks the reader lock.
    /// @warning Locking this object as a writer and incorrectly unlocking it as a reader results in undefined behavior.
    void unlockRead();
    
    /// @brief Unlocks the writer lock.
    /// @warning Locking this object as a reader and incorrectly unlocking it as a writer results in undefined behavior.
    void unlockWrite();
    
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
    int numReaders() const;
    
    /// @brief Returns the number of pending upgraded writers.
    /// @return The number of writers.
    int numPendingWriters() const;

    class Guard
    {
    public:
        /// @brief Construct this object and lock the passed-in mutex.
        /// @param[in] lock ReadWriteMutex which protects a scope during the lifetime of the Guard.
        /// @param[in] acquire Determines the type of ownership.
        /// @note Blocks the current thread until the mutex is acquired.
        Guard(ReadWriteMutex& lock,
              LockTraits::AcquireRead acquire);
        Guard(ReadWriteMutex& lock,
              LockTraits::AcquireWrite acquire);
        Guard(ICoroSync::Ptr sync,
              ReadWriteMutex& lock,
              LockTraits::AcquireRead acquire);
        Guard(ICoroSync::Ptr sync,
              ReadWriteMutex& lock,
              LockTraits::AcquireWrite acquire);
        
        /// @brief Construct this object and try to lock the passed-in mutex.
        /// @param[in] lock ReadWriteMutex which protects a scope during the lifetime of the Guard.
        /// @param[in] acquire Determines the type of ownership.
        /// @note Attempts to lock the mutex. Does not block.
        Guard(ReadWriteMutex& lock,
              LockTraits::AcquireRead acquire,
              LockTraits::TryToLock);
        Guard(ReadWriteMutex& lock,
              LockTraits::AcquireWrite acquire,
              LockTraits::TryToLock);
        
        /// @brief Construct this object and assumes the current state of the lock w/o modifying it.
        /// @param[in] lock ReadWriteMutex which protects a scope during the lifetime of the Guard.
        /// @note Does not block.
        Guard(ReadWriteMutex& lock,
              LockTraits::AdoptLock);
        
        /// @brief Destroy this object and unlocks the underlying mutex if it has ownership.
        ~Guard();
        
        /// @brief Acquire the underlying mutex as reader or writer.
        /// @note Blocks.
        void lockRead();
        void lockRead(ICoroSync::Ptr sync);
        void lockWrite();
        void lockWrite(ICoroSync::Ptr sync);
        
        /// @brief Try to acquire the underlying mutex.
        /// @return True if mutex is locked, false otherwise.
        /// @note Does not block.
        bool tryLockRead();
        bool tryLockWrite();
        
        /// @brief Upgrade this reader to a writer.
        /// @note Blocks until upgrade is performed.
        /// @note The lock must already be owned prior to invoking this function.
        void upgradeToWrite();
        void upgradeToWrite(ICoroSync::Ptr sync);
        
        /// @brief Try to upgrade this reader to a writer.
        /// @return True is succeeded.
        /// @note Does not block.
        /// @note The lock must already be owned prior to invoking this function.
        bool tryUpgradeToWrite();

        /// @brief Unlocks the underlying mutex if it has ownership.
        void unlock();
        
        /// @brief Release the associated mutex without unlocking it.
        void release();
        
        /// @brief Indicates if this object owns the underlying mutex.
        /// @return True if ownership is acquired.
        bool ownsLock() const;
        bool ownsReadLock() const;
        bool ownsWriteLock() const;
        
    private:
        ReadWriteMutex* _mutex{nullptr};
        bool            _ownsLock{false};
        bool            _isUpgraded{false};
    };

private:
    // Members
    mutable ReadWriteSpinLock   _spinlock;
    mutable TaskId              _taskId;
};

}
}

#include <quantum/impl/quantum_read_write_mutex_impl.h>

#endif //BLOOMBERG_QUANTUM_READ_WRITE_MUTEX_H
