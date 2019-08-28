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

#include <limits>

namespace Bloomberg {
namespace quantum {

inline
TaskQueue::WorkItem::WorkItem(TaskPtr task,
                              TaskListIter iter,
                              bool isBlocked,
                              unsigned int blockedQueueRound) :
    _task(task),
    _iter(iter),
    _isBlocked(isBlocked),
    _blockedQueueRound(blockedQueueRound)
{
}

inline
TaskQueue::ProcessTaskResult::ProcessTaskResult(bool isBlocked,
                                                unsigned int blockedQueueRound) :
    _isBlocked(isBlocked),
    _blockedQueueRound(blockedQueueRound)
{
}

inline
TaskQueue::CurrentTaskSetter::CurrentTaskSetter(TaskQueue& taskQueue, const TaskPtr & task) :
    _taskQueue(taskQueue)
{
    _taskQueue.setCurrentTask(task.get());
}

inline
TaskQueue::CurrentTaskSetter::~CurrentTaskSetter()
{
    _taskQueue.setCurrentTask(nullptr);
}
    
inline
TaskQueue::TaskQueue() :
    TaskQueue(Configuration(), nullptr)
{
}

inline
TaskQueue::TaskQueue(const Configuration&, std::shared_ptr<TaskQueue> sharedQueue) :
    _alloc(Allocator<QueueListAllocator>::instance(AllocatorTraits::queueListAllocSize())),
    _runQueue(_alloc),
    _waitQueue(_alloc),
    _queueIt(_runQueue.end()),
    _blockedIt(_runQueue.end()),
    _isBlocked(false),
    _isEmpty(true),
    _isSharedQueueEmpty(true),
    _isInterrupted(false),
    _isIdle(true),
    _terminated(false),
    _isAdvanced(false),
    _sharedQueue(sharedQueue),
    _queueRound(0),
    _lastSleptQueueRound(std::numeric_limits<unsigned int>::max()),
    _lastSleptSharedQueueRound(std::numeric_limits<unsigned int>::max())
{
    if (_sharedQueue)
    {
        _sharedQueue->_helpers.push_back(this);
    }
    _thread = std::make_shared<std::thread>(std::bind(&TaskQueue::run, this));
}

inline
TaskQueue::TaskQueue(const TaskQueue&) :
    TaskQueue()
{

}

inline
TaskQueue::~TaskQueue()
{
    terminate();
}

inline
void TaskQueue::pinToCore(int coreId)
{
#ifdef _WIN32
    SetThreadAffinityMask(_thread->native_handle(), 1 << coreId);
#else
    int cpuSetSize = sizeof(cpu_set_t);
    if (coreId >= 0 && (coreId <= cpuSetSize*8))
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(coreId, &cpuSet);
        pthread_setaffinity_np(_thread->native_handle(), cpuSetSize, &cpuSet);
    }
#endif
}


inline
void TaskQueue::run()
{
    while (!isInterrupted())
    {
        const ProcessTaskResult result = processTask();
        if (_sharedQueue)
        {
            const ProcessTaskResult sharedResult = _sharedQueue->processTask();
            sleepOnBlockedQueue(result, sharedResult);
        }
        else
        {
            sleepOnBlockedQueue(result);
        }
    }
}

inline
void TaskQueue::sleepOnBlockedQueue(const ProcessTaskResult& mainQueueResult)
{
    if (mainQueueResult._isBlocked && mainQueueResult._blockedQueueRound != _lastSleptQueueRound)
    {
        _lastSleptQueueRound = mainQueueResult._blockedQueueRound;
        YieldingThread()();
    }
}
inline
void TaskQueue::sleepOnBlockedQueue(const ProcessTaskResult& mainQueueResult,
                                    const ProcessTaskResult& sharedQueueResult)
{
    const bool allQueuesEmpty = _isEmpty && _isSharedQueueEmpty;
    if (allQueuesEmpty)
    {
        // if both queues are blocked, the thread will be waiting on _notEmptyCond
        return;
    }

    const bool isQueueBlocked =
        mainQueueResult._isBlocked && mainQueueResult._blockedQueueRound != _lastSleptQueueRound;
    const bool isSharedQueueBlocked =
        sharedQueueResult._isBlocked && sharedQueueResult._blockedQueueRound != _lastSleptSharedQueueRound;
    
    if ((isQueueBlocked || _isEmpty) && (isSharedQueueBlocked || _isSharedQueueEmpty))
    {
        _lastSleptQueueRound = mainQueueResult._blockedQueueRound;
        _lastSleptSharedQueueRound = sharedQueueResult._blockedQueueRound;
        YieldingThread()();
    }
}
    
inline
TaskQueue::ProcessTaskResult TaskQueue::processTask()
{
    WorkItem workItem{nullptr, _runQueue.end(), false, 0};
    try
    {
        //Process a task
        workItem = grabWorkItem();
        
        TaskPtr task = workItem._task;
        if (!task)
        {
            return ProcessTaskResult(workItem._isBlocked, workItem._blockedQueueRound);
        }

        int rc;
        {
            // set the current task for local-storage queries
            CurrentTaskSetter taskSetter(*this, task);
            //========================= START/RESUME COROUTINE =========================
            rc = task->run();
            //=========================== END/YIELD COROUTINE ==========================
        }
        
        switch (rc)
        {
            case (int)ITask::RetCode::NotCallable:
                handleNotCallable(workItem);
                break;
            case (int)ITask::RetCode::AlreadyResumed:
                handleAlreadyResumed(workItem);
                break;
            case (int)ITask::RetCode::Blocked:
                handleBlocked(workItem);
                break;
            case (int)ITask::RetCode::Sleeping:
                handleSleeping(workItem);
                break;
            case (int)ITask::RetCode::Running:
                handleRunning(workItem);
                break;
            case (int)ITask::RetCode::Success:
                handleSuccess(workItem);
                break;
            default:
                handleError(workItem);
                break;
        }
    }
    catch (const std::exception& ex)
    {
        handleException(workItem, &ex);
    }
    catch (...)
    {
        handleException(workItem);
    }
    return ProcessTaskResult(workItem._isBlocked, workItem._blockedQueueRound);
}

inline
void TaskQueue::enqueue(ITask::Ptr task)
{
    if (!task)
    {
        return; //nothing to do
    }
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_waitQueueLock);
    doEnqueue(task);
}

inline
bool TaskQueue::tryEnqueue(ITask::Ptr task)
{
    if (!task)
    {
        return false; //nothing to do
    }
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_waitQueueLock, SpinLock::TryToLock{});
    if (lock.ownsLock())
    {
        doEnqueue(task);
    }
    return lock.ownsLock();
}

inline
void TaskQueue::doEnqueue(ITask::Ptr task)
{
    //NOTE: _queueIt remains unchanged following this operation
    _stats.incPostedCount();
    _stats.incNumElements();
    bool isEmpty = _waitQueue.empty();
    if (task->isHighPriority())
    {
        //insert before the current position. If _queueIt == begin(), then the new
        //task will be at the head of the queue.
        _waitQueue.emplace_front(std::static_pointer_cast<Task>(task));
    }
    else
    {
        //insert after the current position. If next(_queueIt) == end()
        //then the new task will be the last element in the queue
        _waitQueue.emplace_back(std::static_pointer_cast<Task>(task));
    }
    if (task->isHighPriority())
    {
        _stats.incHighPriorityCount();
    }
    if (isEmpty)
    {
        //signal on transition from 0 to 1 element only
        signalEmptyCondition(false);
    }
}

inline
ITask::Ptr TaskQueue::dequeue(std::atomic_bool& hint)
{
    return doDequeue(hint, _queueIt);
}

inline
ITask::Ptr TaskQueue::tryDequeue(std::atomic_bool& hint)
{
    return doDequeue(hint, _queueIt);
}

inline
ITask::Ptr TaskQueue::doDequeue(std::atomic_bool&, TaskListIter iter)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    if (iter == _runQueue.end())
    {
        return nullptr;
    }
    if (iter == _blockedIt)
    {
        // we don't really know what's the next blocked task in the queue, so reset it
        _blockedIt = _runQueue.end();
    }
    ITask::Ptr task = *iter;

    task->terminate();
    if (_queueIt == iter)
    {
        _queueIt = _runQueue.erase(iter);
        _isAdvanced = true;
    }
    else
    {
        _runQueue.erase(iter);
    }
    _stats.decNumElements();
    return task;
}

inline
size_t TaskQueue::size() const
{
    return _isIdle ? _stats.numElements() : _stats.numElements() + 1;
}

inline
bool TaskQueue::empty() const
{
    return size() == 0;
}

inline
void TaskQueue::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        {
            std::unique_lock<std::mutex> lock(_notEmptyMutex);
            _isInterrupted = true;
        }
        _notEmptyCond.notify_all();
        _thread->join();
        
        //clear the queues
        while (!_runQueue.empty())
        {
            _runQueue.front()->terminate();
            _runQueue.pop_front();
        }
        //========================= LOCKED SCOPE =========================
        SpinLock::Guard lock(_waitQueueLock);
        while (!_waitQueue.empty())
        {
            _waitQueue.front()->terminate();
            _waitQueue.pop_front();
        }
    }
}

inline
IQueueStatistics& TaskQueue::stats()
{
    return _stats;
}

inline
SpinLock& TaskQueue::getLock()
{
    return _waitQueueLock;
}

inline
void TaskQueue::signalEmptyCondition(bool value)
{
    {
        //========================= LOCKED SCOPE =========================
        std::lock_guard<std::mutex> lock(_notEmptyMutex);
        _isEmpty = value;
    }
    if (!value)
    {
        _notEmptyCond.notify_all();
    }
    // Notify helpers as well
    for(TaskQueue* helper : _helpers)
    {
        helper->signalSharedQueueEmptyCondition(value);
    }
}

inline
void TaskQueue::signalSharedQueueEmptyCondition(bool value)
{
    {
        //========================= LOCKED SCOPE =========================
        std::lock_guard<std::mutex> lock(_notEmptyMutex);
        _isSharedQueueEmpty = value;
    }
    if (!value)
    {
        _notEmptyCond.notify_all();
    }
}

inline
bool TaskQueue::handleNotCallable(const WorkItem& workItem)
{
    return handleError(workItem);
}

inline
bool TaskQueue::handleAlreadyResumed(WorkItem& entry)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    onBlockedTask(entry);
    return false;
}

inline
bool TaskQueue::handleBlocked(WorkItem& entry)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    onBlockedTask(entry);
    return false;
}

inline
bool TaskQueue::handleSleeping(WorkItem& entry)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    onBlockedTask(entry);
    return false;
}

inline
bool TaskQueue::handleRunning(WorkItem& entry)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    onActiveTask(entry);
    return true;
}

inline
bool TaskQueue::handleSuccess(const WorkItem& workItem)
{
    ITaskContinuation::Ptr nextTask;
    //check if there's another task scheduled to run after this one
    nextTask = workItem._task->getNextTask();
    if (nextTask && (nextTask->getType() == ITask::Type::ErrorHandler))
    {
        //skip error handler since we don't have any errors
        nextTask->terminate(); //invalidate the error handler
        nextTask = nextTask->getNextTask();
    }
    //queue next task and de-queue current one
    enqueue(nextTask);
    doDequeue(_isIdle, workItem._iter);
    //Coroutine ended normally with "return 0" statement
    _stats.incCompletedCount();
    return true;
}

inline
bool TaskQueue::handleError(const WorkItem& workItem)
{
    ITaskContinuation::Ptr nextTask;
    //Check if we have a final task to run
    nextTask = workItem._task->getErrorHandlerOrFinalTask();
    //queue next task and de-queue current one
    enqueue(nextTask);
    doDequeue(_isIdle, workItem._iter);
    //Coroutine ended with explicit user error
    _stats.incErrorCount();
#ifdef __QUANTUM_PRINT_DEBUG
    std::lock_guard<std::mutex> guard(Util::LogMutex());
    if (rc == (int)ITask::RetCode::Exception)
    {
        std::cerr << "Coroutine exited with user exception." << std::endl;
    }
    else
    {
        std::cerr << "Coroutine exited with error : " << rc << std::endl;
    }
#endif
    return false;
}

inline
bool TaskQueue::handleException(const WorkItem& workItem, const std::exception* ex)
{
    UNUSED(ex);
    doDequeue(_isIdle, workItem._iter);
#ifdef __QUANTUM_PRINT_DEBUG
    std::lock_guard<std::mutex> guard(Util::LogMutex());
    if (ex != nullptr) {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
    }
    else {
        std::cerr << "Caught unknown exception." << std::endl;
    }
#endif
    return false;
}

inline
bool TaskQueue::isInterrupted()
{
    if (_isEmpty && _isSharedQueueEmpty)
    {
        std::unique_lock<std::mutex> lock(_notEmptyMutex);
        //========================= BLOCK WHEN EMPTY =========================
        //Wait for the queue to have at least one element
        _notEmptyCond.wait(lock, [this]()->bool { return !_isEmpty || !_isSharedQueueEmpty || _isInterrupted; });
    }
    return _isInterrupted;
}

inline
TaskQueue::WorkItem
TaskQueue::grabWorkItem()
{    
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_runQueueLock);
    if ((_queueIt == _runQueue.end()) || (!_isAdvanced && (++_queueIt == _runQueue.end())))
    {
        acquireWaiting();
    }
    _isAdvanced = false; //reset flag
    _isIdle = _runQueue.empty();
    if (_runQueue.empty())
    {
        return WorkItem(nullptr, _runQueue.end(), _isBlocked, _queueRound);
    }
    return WorkItem((*_queueIt), _queueIt, false, 0);
}

inline
void TaskQueue::onBlockedTask(WorkItem& entry)
{
    if (_blockedIt == _runQueue.end())
    {
        _blockedIt = entry._iter;
    }
    else if (_blockedIt == entry._iter)
    {
        _isBlocked = true;
        _blockedIt = _runQueue.end();
    }
    entry._isBlocked = _isBlocked;
    entry._blockedQueueRound = _queueRound;
}

inline
void TaskQueue::onActiveTask(WorkItem& entry)
{
    _isBlocked = false;
    _blockedIt = _runQueue.end();
    entry._isBlocked = _isBlocked;
    entry._blockedQueueRound = _queueRound;
}

inline
bool TaskQueue::isIdle() const
{
    return _isIdle;
}

inline
const std::shared_ptr<std::thread>& TaskQueue::getThread() const
{
    return _thread;
}

inline
void TaskQueue::acquireWaiting()
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_waitQueueLock);
    bool isEmpty = _runQueue.empty();
    if (_waitQueue.empty())
    {
        if (isEmpty)
        {
            signalEmptyCondition(true);
        }
        _queueIt = _runQueue.begin();
        ++_queueRound;
        return;
    }
    if (!isEmpty)
    {
        //rewind by one since we are at end()
        --_queueIt;
    }
    {        
        //splice wait queue unto run queue.
        _runQueue.splice(_runQueue.end(), _waitQueue);
    }
    if (!isEmpty)
    {
        //move to first element from spliced queue
        ++_queueIt;
    }
    else
    {
        _queueIt = _runQueue.begin();
        ++_queueRound;
    }
}

inline
Task*& getCurrentTaskImpl()
{
    static thread_local Task* currentTask = nullptr;
    return currentTask;
}

inline
Task* TaskQueue::getCurrentTask()
{
    return getCurrentTaskImpl();
}

inline void TaskQueue::setCurrentTask(Task* task)
{
    getCurrentTaskImpl() = task;
}

}}

