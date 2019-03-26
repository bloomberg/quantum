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
#ifndef BLOOMBERG_QUANTUM_ITASKCONTINUATION_H
#define BLOOMBERG_QUANTUM_ITASKCONTINUATION_H

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
    using Ptr = std::shared_ptr<ITaskContinuation>;
    using WeakPtr = std::weak_ptr<ITaskContinuation>;
    
    virtual Ptr getNextTask() = 0;
    
    virtual void setNextTask(Ptr nextTask) = 0;
    
    virtual Ptr getPrevTask() = 0;
    
    virtual void setPrevTask(Ptr nextTask) = 0;
    
    virtual Ptr getFirstTask() = 0;
    
    virtual Ptr getErrorHandlerOrFinalTask() = 0;
};

using ITaskContinuationPtr = ITaskContinuation::Ptr;
using ITaskContinuationWeakPtr = ITaskContinuation::WeakPtr;

}}

#endif //BLOOMBERG_QUANTUM_ITASKCONTINUATION_H
