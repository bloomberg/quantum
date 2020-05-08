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
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################

#include <quantum/util/quantum_spinlock_util.h>

namespace Bloomberg {
namespace quantum {

inline
void ReadWriteSpinLock::lockRead()
{
    SpinLockUtil::lockShared(_count, -1, 0, 1);
}

inline
void ReadWriteSpinLock::lockWrite()
{
    SpinLockUtil::lockExclusive(_count, -1, 0);
}

inline
bool ReadWriteSpinLock::tryLockRead()
{
    int oldValue = 0;
    int newValue = 1;
    while (!_count.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (oldValue == -1)
        {
            return false; //write-locked
        }
        else
        {
            newValue = oldValue + 1;
        }
        SpinLockUtil::pauseCPU();
    }
    return true;
}

inline
bool ReadWriteSpinLock::tryLockWrite()
{
    int i = 0;
    return _count.compare_exchange_strong(i, -1, std::memory_order_acq_rel);
}

inline
bool ReadWriteSpinLock::unlockRead()
{
    int oldValue = 1;
    int newValue = 0;
    while (!_count.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (oldValue <= 0)
        {
            return false; //unlocked or write-locked
        }
        else
        {
            newValue = oldValue - 1;
        }
        SpinLockUtil::pauseCPU();
    }
    return true;
}

inline
bool ReadWriteSpinLock::unlockWrite()
{
    int i = -1;
    return _count.compare_exchange_strong(i, 0, std::memory_order_acq_rel);
}

inline
bool ReadWriteSpinLock::isLocked()
{
    return _count.load(std::memory_order_acquire) != 0;
}

inline
bool ReadWriteSpinLock::isReadLocked()
{
    return _count.load(std::memory_order_acquire) > 0;
}

inline
bool ReadWriteSpinLock::isWriteLocked()
{
    return _count.load(std::memory_order_acquire) == -1;
}

inline
int ReadWriteSpinLock::numReaders() const
{
    int num = _count.load(std::memory_order_acquire);
    return num == -1 ? 0 : num;
}

inline
ReadWriteSpinLock::ReadGuard::ReadGuard(ReadWriteSpinLock& lock) :
    _spinlock(lock),
    _ownsLock(true)
{
    _spinlock.lockRead();
}

inline
ReadWriteSpinLock::ReadGuard::ReadGuard(ReadWriteSpinLock& lock, ReadWriteSpinLock::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLockRead())
{
}

inline
ReadWriteSpinLock::ReadGuard::~ReadGuard()
{
    if (_ownsLock) {
        _spinlock.unlockRead();
    }
}

inline
void ReadWriteSpinLock::ReadGuard::lock()
{
    if (!_ownsLock) {
        _spinlock.lockRead();
        _ownsLock = true;
    }
}

inline
bool ReadWriteSpinLock::ReadGuard::tryLock()
{
    if (!_ownsLock) {
        _ownsLock = _spinlock.tryLockRead();
    }
    return _ownsLock;
}

inline
bool ReadWriteSpinLock::ReadGuard::ownsLock() const
{
    return _ownsLock;
}

inline
void ReadWriteSpinLock::ReadGuard::unlock()
{
    if (_ownsLock) {
        _spinlock.unlockRead();
    }
}

inline
ReadWriteSpinLock::WriteGuard::WriteGuard(ReadWriteSpinLock& lock) :
    _spinlock(lock),
    _ownsLock(true)
{
    _spinlock.lockWrite();
}

inline
ReadWriteSpinLock::WriteGuard::WriteGuard(ReadWriteSpinLock& lock, ReadWriteSpinLock::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLockWrite())
{
}

inline
ReadWriteSpinLock::WriteGuard::~WriteGuard()
{
    if (_ownsLock) {
        _spinlock.unlockWrite();
    }
}

inline
void ReadWriteSpinLock::WriteGuard::lock()
{
    if (!_ownsLock) {
        _spinlock.lockWrite();
        _ownsLock = true;
    }
}

inline
bool ReadWriteSpinLock::WriteGuard::tryLock()
{
    if (!_ownsLock) {
        _ownsLock = _spinlock.tryLockWrite();
    }
    return _ownsLock;
}

inline
bool ReadWriteSpinLock::WriteGuard::ownsLock() const
{
    return _ownsLock;
}

inline
void ReadWriteSpinLock::WriteGuard::unlock()
{
    if (_ownsLock) {
        _spinlock.unlockWrite();
    }
}

    
}
}
