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
/*
The atomic 32-bit counter is divided into two halves, the high 16 bits representing the number
of pending writer upgrades and the low 16 bit representing the lock state as following:
   0 (unlocked)
   -1 (write-locked)
   >0 (read-locked and the number represents the number of readers)
   
When a reader is upgraded to a writer, the number of readers is decremented by 1 and the number
of pending writers is incremented by 1 if the upgrade can't happen immediately. If however
there's only a single reader, the upgrade occurs immediately and the number of readers (i.e. 1)
gets converted directly to a writer (i.e. -1).

State transitions:
H|L represent the high (pending upgrades) and low (reader/writer owners) portions of
the 32-bit counter.

Reader Lock
1) 0|L -> 0|L+1 where L>=0

Reader Unlock
1) H|L -> H|L-1

Reader Upgrade
1) H|1 -> H|-1 (direct upgrade)
2) H|L -> H+1|L-1 then goto Writer (2)

Reader blocks when
1) H|-1
2) H|L -> where H>0

Writer Lock
1) H|0 -> H|-1 where H>=0 (regular writer)
2) H|0 -> H-1|-1 where H>0 (upgraded writer)

Writer Unlock
1) H|-1 -> H|0

Writer blocks when
1) H|-1
2) H|L where L>0
*/

#include <quantum/util/quantum_spinlock_util.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                ReadWriteSpinLock
//==============================================================================================
inline
void ReadWriteSpinLock::lockRead()
{
    SpinLockUtil::lockRead(_count);
}

inline
void ReadWriteSpinLock::lockWrite()
{
    SpinLockUtil::lockWrite(_count);
}

inline
bool ReadWriteSpinLock::tryLockRead()
{
    return SpinLockUtil::lockRead(_count, true);
}

inline
bool ReadWriteSpinLock::tryLockWrite()
{
    return SpinLockUtil::lockWrite(_count, true);
}

inline
void ReadWriteSpinLock::unlockRead()
{
    SpinLockUtil::unlockRead(_count);
}

inline
void ReadWriteSpinLock::unlockWrite()
{
    SpinLockUtil::unlockWrite(_count);
}

inline
void ReadWriteSpinLock::upgradeToWrite()
{
    SpinLockUtil::upgradeToWrite(_count);
}

inline
bool ReadWriteSpinLock::tryUpgradeToWrite()
{
    return SpinLockUtil::upgradeToWrite(_count, true);
}

inline
bool ReadWriteSpinLock::isLocked() const
{
    return SpinLockUtil::isLocked(_count);
}

inline
bool ReadWriteSpinLock::isReadLocked() const
{
    return isLocked() && !isWriteLocked();
}

inline
bool ReadWriteSpinLock::isWriteLocked() const
{
    return SpinLockUtil::isWriteLocked(_count);
}

inline
int ReadWriteSpinLock::numReaders() const
{
    return SpinLockUtil::numReaders(_count);
}

inline
int ReadWriteSpinLock::numPendingWriters() const
{
    return SpinLockUtil::numPendingWriters(_count);
}

//==============================================================================================
//                                ReadWriteSpinLock::Guard
//==============================================================================================
inline
ReadWriteSpinLock::Guard::Guard(ReadWriteSpinLock& lock,
                                LockTraits::AcquireRead) :
    _spinlock(lock),
    _ownsLock(true),
    _isUpgraded(false)
{
    _spinlock.lockRead();
}

inline
ReadWriteSpinLock::Guard::Guard(ReadWriteSpinLock& lock,
                                LockTraits::AcquireWrite) :
    _spinlock(lock),
    _ownsLock(true),
    _isUpgraded(true)
{
    _spinlock.lockWrite();
}

inline
ReadWriteSpinLock::Guard::Guard(ReadWriteSpinLock& lock,
                                LockTraits::AcquireRead,
                                LockTraits::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLockRead()),
    _isUpgraded(false)
{
}

inline
ReadWriteSpinLock::Guard::Guard(ReadWriteSpinLock& lock,
                                LockTraits::AcquireWrite,
                                LockTraits::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLockWrite()),
    _isUpgraded(_ownsLock)
{
}

inline
ReadWriteSpinLock::Guard::Guard(ReadWriteSpinLock& lock,
                                LockTraits::AdoptLock) :
    _spinlock(lock),
    _ownsLock(lock.isLocked()),
    _isUpgraded(lock.isWriteLocked())
{
}

inline
ReadWriteSpinLock::Guard::~Guard()
{
    if (ownsLock()) {
        unlock();
    }
}

inline
void ReadWriteSpinLock::Guard::lockRead()
{
    assert (!ownsLock());
    _spinlock.lockRead();
    _ownsLock = true;
    _isUpgraded = false;
}

inline
void ReadWriteSpinLock::Guard::lockWrite()
{
    assert (!ownsLock());
    _spinlock.lockWrite();
    _ownsLock = _isUpgraded = true;
}

inline
bool ReadWriteSpinLock::Guard::tryLockRead()
{
    assert (!ownsLock());
    _ownsLock = _spinlock.tryLockRead();
    _isUpgraded = false;
    return _ownsLock;
}

inline
bool ReadWriteSpinLock::Guard::tryLockWrite()
{
    assert (!ownsLock());
    _ownsLock = _isUpgraded = _spinlock.tryLockWrite();
    return _ownsLock;
}

inline
void ReadWriteSpinLock::Guard::upgradeToWrite()
{
    assert (ownsLock());
    _spinlock.upgradeToWrite();
    _isUpgraded = true;
}

inline
bool ReadWriteSpinLock::Guard::tryUpgradeToWrite()
{
    assert (ownsLock());
    _isUpgraded = _spinlock.tryUpgradeToWrite();
    return _isUpgraded;
}

inline
bool ReadWriteSpinLock::Guard::ownsLock() const
{
    return _ownsLock;
}

inline
bool ReadWriteSpinLock::Guard::ownsReadLock() const
{
    return _ownsLock && !_isUpgraded;
}

inline
bool ReadWriteSpinLock::Guard::ownsWriteLock() const
{
    return _ownsLock && _isUpgraded;
}

inline
void ReadWriteSpinLock::Guard::unlock()
{
    assert(ownsLock());
    if (ownsReadLock()) {
        _spinlock.unlockRead();
    }
    else {
        _spinlock.unlockWrite();
    }
    _ownsLock = _isUpgraded = false;
}
    
}
}
