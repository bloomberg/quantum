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
DispatcherCore::DispatcherCore(const Configuration& config) :
    _sharedCoroAnyQueue(config.getCoroutineSharingForAny() ? std::make_shared<TaskQueue>(config, nullptr): nullptr),
    _sharedIoQueues((config.getNumIoThreads() <= 0) ? 1 : config.getNumIoThreads(), IoQueue(config, nullptr)),
    _ioQueues((config.getNumIoThreads() <= 0) ? 1 : config.getNumIoThreads(), IoQueue(config, &_sharedIoQueues)),
    _loadBalanceSharedIoQueues(false),
    _terminated(false)
{
    const int coroCount = (config.getNumCoroutineThreads() == -1) ? std::thread::hardware_concurrency() :
        (config.getNumCoroutineThreads() == 0) ? 1 : config.getNumCoroutineThreads();

    _coroQueueIdRangeForAny = std::make_pair(0, coroCount-1);
    
    const auto& coroQueueIdRangeForAny = config.getCoroQueueIdRangeForAny();
    // set the range to the default if the configured one is invalid or empty
    if (coroQueueIdRangeForAny.first <= coroQueueIdRangeForAny.second &&
        coroQueueIdRangeForAny.first >= 0 &&
        coroQueueIdRangeForAny.second < (int)coroCount)
    {
        _coroQueueIdRangeForAny = coroQueueIdRangeForAny;
    }
    
    // set thread name for shared queue
    if (_sharedCoroAnyQueue) {
        IQueue::setThreadName(IQueue::QueueType::Coro,
                              _sharedCoroAnyQueue->getThread()->native_handle(),
                              0,
                              true,
                              false);
    }

    // start the coro threads
    _coroQueues.reserve(coroCount);
    for (int coroId = 0; coroId < coroCount; ++coroId)
    {
        bool hasSharedQueue = (coroId >= _coroQueueIdRangeForAny.first &&
                               coroId <= _coroQueueIdRangeForAny.second);
        _coroQueues.emplace_back(config, hasSharedQueue ? _sharedCoroAnyQueue : nullptr);
        // set thread name for coro queues
        IQueue::setThreadName(IQueue::QueueType::Coro,
                              _coroQueues.back().getThread()->native_handle(),
                              coroId,
                              false,
                              hasSharedQueue);
    }
    
    // set thread names for io queues
    for (size_t ioId = 0; ioId < _ioQueues.size(); ++ioId)
    {
        IQueue::setThreadName(IQueue::QueueType::IO,
                              _ioQueues[ioId].getThread()->native_handle(),
                              ioId,
                              false,
                              false);
    }
    
    // pin to cores
    if (config.getPinCoroutineThreadsToCores())
    {
        unsigned int cores = std::thread::hardware_concurrency();
        for (size_t i = 0; i < _coroQueues.size(); ++i)
        {
            _coroQueues[i].pinToCore(i%cores);
        }
    }
}

inline
DispatcherCore::~DispatcherCore()
{
    terminate();
}

inline
void DispatcherCore::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        for (auto&& queue : _coroQueues)
        {
            queue.terminate();
        }
        if (_sharedCoroAnyQueue)
        {
            _sharedCoroAnyQueue->terminate();
        }
        for (auto&& queue : _ioQueues)
        {
            queue.terminate();
        }
        for (auto&& queue : _sharedIoQueues)
        {
            queue.terminate();
        }
    }
}

inline
size_t DispatcherCore::size(IQueue::QueueType type,
                            int queueId) const
{
    if (type == IQueue::QueueType::All)
    {
        if (queueId != (int)IQueue::QueueId::All)
        {
            throw std::runtime_error("Cannot specify queue id");
        }
        return coroSize((int)IQueue::QueueId::All) + ioSize((int)IQueue::QueueId::All);
    }
    else if (type == IQueue::QueueType::Coro)
    {
        return coroSize(queueId);
    }
    return ioSize(queueId);
}

inline
bool DispatcherCore::empty(IQueue::QueueType type,
                           int queueId) const
{
    if (type == IQueue::QueueType::All)
    {
        if (queueId != (int)IQueue::QueueId::All)
        {
            throw std::runtime_error("Cannot specify queue id");
        }
        return coroEmpty((int)IQueue::QueueId::All) && ioEmpty((int)IQueue::QueueId::All);
    }
    else if (type == IQueue::QueueType::Coro)
    {
        return coroEmpty(queueId);
    }
    return ioEmpty(queueId);
}

inline
size_t DispatcherCore::coroSize(int queueId) const
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        size_t size = 0;
        for (auto&& queue : _coroQueues)
        {
            size += queue.size();
        }
        if (_sharedCoroAnyQueue)
        {
            size += _sharedCoroAnyQueue->size();
        }
        return size;
    }
    else if ((queueId >= (int)_coroQueues.size()) || (queueId < 0))
    {
        throw std::runtime_error("Invalid coroutine queue id");
    }
    return _coroQueues.at(queueId).size();
}

inline
bool DispatcherCore::coroEmpty(int queueId) const
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        for (auto&& queue : _coroQueues)
        {
            if (!queue.empty()) return false;
        }
        if (_sharedCoroAnyQueue && !_sharedCoroAnyQueue->empty())
        {
            return false;
        }
        return true;
    }
    else if ((queueId >= (int)_coroQueues.size()) || (queueId < 0))
    {
        throw std::runtime_error("Invalid coroutine queue id");
    }
    return _coroQueues.at(queueId).empty();
}

inline
size_t DispatcherCore::ioSize(int queueId) const
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        size_t size = 0;
        for (auto&& queue : _ioQueues)
        {
            size += queue.size();
        }
        for (auto&& queue : _sharedIoQueues)
        {
            size += queue.size();
        }
        return size;
    }
    else if (queueId == (int)IQueue::QueueId::Any)
    {
        size_t size = 0;
        for (auto&& queue : _sharedIoQueues)
        {
            size += queue.size();
        }
        return size;
    }
    return _ioQueues.at(queueId).size();
}

inline
bool DispatcherCore::ioEmpty(int queueId) const
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        for (auto&& queue : _sharedIoQueues)
        {
            if (!queue.empty())
            {
                return false;
            }
        }
        for (auto&& queue : _ioQueues)
        {
            if (!queue.empty())
            {
                return false;
            }
        }
        return true;
    }
    else if (queueId == (int)IQueue::QueueId::Any)
    {
        for (auto&& queue : _sharedIoQueues)
        {
            if (!queue.empty())
            {
                return false;
            }
        }
        return true;
    }
    return _ioQueues.at(queueId).empty();
}

inline
QueueStatistics DispatcherCore::stats(IQueue::QueueType type, int queueId)
{
    if (type == IQueue::QueueType::All)
    {
        if (queueId != (int)IQueue::QueueId::All)
        {
            throw std::runtime_error("Cannot specify queue id");
        }
        return coroStats((int)IQueue::QueueId::All) + ioStats((int)IQueue::QueueId::All);
    }
    else if (type == IQueue::QueueType::Coro)
    {
        return coroStats(queueId);
    }
    return ioStats(queueId);
}

inline
QueueStatistics DispatcherCore::coroStats(int queueId)
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        QueueStatistics stats;
        for (auto&& queue : _coroQueues)
        {
            stats += queue.stats();
        }
        if (_sharedCoroAnyQueue)
        {
            stats += _sharedCoroAnyQueue->stats();
        }
        return stats;
    }
    else
    {
        if ((queueId >= (int)_coroQueues.size()) || (queueId < 0))
        {
            throw std::runtime_error("Invalid coroutine queue id");
        }
        return static_cast<const QueueStatistics&>(_coroQueues.at(queueId).stats());
    }
}

inline
QueueStatistics DispatcherCore::ioStats(int queueId)
{
    if (queueId == (int)IQueue::QueueId::All)
    {
        QueueStatistics stats;
        for (auto&& queue : _ioQueues)
        {
            stats += queue.stats();
        }
        for (auto&& queue : _sharedIoQueues)
        {
            stats += queue.stats();
        }
        return stats;
    }
    else if (queueId == (int)IQueue::QueueId::Any)
    {
        QueueStatistics stats;
        for (auto&& queue : _sharedIoQueues)
        {
            stats += queue.stats();
        }
        return stats;
    }
    else
    {
        if ((queueId >= (int)_ioQueues.size()) || (queueId < 0))
        {
            throw std::runtime_error("Invalid IO queue id");
        }
        return static_cast<const QueueStatistics&>(_ioQueues.at(queueId).stats());
    }
}

inline
void DispatcherCore::resetStats()
{
    for (auto&& queue : _coroQueues)
    {
        queue.stats().reset();
    }
    if (_sharedCoroAnyQueue)
    {
        _sharedCoroAnyQueue->stats().reset();
    }
    for (auto&& queue : _sharedIoQueues)
    {
        queue.stats().reset();
    }
    for (auto&& queue : _ioQueues)
    {
        queue.stats().reset();
    }
}

inline
void DispatcherCore::post(Task::Ptr task)
{
    if (!task)
    {
        return;
    }
    
    if (task->getQueueId() == (int)IQueue::QueueId::Any)
    {
        if (_sharedCoroAnyQueue)
        {
            _sharedCoroAnyQueue->enqueue(task);
            return;
        }
        
        size_t index = 0;
        
        //Insert into the shortest queue or the first empty queue found
        size_t numTasks = std::numeric_limits<size_t>::max();
        for (size_t i = (size_t)_coroQueueIdRangeForAny.first; i <= (size_t)_coroQueueIdRangeForAny.second; ++i)
        {
            size_t queueSize = _coroQueues[i].size();
            if (queueSize < numTasks)
            {
                numTasks = queueSize;
                index = i;
            }
            if (numTasks == 0)
            {
                break; //reached an empty queue
            }
        }
        
        task->setQueueId(index); //overwrite the queueId with the selected one
    }
    else
    {
        if (task->getQueueId() >= (int)_coroQueues.size())
        {
            throw std::runtime_error("Queue id out of bounds");
        }
    }
    
    _coroQueues.at(task->getQueueId()).enqueue(task);
}

inline
void DispatcherCore::postAsyncIo(IoTask::Ptr task)
{
    if (!task)
    {
        return;
    }
    
    if (task->getQueueId() == (int)IQueue::QueueId::Any)
    {
        if (_loadBalanceSharedIoQueues)
        {
            static size_t index = 0;
            //loop until we can find an queue that won't block
            while (1) {
                if (_sharedIoQueues.at(++index % _sharedIoQueues.size()).tryEnqueue(task)) {
                    break;
                }
            }
        }
        else
        {
            //insert the task into the shared queue
            _sharedIoQueues[0].enqueue(task);
        
            //Signal all threads there is work to do
            for (auto&& queue : _ioQueues)
            {
                queue.signalEmptyCondition(false);
            }
        }
    }
    else
    {
        if (task->getQueueId() >= (int)_ioQueues.size())
        {
            throw std::runtime_error("Queue id out of bounds");
        }
        
        //Run on specific queue
        _ioQueues.at(task->getQueueId()).enqueue(task);
    }
}

inline
int DispatcherCore::getNumCoroutineThreads() const
{
    return _coroQueues.size();
}

inline
int DispatcherCore::getNumIoThreads() const
{
    return _ioQueues.size();
}

inline
const std::pair<int, int>& DispatcherCore::getCoroQueueIdRangeForAny() const
{
    return _coroQueueIdRangeForAny;
}
 
}}
