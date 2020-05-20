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
#include <boost/functional/hash.hpp>
#include <functional>
#include <limits>
#include <atomic>

namespace Bloomberg {
namespace quantum {

inline
TaskId::TaskId(ThisThreadTag) :
    _threadId(std::this_thread::get_id())
{
}

inline
TaskId::TaskId(CoroContextTag) :
    _id(-generate())
{
    //the thread id will be populated when the task runs
}

inline
TaskId::TaskId(ThreadContextTag) :
    _id(generate())
{
    //the thread id will be populated when the task runs
}

inline
bool TaskId::operator==(const TaskId& rhs) const
{
    if (isCoroutine() && rhs.isCoroutine()) {
        //compare the coroutine id only since a coroutine may run
        //on different threads if shared mode is enabled
        return _id == rhs._id;
    }
    return std::tie(_id, _threadId) == std::tie(rhs._id, rhs._threadId);
}

inline
bool TaskId::operator!=(const TaskId& rhs) const
{
    return !operator==(rhs);
}

inline
bool TaskId::operator<(const TaskId& rhs) const
{
    if (isCoroutine() && rhs.isCoroutine()) {
        //compare the coroutine id only since a coroutine may run
        //on different threads if shared mode is enabled
        return _id < rhs._id;
    }
    return std::tie(_id, _threadId) < std::tie(rhs._id, rhs._threadId);
}

inline
bool TaskId::operator>(const TaskId& rhs) const
{
    return !operator==(rhs) && !operator<(rhs);
}

inline
bool TaskId::operator<=(const TaskId& rhs) const
{
    return !operator>(rhs);
}

inline
bool TaskId::operator>=(const TaskId& rhs) const
{
    return !operator<(rhs);
}

inline
size_t TaskId::hashValue() const
{
    size_t seed = std::hash<std::thread::id>()(_threadId);
    boost::hash_combine(seed, _id);
    return seed;
}

inline
size_t TaskId::id() const
{
    return static_cast<size_t>(_id);
}

inline
std::thread::id TaskId::threadId() const
{
    return _threadId;
}

inline
void TaskId::assignCurrentThread()
{
    _threadId = std::this_thread::get_id();
}

inline
bool TaskId::isCoroutine() const
{
    return _id < 0;
}

inline
ssize_t TaskId::generate()
{
    static std::atomic<ssize_t> gen{1};
    ssize_t i = gen.load(std::memory_order_acquire);
    ssize_t j = ((i+1) < 0) ? 1 : (i+1); //check for overflow
    while (!gen.compare_exchange_weak(i, j, std::memory_order_acq_rel)) {
        if (i < 0) {
            //rollover
            j = 1;
        }
        else {
            j = ((i+1) < 0) ? 1 : (i+1); //check for overflow
        }
    }
    return j;
}

inline
std::ostream& operator<<(std::ostream& os, const TaskId& tid)
{
    if (tid.isCoroutine()) {
        //task running on a coroutine
        os << 'C';
    }
    else {
        //task running on a thread
        os << 'T';
    }
    os << tid._id << ":";
    os << tid._threadId;
    return os;
}

}}
