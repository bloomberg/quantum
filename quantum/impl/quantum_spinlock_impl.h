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
#include <chrono>
#include <random>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                                   SpinLock
//==============================================================================
inline
void SpinLock::lock()
{
    SpinLockUtil::lockExclusive(_flag, 1, 0);
}

inline
bool SpinLock::tryLock()
{
    int i = 0;
    return _flag.compare_exchange_strong(i, 1, std::memory_order_acquire);
}

inline
void SpinLock::unlock()
{
    int i = 1;
    _flag.compare_exchange_strong(i, 0, std::memory_order_release);
}

inline
SpinLock::Guard::Guard(SpinLock& lock) :
    _spinlock(lock),
    _ownsLock(true)
{
    _spinlock.lock();
}

inline
SpinLock::Guard::Guard(SpinLock& lock, SpinLock::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLock())
{
}

inline
SpinLock::Guard::Guard(SpinLock& lock, SpinLock::AdoptLock) :
    _spinlock(lock),
    _ownsLock(true)
{
    //Make sure the lock is already taken
    assert(!_spinlock.tryLock());
}

inline
SpinLock::Guard::~Guard()
{
    if (_ownsLock) {
        _spinlock.unlock();
    }
}

inline
bool SpinLock::Guard::tryLock()
{
    if (!_ownsLock) {
        _ownsLock = _spinlock.tryLock();
    }
    return _ownsLock;
}

inline
void SpinLock::Guard::lock()
{
    if (!_ownsLock) {
        _spinlock.lock();
        _ownsLock = true;
    }
}

inline
bool SpinLock::Guard::ownsLock() const {
    return _ownsLock;
}

inline
void SpinLock::Guard::unlock() {
    if (_ownsLock) {
        _spinlock.unlock();
    }
}

inline
SpinLock::ReverseGuard::ReverseGuard(SpinLock& lock) :
    _spinlock(lock)
{
    _spinlock.unlock();
}

inline
SpinLock::ReverseGuard::~ReverseGuard()
{
    _spinlock.lock();
}

}}
