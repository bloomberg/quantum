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
#ifndef BLOOMBERG_QUANTUM_TASK_QUEUE_H
#define BLOOMBERG_QUANTUM_TASK_QUEUE_H

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
#include <quantum/quantum_task.h>
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
    using TaskList = std::list<Task::Ptr, ContiguousPoolManager<ITask::Ptr>>;
    using TaskListIter = TaskList::iterator;
    
    TaskQueue();
    
    TaskQueue(const Configuration& config,
              std::shared_ptr<TaskQueue> sharedQueue);
    
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
    
    const std::shared_ptr<std::thread>& getThread() const final;

    static Task* getCurrentTask();

    static void setCurrentTask(Task* task);

private:
    struct WorkItem
    {
        WorkItem(TaskPtr task,
                 TaskListIter iter,
                 bool isBlocked,
                 unsigned int blockedQueueRound);
        
        TaskPtr _task;                   // task pointer
        TaskListIter _iter;              // task iterator
        bool _isBlocked;                 // true if the entire queue is blocked
        unsigned int _blockedQueueRound; // blocked queue round id
    };
    struct ProcessTaskResult
    {
        ProcessTaskResult(bool isBlocked,
                          unsigned int blockedQueueRound);
            
        bool _isBlocked;                 // true if the entire queue is blocked
        unsigned int _blockedQueueRound; // blocked queue round id
    };
    struct CurrentTaskSetter
    {
        CurrentTaskSetter(TaskQueue& taskQueue, const TaskPtr & task);
        ~CurrentTaskSetter();

        TaskQueue& _taskQueue;
    };
    //Coroutine result handlers
    bool handleNotCallable(const WorkItem& entry);
    bool handleAlreadyResumed(WorkItem& entry);
    bool handleRunning(WorkItem& entry);
    bool handleSuccess(const WorkItem& entry);
    bool handleBlocked(WorkItem& entry);
    bool handleSleeping(WorkItem& entry);
    bool handleError(const WorkItem& entry);
    bool handleException(const WorkItem& workItem,
                         const std::exception* ex = nullptr);

    void onBlockedTask(WorkItem& entry);
    void onActiveTask(WorkItem& entry);
    
    bool isInterrupted();
    void signalSharedQueueEmptyCondition(bool value);
    ProcessTaskResult processTask();
    WorkItem grabWorkItem();
    void doEnqueue(ITask::Ptr task);
    ITask::Ptr doDequeue(std::atomic_bool& hint,
                         TaskListIter iter);
    void acquireWaiting();
    void sleepOnBlockedQueue(const ProcessTaskResult& mainQueueResult);
    void sleepOnBlockedQueue(const ProcessTaskResult& mainQueueResult,
                             const ProcessTaskResult& sharedQueueResult);

    
    QueueListAllocator                  _alloc;
    std::shared_ptr<std::thread>        _thread;
    TaskList                            _runQueue;
    TaskList                            _waitQueue;
    TaskListIter                        _queueIt;
    TaskListIter                        _blockedIt;
    bool                                _isBlocked;
    mutable SpinLock                    _runQueueLock;
    mutable SpinLock                    _waitQueueLock;
    std::mutex                          _notEmptyMutex; //for accessing the condition variable
    std::condition_variable             _notEmptyCond;
    std::atomic_bool                    _isEmpty;
    std::atomic_bool                    _isSharedQueueEmpty;
    std::atomic_bool                    _isInterrupted;
    std::atomic_bool                    _isIdle;
    std::atomic_bool                    _terminated;
    bool                                _isAdvanced;
    QueueStatistics                     _stats;
    std::shared_ptr<TaskQueue>          _sharedQueue;
    std::vector<TaskQueue*>             _helpers;
    unsigned int                        _queueRound;
    unsigned int                        _lastSleptQueueRound;
    unsigned int                        _lastSleptSharedQueueRound;
};

}}

#include <quantum/impl/quantum_task_queue_impl.h>

#endif //BLOOMBERG_QUANTUM_TASK_QUEUE_H
