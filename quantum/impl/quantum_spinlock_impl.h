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

namespace Bloomberg {
namespace quantum {

inline
SpinLock::SpinLock() :
    _flag ATOMIC_FLAG_INIT
{}

inline
void SpinLock::lock()
{
    while (_flag.test_and_set(std::memory_order_acquire)); //spin
}

inline
bool SpinLock::tryLock()
{
    return !_flag.test_and_set(std::memory_order_acquire);
}

inline
void SpinLock::unlock()
{
    _flag.clear(std::memory_order_release);
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
