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
#ifndef BLOOMBERG_QUANTUM_IO_TASK_H
#define BLOOMBERG_QUANTUM_IO_TASK_H

#include <quantum/interface/quantum_itask.h>
#include <quantum/quantum_capture.h>
#include <quantum/quantum_promise.h>
#include <quantum/util/quantum_util.h>
#include <functional>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class IoTask
//==============================================================================================
/// @class IoTask.
/// @brief Long running or blocking task running in the IO thread pool.
/// @note For internal use only.
class IoTask : public ITask
{
public:
    using Ptr = std::shared_ptr<IoTask>;
    using WeakPtr = std::weak_ptr<IoTask>;
    
    template <class RET, class FUNC, class ... ARGS>
    IoTask(std::true_type,
           std::shared_ptr<Promise<RET>> promise,
           int queueId,
           bool isHighPriority,
           FUNC&& func,
           ARGS&&... args);
    
    template <class RET, class FUNC, class ... ARGS>
    IoTask(std::false_type,
           std::shared_ptr<Promise<RET>> promise,
           int queueId,
           bool isHighPriority,
           FUNC&& func,
           ARGS&&... args);

    IoTask(const IoTask&) = delete;
    IoTask(IoTask&& other) noexcept;
    IoTask& operator=(const IoTask&) = delete;
    IoTask& operator=(IoTask&& other) noexcept;
    
    ~IoTask() override;
    
    //ITerminate
    void terminate() final;
    
    //ITask
    int run() final;
    void setQueueId(int queueId) final;
    int getQueueId() const final;
    Type getType() const final;
    TaskId getTaskId() const final;
    bool isBlocked() const final;
    bool isSleeping(bool updateTimer) final;
    bool isHighPriority() const final;
    bool isSuspended() const final;
    ITask::LocalStorage& getLocalStorage() final;
    
    //===================================
    //           NEW / DELETE
    //===================================
    static void* operator new(size_t size);
    static void operator delete(void* p);
    static void deleter(IoTask* p);
    
private:
    Function<int()>         _func;      //the current runnable io function
    std::atomic_bool        _terminated;
    int                     _queueId;
    bool                    _isHighPriority;
    TaskId                  _taskId;
    ITask::LocalStorage     _localStorage; // local storage of the IO task
};

using IoTaskPtr = IoTask::Ptr;
using IoTaskWeakPtr = IoTask::WeakPtr;

}}

#include <quantum/impl/quantum_io_task_impl.h>

#endif //BLOOMBERG_QUANTUM_IO_TASK_H
