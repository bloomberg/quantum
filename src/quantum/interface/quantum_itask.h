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
#ifndef QUANTUM_ITASK_H
#define QUANTUM_ITASK_H

#include <memory>
#include <limits>
#include <quantum/quantum_traits.h>
#include <quantum/interface/quantum_iterminate.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  interface ITask
//==============================================================================================
/// @interface ITask
/// @brief Interface to a task. For internal use only.
struct ITask : public ITerminate
{
    using Ptr = std::shared_ptr<ITask>;
    using WeakPtr = std::weak_ptr<ITask>;
    
    enum class Type : int
    {
        Standalone, First, Continuation, ErrorHandler, Final, Termination, IO
    };
    
    enum class RetCode : int
    {
        Success = 0,
        Running = std::numeric_limits<int>::max(),
        Exception = (int)Running-1,
        NotCallable = (int)Running-2,
    };
    
    ~ITask() = default;
    
    virtual int run() = 0;
    
    virtual void setQueueId(int queueId) = 0;
    
    virtual int getQueueId() = 0;
    
    virtual Type getType() const = 0;
    
    virtual bool isBlocked() const = 0;
    
    virtual bool isHighPriority() const = 0;
};

}}

#endif //QUANTUM_ITASK_H
