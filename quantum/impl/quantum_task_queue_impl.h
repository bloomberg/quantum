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

namespace Bloomberg {
namespace quantum {

inline
TaskQueue::TaskQueue() :
    TaskQueue(Configuration())
{
}

inline
TaskQueue::TaskQueue(const Configuration&) :
    _alloc(Allocator<QueueListAllocator>::instance(AllocatorTraits::queueListAllocSize())),
    _runQueue(_alloc),
    _waitQueue(_alloc),
    _queueIt(_runQueue.end()),
    _blockedIt(_runQueue.end()),
    _isEmpty(true),
    _isInterrupted(false),
    _isIdle(true),
    _terminated(false),
    _isAdvanced(false)
{
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
    while (true)
    {
        try
        {
            if (_isEmpty)
            {
                _blockedIt = _runQueue.end(); //clear iterator
                std::unique_lock<std::mutex> lock(_notEmptyMutex);
                //========================= BLOCK WHEN EMPTY =========================
                //Wait for the queue to have at least one element
                _notEmptyCond.wait(lock, [this]()->bool { return !_isEmpty || _isInterrupted; });
            }
            
            if (_isInterrupted)
            {
                break;
            }
            
            //Iterate to the next runnable task
            if (advance() == _runQueue.end())
            {
                continue;
            }
            
            //Check if we need to pause this thread
            if (_blockedIt == _queueIt) {
                //All coroutines are blocked so we yield
                YieldingThread()();
            }
            
            //Process current task
            ITaskContinuation::Ptr task = *_queueIt;
            
            //Check if blocked or sleeping
            if (task->isBlocked() || task->isSleeping(true))
            {
                if (_blockedIt == _runQueue.end()) {
                    _blockedIt = _queueIt;
                }
                continue;
            }
            
            //========================= START/RESUME COROUTINE =========================
            int rc = task->run();
            //=========================== END/YIELD COROUTINE ==========================
            
            if (rc != (int)ITask::RetCode::Running) //Coroutine ended
            {
                //clear the blocked position iterator if it's the same as the finished task
                if (_blockedIt == _queueIt) {
                    _blockedIt = _runQueue.end();
                }
                ITaskContinuation::Ptr nextTask;
                if (rc == (int)ITask::RetCode::Success)
                {
                    //Coroutine ended normally with "return 0" statement
                    _stats.incCompletedCount();
                    
                    //check if there's another task scheduled to run after this one
                    nextTask = task->getNextTask();
                    if (nextTask && (nextTask->getType() == ITask::Type::ErrorHandler))
                    {
                        //skip error handler since we don't have any errors
                        nextTask->terminate(); //invalidate the error handler
                        nextTask = nextTask->getNextTask();
                    }
                }
                else
                {
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
                    //Check if we have a final task to run
                    nextTask = task->getErrorHandlerOrFinalTask();
                }
                //queue next task and de-queue current one
                enqueue(nextTask);
                dequeue(_isIdle);
            }
            else if (!task->isBlocked() && !task->isSleeping()) {
                //This coroutine will run again so we reset the blocked position iterator
                _blockedIt = _runQueue.end();
            }
        }
        catch (std::exception& ex)
        {
            UNUSED(ex);
            dequeue(_isIdle); //remove error task
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Caught exception: " << ex.what() << std::endl;
#endif
        }
        catch (...)
        {
            dequeue(_isIdle); //remove error task
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Caught unknown exception." << std::endl;
#endif
        }
    } //while(true)
}

inline
void TaskQueue::enqueue(ITask::Ptr task)
{
    if (!task)
    {
        return; //nothing to do
    }
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_spinlock);
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
    SpinLock::Guard lock(_spinlock, SpinLock::TryToLock{});
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
    _stats.incPostedCount();
    _stats.incNumElements();
    if (isEmpty)
    {
        //signal on transition from 0 to 1 element only
        signalEmptyCondition(false);
    }
}

inline
ITask::Ptr TaskQueue::dequeue(std::atomic_bool& hint)
{
    return doDequeue(hint);
}

inline
ITask::Ptr TaskQueue::tryDequeue(std::atomic_bool& hint)
{
    return doDequeue(hint);
}

inline
ITask::Ptr TaskQueue::doDequeue(std::atomic_bool& hint)
{
    hint = (_queueIt == _runQueue.end());
    if (hint)
    {
        return nullptr;
    }
    ITask::Ptr task = *_queueIt;
    task->terminate();
    //Remove error task from the queue
    _queueIt = _runQueue.erase(_queueIt);
    //_queueIt now points to the next element in the list or to _queue.end()
    _stats.decNumElements();
    _isAdvanced = true;
    return task;
}

inline
size_t TaskQueue::size() const
{
    return _stats.numElements();
}

inline
bool TaskQueue::empty() const
{
    return _stats.numElements() == 0;
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
        SpinLock::Guard lock(_spinlock);
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
    return _spinlock;
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
}

inline
TaskQueue::TaskListIter TaskQueue::advance()
{
    //Iterate to the next element
    if ((_queueIt == _runQueue.end()) || (!_isAdvanced && (++_queueIt == _runQueue.end())))
    {
        acquireWaiting();
    }
    _isAdvanced = false; //reset flag
    if (_queueIt == _runQueue.end())
    {
        signalEmptyCondition(true);
    }
    return _queueIt;
}

inline
bool TaskQueue::isIdle() const
{
    return _isIdle;
}

inline
void TaskQueue::acquireWaiting()
{
    //========================= LOCKED SCOPE =========================
    if (_waitQueue.empty())
    {
        _queueIt = _runQueue.begin();
        return;
    }
    bool isEmpty = _runQueue.empty();
    if (!isEmpty)
    {
        //rewind by one since we are at end()
        --_queueIt;
    }
    {
        //splice wait queue unto run queue.
        SpinLock::Guard lock(_spinlock);
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
    }
}

}}

