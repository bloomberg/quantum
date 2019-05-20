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

#include <atomic>
#include <mutex>

namespace Bloomberg {
namespace quantum {

class ReadWriteSpinlock
{
public:
    using TryToLock = std::try_to_lock_t;
    
    /// @brief Constructor. The object is in the unlocked state.
    ReadWriteSpinlock() = default;
    
    /// @brief Copy constructor.
    ReadWriteSpinlock(const ReadWriteSpinlock&) = delete;
    
    /// @brief Move constructor.
    ReadWriteSpinlock(ReadWriteSpinlock&&) = default;
    
    /// @brief Copy assignment operator.
    ReadWriteSpinlock& operator=(const ReadWriteSpinlock&) = delete;
    
    /// @brief Move assignment operator.
    ReadWriteSpinlock& operator=(ReadWriteSpinlock&&) = default;
    
    /// @brief Lock this object as a reader (shared with other readers)
    void lockRead();
    
    /// @brief Lock this object as a writer (exclusive)
    void lockWrite();
    
    /// @brief Attempts to lock this object as a reader (shared with other readers)
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockRead();
    
    /// @brief Attempts to lock this object as a writer (exclusive)
    /// @return True if the lock operation succeeded, false otherwise.
    bool tryLockWrite();
    
    /// @brief Unlocks the reader lock.
    /// @warning Locking this object as a writer and incorrectly unlocking it as a reader results in undefined behavior.
    void unlockRead();
    
    /// @brief Unlocks the writer lock.
    /// @warning Locking this object as a reader and incorrectly unlocking it as a writer results in undefined behavior.
    void unlockWrite();
    
    /// @bried Determines if this object is either read or write locked.
    /// @return True if locked, false otherwise.
    bool isLocked();
    
    /// @bried Determines if this object is read locked.
    /// @return True if locked, false otherwise.
    bool isReadLocked();
    
    /// @bried Determines if this object is write locked.
    /// @return True if locked, false otherwise.
    bool isWriteLocked();
    
    /// @brief Returns the number of readers holding the lock.
    /// @return The number of readers.
    int numReaders() const;
    
    class ReadGuard
    {
    public:
        /// @brief Construct this object and lock the passed-in spinlock as a reader.
        /// @param[in] lock ReadWriteSpinlock which protects a scope during the lifetime of the Guard.
        /// @note Blocks the current thread until the spinlock is acquired.
        explicit ReadGuard(ReadWriteSpinlock& lock);
        
        /// @brief Construct this object and tries to lock the passed-in spinlock as a reader.
        /// @param[in] lock ReadWriteSpinlock which protects a scope during the lifetime of the Guard.
        /// @note Attempts to lock the spinlock. Does not block.
        ReadGuard(ReadWriteSpinlock& lock, ReadWriteSpinlock::TryToLock);
        
        /// @brief Destroy this object and unlock the underlying spinlock.
        ~ReadGuard();
        
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
    private:
        ReadWriteSpinlock&	_spinlock;
        bool                _ownsLock;
    };
    
    class WriteGuard
    {
    public:
        /// @brief Construct this object and lock the passed-in spinlock as a writer.
        /// @param[in] lock ReadWriteSpinlock which protects a scope during the lifetime of the Guard.
        /// @note Blocks the current thread until the spinlock is acquired.
        explicit WriteGuard(ReadWriteSpinlock& lock);
        
        /// @brief Construct this object and tries to lock the passed-in spinlock as a writer.
        /// @param[in] lock ReadWriteSpinlock which protects a scope during the lifetime of the Guard.
        /// @note Attempts to lock the spinlock. Does not block.
        WriteGuard(ReadWriteSpinlock& lock, ReadWriteSpinlock::TryToLock);
        
        /// @brief Destroy this object and unlock the underlying spinlock.
        ~WriteGuard();
        
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
    private:
        ReadWriteSpinlock&	_spinlock;
        bool                _ownsLock;
    };
    
private:
    std::atomic_int     _count{0};
};

}
}

#include <quantum/impl/quantum_read_write_spinlock_impl.h>

#endif //BLOOMBERG_QUANTUM_READ_WRITE_SPINLOCK_H
