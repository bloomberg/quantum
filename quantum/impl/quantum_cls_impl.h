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

namespace Bloomberg {
namespace quantum {
namespace cls {

template <typename T>
T*& variable(const std::string& key)
{
    Task* task = TaskQueue::getCurrentTask();
    if (task == nullptr)
    {
        throw std::runtime_error("Current task not set");
    }
    
    void** r = &task->getCoroLocalStorage().emplace(key, nullptr).first->second;
    return *reinterpret_cast<T**>(r);
}

}}}
