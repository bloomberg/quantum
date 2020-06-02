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
//                                class ReadWriteMutex
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
//                                class ReadGuard
//==============================================================================================
inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock) :
    ReadWriteMutex::ReadGuard::ReadGuard(nullptr, lock)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ICoroSync::Ptr sync,
                                     ReadWriteMutex& lock) :
    _mutex(&lock),
    _ownsLock(true),
    _isUpgraded(false)
{
    _mutex->lockRead(sync);
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock,
                                     ReadWriteMutex::TryToLock) :
    _mutex(&lock),
    _ownsLock(_mutex->tryLockRead()),
    _isUpgraded(false)
{
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock,
                                     ReadWriteMutex::AdoptLock) :
    _mutex(&lock),
    _ownsLock(lock.isLocked()),
    _isUpgraded(lock.isWriteLocked())
{
}

inline
ReadWriteMutex::ReadGuard::~ReadGuard()
{
    if (ownsLock()) {
        unlock();
    }
}

inline
void ReadWriteMutex::ReadGuard::lock()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lock(nullptr);
}

inline
void ReadWriteMutex::ReadGuard::lock(ICoroSync::Ptr sync)
{
    assert(_mutex && !ownsLock());
    _mutex->lockRead(sync);
    _ownsLock = true;
}

inline
bool ReadWriteMutex::ReadGuard::tryLock()
{
    
    assert(_mutex && !ownsLock());
    _ownsLock = _mutex->tryLockRead();
    return _ownsLock;
}

inline
void ReadWriteMutex::ReadGuard::upgradeToWrite()
{
    //Application must use the other upgradeToWrite() overload if we are running inside a coroutine
    assert(!local::context());
    upgradeToWrite(nullptr);
}

inline
void ReadWriteMutex::ReadGuard::upgradeToWrite(ICoroSync::Ptr sync)
{
    assert(_mutex && ownsReadLock());
    _mutex->upgradeToWrite(sync);
    _isUpgraded = true;
}

inline
bool ReadWriteMutex::ReadGuard::tryUpgradeToWrite()
{
    assert(_mutex && ownsReadLock());
    _isUpgraded = _mutex->tryUpgradeToWrite();
    return _isUpgraded;
}

inline
void ReadWriteMutex::ReadGuard::unlock()
{
    assert(_mutex && ownsLock());
    if (ownsReadLock()) {
        _mutex->unlockRead();
    }
    else {
        _mutex->unlockWrite();
    }
    _ownsLock = false;
    _isUpgraded = false;
}

inline
void ReadWriteMutex::ReadGuard::release()
{
    _ownsLock = false;
    _isUpgraded = false;
    _mutex = nullptr;
}

inline
bool ReadWriteMutex::ReadGuard::ownsLock() const
{
    return _ownsLock;
}

inline
bool ReadWriteMutex::ReadGuard::ownsReadLock() const
{
    return _ownsLock && !_isUpgraded;
}

inline
bool ReadWriteMutex::ReadGuard::ownsWriteLock() const
{
    return _ownsLock && _isUpgraded;
}

//==============================================================================================
//                                class WriteGuard
//==============================================================================================
inline
ReadWriteMutex::WriteGuard::WriteGuard(ReadWriteMutex& lock) :
    ReadWriteMutex::WriteGuard::WriteGuard(nullptr, lock)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
ReadWriteMutex::WriteGuard::WriteGuard(ICoroSync::Ptr sync,
                                       ReadWriteMutex& lock) :
    _mutex(&lock),
    _ownsLock(true)
{
    _mutex->lockWrite(sync);
}

inline
ReadWriteMutex::WriteGuard::WriteGuard(ReadWriteMutex& lock,
                                       ReadWriteMutex::TryToLock) :
    _mutex(&lock),
    _ownsLock(_mutex->tryLockWrite())
{
}

inline
ReadWriteMutex::WriteGuard::WriteGuard(ReadWriteMutex& lock,
                                       ReadWriteMutex::AdoptLock) :
    _mutex(&lock),
    _ownsLock(lock.isWriteLocked())
{
}

inline
ReadWriteMutex::WriteGuard::~WriteGuard()
{
    if (ownsLock()) {
        unlock();
    }
}

inline
void ReadWriteMutex::WriteGuard::lock()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lock(nullptr);
}

inline
void ReadWriteMutex::WriteGuard::lock(ICoroSync::Ptr sync)
{
    assert(_mutex && !ownsLock());
    _mutex->lockWrite(sync);
    _ownsLock = true;
}

inline
bool ReadWriteMutex::WriteGuard::tryLock()
{
    assert(_mutex && !ownsLock());
    _ownsLock = _mutex->tryLockWrite();
    return _ownsLock;
}

inline
void ReadWriteMutex::WriteGuard::unlock()
{
    assert(_mutex && ownsLock());
    _mutex->unlockWrite();
    _ownsLock = false;
}

inline
void ReadWriteMutex::WriteGuard::release()
{
    _ownsLock = false;
    _mutex = nullptr;
}

inline
bool ReadWriteMutex::WriteGuard::ownsLock() const
{
    return _ownsLock;
}

}}

