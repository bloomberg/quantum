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
    _queueIt(_queue.end()),
    _isEmpty(true),
    _isInterrupted(false),
    _isIdle(true),
    _terminated(ATOMIC_FLAG_INIT)
{
    _thread = std::make_shared<std::thread>(std::bind(&TaskQueue::run, this));
}

inline
TaskQueue::~TaskQueue()
{
    terminate();
}

inline
void TaskQueue::pinToCore(int coreId)
{
    int cpuSetSize = sizeof(cpu_set_t);
    
    if (coreId >= 0 && (coreId <= cpuSetSize*8))
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(coreId, &cpuSet);
        pthread_setaffinity_np(_thread->native_handle(), cpuSetSize, &cpuSet);
    }
}

inline
void TaskQueue::run()
{
    try
    {
        while (true)
        {
            if (_isEmpty)
            {
                std::unique_lock<std::mutex> lock(_notEmptyMutex);
                //========================= BLOCK WHEN EMPTY =========================
                //Wait for the queue to have at least one element
                _notEmptyCond.wait(lock, [this]() -> bool { return !_isEmpty || _isInterrupted; });
            }
            
            if (_isInterrupted)
            {
                break;
            }

            //Iterate to the next runnable task
            advance();
            
            if (_queueIt == _queue.end())
            {
                continue;
            }
            
            //Process current task
            ITaskContinuation::ptr task = *_queueIt;
            if (task->isBlocked())
            {
                continue;
            }
            
            //========================= START/RESUME COROUTINE =========================
            _isIdle = false;
            int rc = task->run();
            _isIdle = true;
            //=========================== END/YIELD COROUTINE ==========================
            
            if (rc != (int)ITask::RetCode::Running) //Coroutine ended
            {
                ITaskContinuation::ptr nextTask;
                if (rc == (int)ITask::RetCode::Success)
                {
                    //Coroutine ended normally with "return 0" statement
                    _stats.incCompletedCount();
                    
                    //check to see interfaces there's another task scheduled to run after this one
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
                    
#ifdef _QUANTUM_PRINT_DEBUG_
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
                    //Check to see interfaces we have a final task to run
                    nextTask = task->getErrorHandlderOrFinalTask();
                }
                
                if (nextTask)
                {
                    //queue it
                    enqueue(nextTask);
                }
                
                task->terminate(); //terminate current task
                
                SpinLock::Guard lock(_spinlock);
                //========================= LOCKED SCOPE =========================
                //Remove completed task from the queue
                _queueIt = _queue.erase(_queueIt);
            }
        }
    }
    catch (std::exception& ex)
    {
        UNUSED(ex);
#ifdef _QUANTUM_PRINT_DEBUG_
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr << "Caught exception: " << ex.what() << std::endl;
#endif
    }
}

inline
void TaskQueue::enqueue(ITask::ptr task)
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_spinlock);
    _stats.incPostedCount();
    
    //NOTE: _queueIt remains unchanged following this operation
    if (_queue.empty() || !task->isHighPriority())
    {
        //insert before the current position. If _queueIt == begin(), then the new
        //task will be at the head of the queue.
        _queue.insert(_queueIt, std::static_pointer_cast<Task>(task));
    }
    else
    {
        //insert after the current position. If next(_queueIt) == end()
        //then the new task will be the last element in the queue
        _queue.insert(std::next(_queueIt), std::static_pointer_cast<Task>(task));
    }
    if (task->isHighPriority()) _stats.incHighPriorityCount();
    signalEmptyCondition(false);
}

inline
ITask::ptr TaskQueue::dequeue()
{
    //not supported
    return nullptr;
}

inline
size_t TaskQueue::size() const
{
#if (__cplusplus >= 201703L)
    return _queue.size();
#else
    //Until c++17, list::size() takes linear time. It must also be protected, hence making code much slower.
    return _stats.postedCount() - _stats.errorCount() - _stats.completedCount();
#endif
}

inline
bool TaskQueue::empty() const
{
    return _queue.empty();
}

inline
void TaskQueue::terminate()
{
    if (!_terminated.test_and_set())
    {
        _isInterrupted = true;
        _notEmptyCond.notify_all();
        _thread->join();
        
        //clear the queue
        while (!_queue.empty())
        {
            _queue.front()->terminate();
            _queue.pop_front();
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
    std::lock_guard<std::mutex> lock(_notEmptyMutex);
    _isEmpty = value;
    if (!value)
    {
        _notEmptyCond.notify_all();
    }
}

inline
void TaskQueue::advance()
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_spinlock);
    //Iterate to the next element
    if ((_queueIt == _queue.end()) || (++_queueIt == _queue.end()))
    {
        _queueIt = _queue.begin();
    }
    if (_queueIt == _queue.end())
    {
        signalEmptyCondition(true);
    }
}

inline
bool TaskQueue::isIdle() const
{
    return _isIdle;
}

}}

