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

//==============================================================================================
//                                class ReadGuard
//==============================================================================================
inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock) :
    ReadWriteMutex::ReadGuard::ReadGuard(nullptr, lock)
{
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ICoroSync::Ptr sync,
                                     ReadWriteMutex& lock) :
    _mutex(&lock),
    _ownsLock(true)
{
    _mutex->lockRead(sync);
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock,
                                     ReadWriteMutex::TryToLock) :
    _mutex(&lock),
    _ownsLock(_mutex->tryLockRead())
{
}

inline
ReadWriteMutex::ReadGuard::ReadGuard(ReadWriteMutex& lock,
                                     ReadWriteMutex::AdoptLock) :
    _mutex(&lock),
    _ownsLock(true)
{
}

inline
ReadWriteMutex::ReadGuard::~ReadGuard()
{
    unlock();
}

inline
void ReadWriteMutex::ReadGuard::lock()
{
    lock(nullptr);
}

inline
void ReadWriteMutex::ReadGuard::lock(ICoroSync::Ptr sync)
{
    if (_ownsLock) return;
    assert(_mutex);
    _mutex->lockRead(sync);
    _ownsLock = true;
}

inline
bool ReadWriteMutex::ReadGuard::tryLock()
{
    
    if (_ownsLock) return true;
    assert(_mutex);
    _ownsLock = _mutex->tryLockRead();
    return _ownsLock;
}

inline
void ReadWriteMutex::ReadGuard::unlock()
{
    if (!_ownsLock) return;
    _mutex->unlockRead();
    _ownsLock = false;
}

inline
void ReadWriteMutex::ReadGuard::release()
{
    _ownsLock = false;
    _mutex = nullptr;
}

inline
bool ReadWriteMutex::ReadGuard::ownsLock() const
{
    return _ownsLock;
}

//==============================================================================================
//                                class WriteGuard
//==============================================================================================
inline
ReadWriteMutex::WriteGuard::WriteGuard(ReadWriteMutex& lock) :
    ReadWriteMutex::WriteGuard::WriteGuard(nullptr, lock)
{
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
    _ownsLock(true)
{
    assert(!_mutex->tryLockWrite());
}

inline
ReadWriteMutex::WriteGuard::~WriteGuard()
{
    unlock();
}

inline
void ReadWriteMutex::WriteGuard::lock()
{
    lock(nullptr);
}

inline
void ReadWriteMutex::WriteGuard::lock(ICoroSync::Ptr sync)
{
    if (_ownsLock) return;
    assert(_mutex);
    _mutex->lockWrite(sync);
    _ownsLock = true;
}

inline
bool ReadWriteMutex::WriteGuard::tryLock()
{
    
    if (_ownsLock) return true;
    assert(_mutex);
    _ownsLock = _mutex->tryLockWrite();
    return _ownsLock;
}

inline
void ReadWriteMutex::WriteGuard::unlock()
{
    if (!_ownsLock) return;
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

