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
#ifndef QUANTUM_ITASKCONTINUATION_H
#define QUANTUM_ITASKCONTINUATION_H

#include <quantum/interface/quantum_itask.h>
#include <memory>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  interface ITaskContinuation
//==============================================================================================
/// @interface ITaskContinuation
/// @brief Interface to a task continuation. For internal use only.
struct ITaskContinuation : public ITask
{
    using ptr = std::shared_ptr<ITaskContinuation>;
    using wptr = std::weak_ptr<ITaskContinuation>;
    
    virtual ptr getNextTask() = 0;
    
    virtual void setNextTask(ptr nextTask) = 0;
    
    virtual ptr getPrevTask() = 0;
    
    virtual void setPrevTask(ptr nextTask) = 0;
    
    virtual ptr getFirstTask() = 0;
    
    virtual ptr getErrorHandlderOrFinalTask() = 0;
};

}}

#endif //QUANTUM_ITASKCONTINUATION_H
