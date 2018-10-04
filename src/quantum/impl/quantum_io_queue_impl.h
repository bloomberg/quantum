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
IoQueue::IoQueue(IoQueue* mainIoQueue) :
    _sharedIoQueue(mainIoQueue),
    _queue(Allocator<QueueListAllocator>::instance(AllocatorTraits::queueListAllocSize())),
    _isEmpty(true),
    _isInterrupted(false),
    _isIdle(true),
    _terminated(ATOMIC_FLAG_INIT)
{
    _thread = std::make_shared<std::thread>(std::bind(&IoQueue::run, this));
}

inline
IoQueue::IoQueue(const IoQueue& other) :
    IoQueue(other._sharedIoQueue)
{}

inline
IoQueue::~IoQueue()
{
    terminate();
}

inline
void IoQueue::pinToCore(int)
{
    //Not used
}

inline
void IoQueue::run()
{
    while (true)
    {
        try
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
            ITask::Ptr task = grabWorkItem();
            if (!task)
            {
                continue;
            }
            
            //========================= START TASK =========================
            _isIdle = false;
            int rc = task->run();
            _isIdle = true;
            //========================== END TASK ==========================

            if (rc == (int)ITask::RetCode::Success)
            {
                if (_sharedIoQueue)
                {
                    //this is a worker queue
                    if (task->getQueueId() == (int)IQueue::QueueId::Any)
                    {
                        _stats.incSharedQueueCompletedCount();
                    }
                    else
                    {
                        _stats.incCompletedCount();
                    }
                }
                else
                {
                    _stats.incCompletedCount();
                }
            }
            else
            {
                //IO task ended with error
                if (_sharedIoQueue)
                {
                    //this is a worker queue
                    if (task->getQueueId() == (int)IQueue::QueueId::Any)
                    {
                        _stats.incSharedQueueErrorCount();
                    }
                    else
                    {
                        _stats.incErrorCount();
                    }
                }
                else
                {
                    _stats.incErrorCount();
                }

#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                if (rc == (int)ITask::RetCode::Exception)
                {
                    std::cerr << "IO task exited with user exception." << std::endl;
                }
                else
                {
                    std::cerr << "IO task exited with error : " << rc << std::endl;
                }
#endif
            }
        }
        catch (std::exception& ex)
        {
            UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Caught exception: " << ex.what() << std::endl;
#endif
        }
        catch (...)
        {
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Caught unknown exception." << std::endl;
#endif
        }
    } //while
}

inline
void IoQueue::enQueue(ITask::Ptr task)
{
    if (!task)
    {
        return; //nothing to do
    }
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_spinlock);
    _stats.incPostedCount();
    if (task->isHighPriority())
    {
        _stats.incHighPriorityCount();
        _queue.emplace_front(std::static_pointer_cast<IoTask>(task));
    }
    else
    {
        _queue.emplace_back(std::static_pointer_cast<IoTask>(task));
    }
    signalEmptyCondition(false);
}

//Must be called from within a locked context
inline
ITask::Ptr IoQueue::deQueue()
{
    if (_queue.empty())
    {
        return nullptr;
    }
    ITask::Ptr task = std::move(_queue.front());
    _queue.pop_front();
    return task;
}

inline
size_t IoQueue::size() const
{
#if (__cplusplus >= 201703L)
    return _queue.size();
#else
    //Until c++17, list::size() takes linear time. It must also be protected, hence making code much slower.
    return _stats.postedCount() -
           _stats.errorCount() - _stats.sharedQueueErrorCount() - //errors
           _stats.completedCount() - _stats.sharedQueueCompletedCount(); //successes
#endif
}

inline
bool IoQueue::empty() const
{
    return _queue.empty() && _isIdle;
}

inline
void IoQueue::terminate()
{
    if (!_terminated.test_and_set())
    {
        _isInterrupted = true;
        _notEmptyCond.notify_all();
        _thread->join();
        _queue.clear();
    }
}

inline
IQueueStatistics& IoQueue::stats()
{
    return _stats;
}

inline
SpinLock& IoQueue::getLock()
{
    return _spinlock;
}

inline
void IoQueue::signalEmptyCondition(bool value)
{
    std::lock_guard<std::mutex> lock(_notEmptyMutex);
    _isEmpty = value;
    if (!value)
    {
        _notEmptyCond.notify_all();
    }
}

inline
ITask::Ptr IoQueue::grabWorkItem()
{
    //========================= LOCKED SCOPE =========================
    SpinLock::Guard lock(_spinlock);
    ITask::Ptr task = deQueue();
    if (!task)
    {
        if (_sharedIoQueue)
        {
            //========================= LOCKED SCOPE (MAIN QUEUE) =========================
            SpinLock::Guard lock(_sharedIoQueue->getLock());
            task = _sharedIoQueue->deQueue();
            if (!task)
            {
                signalEmptyCondition(true);
            }
        }
        else
        {
            //This is the main queue
            signalEmptyCondition(true);
        }
    }
    return task;
}

inline
bool IoQueue::isIdle() const
{
    return _isIdle;
}

}}
