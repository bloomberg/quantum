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
#include <quantum/quantum_task_queue.h>
#include <stdexcept>
#include <memory>

namespace Bloomberg {
namespace quantum {

namespace local {

template <typename T>
T*& variable(const std::string& key)
{
    // default thread local map to be used outside of coroutines
    thread_local ITask::LocalStorage defaultStorage;
    
    ITask::Ptr task = IQueue::getCurrentTask();
    ITask::LocalStorage& storage = task ? task->getLocalStorage() : defaultStorage;
    
    void** r = &storage.emplace(key, nullptr).first->second;
    return *reinterpret_cast<T**>(r);
}

inline
VoidContextPtr context()
{
    ITask::Ptr task = IQueue::getCurrentTask();
    if (!task || (task->getType() == ITask::Type::IO))
    {
        return nullptr;
    }
    return std::static_pointer_cast<Context<Void>, ITaskAccessor>
        (std::static_pointer_cast<Task>(task)->getTaskAccessor());
}

inline
TaskId taskId()
{
    ITask::Ptr task = IQueue::getCurrentTask();
    return task ? task->getTaskId() : TaskId(TaskId::ThisThreadTag{});
}

}}}
