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

namespace Bloomberg {
namespace quantum {

void yield(ICoroSync::Ptr sync);

//==============================================================================================
//                                 ReadWriteMutex
//==============================================================================================
inline
void ReadWriteMutex::lockRead()
{
    //Application must use the other lockRead() overload if we are running inside a coroutine
    assert(!local::context());
    lockRead(nullptr);
}

inline
void ReadWriteMutex::lockRead(ICoroSync::Ptr sync)
{
    while (!tryLockRead())
    {
        yield(sync);
    }
}

inline
void ReadWriteMutex::lockWrite()
{
    //Application must use the other lockWrite() overload if we are running inside a coroutine
    assert(!local::context());
    lockWrite(nullptr);
}

inline
void ReadWriteMutex::lockWrite(ICoroSync::Ptr sync)
{
    while (!tryLockWrite())
    {
        yield(sync);
    }
}

inline
bool ReadWriteMutex::tryLockRead()
{
    return _spinlock.tryLockRead();
}

inline
bool ReadWriteMutex::tryLockWrite()
{
    bool rc = _spinlock.tryLockWrite();
    if (rc) {
        //mutex is write-locked
        _taskId = local::taskId();
        //task id must be valid
        assert(_taskId != TaskId{});
    }
    return rc;
}

inline
void ReadWriteMutex::upgradeToWrite()
{
    //Application must use the other upgradeToWrite() overload if we are running inside a coroutine
    assert(!local::context());
    upgradeToWrite(nullptr);
}

inline
void ReadWriteMutex::upgradeToWrite(ICoroSync::Ptr sync)
{
    while (!tryUpgradeToWrite())
    {
        yield(sync);
    }
}

inline
bool ReadWriteMutex::tryUpgradeToWrite()
{
    return _spinlock.tryUpgradeToWrite();
}

inline
void ReadWriteMutex::unlockRead()
{
    _spinlock.unlockRead();
}

inline
void ReadWriteMutex::unlockWrite()
{
    if (_taskId != local::taskId()) {
        //invalid operation
        assert(false);
    }
    _taskId = TaskId{}; //reset the task id
    _spinlock.unlockWrite();
}

inline
bool ReadWriteMutex::isLocked() const
{
    return _spinlock.isLocked();
}

inline
bool ReadWriteMutex::isReadLocked() const
{
    return _spinlock.isReadLocked();
}

inline
bool ReadWriteMutex::isWriteLocked() const
{
    return _spinlock.isWriteLocked();
}

inline
int ReadWriteMutex::numReaders() const
{
    return _spinlock.numReaders();
}

inline
int ReadWriteMutex::numPendingWriters() const
{
    return _spinlock.numPendingWriters();
}

//==============================================================================================
//                                 ReadWriteMutex::Guard
//==============================================================================================
inline
ReadWriteMutex::Guard::Guard(ReadWriteMutex& lock,
                             LockTraits::AcquireRead acquire) :
    ReadWriteMutex::Guard::Guard(nullptr, lock, acquire)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
ReadWriteMutex::Guard::Guard(ReadWriteMutex& lock,
                             LockTraits::AcquireWrite acquire) :
    ReadWriteMutex::Guard::Guard(nullptr, lock, acquire)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
ReadWriteMutex::Guard::Guard(ICoroSync::Ptr sync,
                             ReadWriteMutex& lock,
                             LockTraits::AcquireRead) :
    _mutex(&lock),
    _ownsLock(true),
    _isUpgraded(false)
{
    _mutex->lockRead(sync);
}

inline
ReadWriteMutex::Guard::Guard(ICoroSync::Ptr sync,
                             ReadWriteMutex& lock,
                             LockTraits::AcquireWrite) :
    _mutex(&lock),
    _ownsLock(true),
    _isUpgraded(true)
{
    _mutex->lockWrite(sync);
}

inline
ReadWriteMutex::Guard::Guard(ReadWriteMutex& lock,
                             LockTraits::AcquireRead,
                             LockTraits::TryToLock) :
    _mutex(&lock),
    _ownsLock(_mutex->tryLockRead()),
    _isUpgraded(false)
{
}

inline
ReadWriteMutex::Guard::Guard(ReadWriteMutex& lock,
                             LockTraits::AcquireWrite,
                             LockTraits::TryToLock) :
    _mutex(&lock),
    _ownsLock(_mutex->tryLockWrite()),
    _isUpgraded(_ownsLock)
{
}

inline
ReadWriteMutex::Guard::Guard(ReadWriteMutex& lock,
                             LockTraits::AdoptLock) :
    _mutex(&lock),
    _ownsLock(lock.isLocked()),
    _isUpgraded(lock.isWriteLocked())
{
}

inline
ReadWriteMutex::Guard::~Guard()
{
    if (ownsLock()) {
        unlock();
    }
}

inline
void ReadWriteMutex::Guard::lockRead()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lockRead(nullptr);
}

inline
void ReadWriteMutex::Guard::lockWrite()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lockWrite(nullptr);
}

inline
void ReadWriteMutex::Guard::lockRead(ICoroSync::Ptr sync)
{
    assert(_mutex && !ownsLock());
    _mutex->lockRead(sync);
    _ownsLock = true;
    _isUpgraded = false;
}

inline
void ReadWriteMutex::Guard::lockWrite(ICoroSync::Ptr sync)
{
    assert(_mutex && !ownsLock());
    _mutex->lockWrite(sync);
    _ownsLock = _isUpgraded = true;
}

inline
bool ReadWriteMutex::Guard::tryLockRead()
{
    
    assert(_mutex && !ownsLock());
    _ownsLock = _mutex->tryLockRead();
    return _ownsLock;
}

inline
bool ReadWriteMutex::Guard::tryLockWrite()
{
    
    assert(_mutex && !ownsLock());
    _ownsLock = _isUpgraded = _mutex->tryLockWrite();
    return _ownsLock;
}

inline
void ReadWriteMutex::Guard::upgradeToWrite()
{
    //Application must use the other upgradeToWrite() overload if we are running inside a coroutine
    assert(!local::context());
    upgradeToWrite(nullptr);
}

inline
void ReadWriteMutex::Guard::upgradeToWrite(ICoroSync::Ptr sync)
{
    assert(_mutex && ownsReadLock());
    _mutex->upgradeToWrite(sync);
    _isUpgraded = true;
}

inline
bool ReadWriteMutex::Guard::tryUpgradeToWrite()
{
    assert(_mutex && ownsReadLock());
    _isUpgraded = _mutex->tryUpgradeToWrite();
    return _isUpgraded;
}

inline
void ReadWriteMutex::Guard::unlock()
{
    assert(_mutex && ownsLock());
    if (ownsReadLock()) {
        _mutex->unlockRead();
    }
    else {
        _mutex->unlockWrite();
    }
    _ownsLock = _isUpgraded = false;
}

inline
void ReadWriteMutex::Guard::release()
{
    _ownsLock = _isUpgraded = false;
    _mutex = nullptr;
}

inline
bool ReadWriteMutex::Guard::ownsLock() const
{
    return _ownsLock;
}

inline
bool ReadWriteMutex::Guard::ownsReadLock() const
{
    return _ownsLock && !_isUpgraded;
}

inline
bool ReadWriteMutex::Guard::ownsWriteLock() const
{
    return _ownsLock && _isUpgraded;
}

}}

