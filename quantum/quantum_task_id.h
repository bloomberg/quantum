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
#ifndef BLOOMBERG_QUANTUM_TASK_ID_H
#define BLOOMBERG_QUANTUM_TASK_ID_H

#include <ostream>
#include <string>
#include <functional>
#include <thread>

namespace Bloomberg {
namespace quantum {

struct CoroContextTag{};
struct ThreadContextTag{};

// Fwd declarations
class TaskId;
namespace local {
    TaskId taskId();
}

class TaskId
{
    friend class Task;
    friend class IoTask;
    friend std::ostream& operator<<(std::ostream&, const TaskId&);
    friend TaskId local::taskId();
public:
    TaskId() = default;
    
    //Equality operators
    bool operator==(const TaskId&) const;
    bool operator!=(const TaskId&) const;
    bool operator<(const TaskId&) const;
    bool operator>(const TaskId&) const;
    bool operator<=(const TaskId&) const;
    bool operator>=(const TaskId&) const;
    
    /// @brief Produces a hash value suitable for unordered map insertions
    /// @return A hash
    size_t hashValue() const;
    
    /// @brief Get the id associated with this coroutine or IO task.
    size_t id() const;
    
    /// @brief Get the thread id where the coroutine or IO task is executing.
    std::thread::id threadId() const;
    
    /// @brief Checks if this id belongs to a coroutine.
    /// @return True is it's a coroutine.
    bool isCoroutine() const;
    
protected:
    struct ThisThreadTag{};
    /// @brief Create using current thread id
    explicit TaskId(ThisThreadTag);
    /// @brief Create a coroutine id.
    explicit TaskId(CoroContextTag);
    /// @brief Create an IO task id.
    explicit TaskId(ThreadContextTag);
    
    void assignCurrentThread();
private:
    static ssize_t generate();
    
    ssize_t         _id{0}; //negative values reserved for coroutines
    std::thread::id _threadId;
};

std::ostream& operator<<(std::ostream&, const TaskId&);

}
}

#include <quantum/impl/quantum_task_id_impl.h>

namespace std {

template <>
struct hash<Bloomberg::quantum::TaskId>
{
    size_t operator()(const Bloomberg::quantum::TaskId& rhs) const {
        return rhs.hashValue();
    }
};

}

#endif //BLOOMBERG_QUANTUM_TASK_ID_H
