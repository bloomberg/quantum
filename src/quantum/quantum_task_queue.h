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
#ifndef QUANTUM_TASK_QUEUE_H
#define QUANTUM_TASK_QUEUE_H

#include <list>
#include <atomic>
#include <functional>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <quantum/interface/quantum_itask_continuation.h>
#include <quantum/interface/quantum_iterminate.h>
#include <quantum/interface/quantum_iqueue.h>
#include <quantum/quantum_spinlock.h>
#include <quantum/quantum_yielding_thread.h>
#include <quantum/quantum_queue_statistics.h>
#include <quantum/quantum_configuration.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class TaskQueue
//==============================================================================================
/// @class TaskQueue.
/// @brief Thread queue for running coroutines.
/// @note For internal use only.
class TaskQueue : public IQueue
{
public:
    using TaskList = std::list<Task::Ptr, QueueListAllocator>;
    using TaskListIter = TaskList::iterator;
    
    TaskQueue();
    
    explicit TaskQueue(const Configuration& config);
    
    TaskQueue(const TaskQueue& other);
    
    TaskQueue(TaskQueue&& other) = default;
    
    ~TaskQueue();
    
    void pinToCore(int coreId) final;
    
    void run() final;
    
    void enqueue(ITask::Ptr task) final;
    
    bool tryEnqueue(ITask::Ptr task) final;
    
    ITask::Ptr dequeue(std::atomic_bool& hint) final;
    
    ITask::Ptr tryDequeue(std::atomic_bool& hint) final;
    
    size_t size() const final;
    
    bool empty() const final;
    
    void terminate() final;
    
    IQueueStatistics& stats() final;
    
    SpinLock& getLock() final;
    
    void signalEmptyCondition(bool value) final;
    
    bool isIdle() const final;

private:
    TaskListIter advance();
    void doEnqueue(ITask::Ptr task);
    ITask::Ptr doDequeue(std::atomic_bool& hint);
    
    std::shared_ptr<std::thread>        _thread;
    TaskList                            _queue;
    TaskListIter                        _queueIt;
    mutable SpinLock                    _spinlock;
    std::mutex                          _notEmptyMutex; //for accessing the condition variable
    std::condition_variable             _notEmptyCond;
    std::atomic_bool                    _isEmpty;
    std::atomic_bool                    _isInterrupted;
    std::atomic_bool                    _isIdle;
    std::atomic_flag                    _terminated;
    bool                                _isAdvanced;
    QueueStatistics                     _stats;
};

}}

#include <quantum/impl/quantum_task_queue_impl.h>

#endif //QUANTUM_TASK_QUEUE_H
