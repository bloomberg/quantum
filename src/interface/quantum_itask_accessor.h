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
#ifndef BLOOMBERG_QUANTUM_ITASK_ACCESSOR_H
#define BLOOMBERG_QUANTUM_ITASK_ACCESSOR_H

#include <memory>
#include <quantum/interface/quantum_itask.h>
#include <quantum/interface/quantum_iterminate.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  interface ITaskAccessor
//==============================================================================================
/// @interface ITaskAccessor
/// @brief Interface to a task accessor. For internal use only.
struct ITaskAccessor : public ITerminate
{
    using Ptr = std::shared_ptr<ITaskAccessor>;
    
    virtual void setTask(ITask::Ptr task) = 0;
    
    virtual ITask::Ptr getTask() const = 0;
    
    virtual bool isBlocked() const = 0;
    
    virtual bool isSleeping(bool updateTimer = false) = 0;
};

using ITaskAccessorPtr = ITaskAccessor::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ITASK_ACCESSOR_H
