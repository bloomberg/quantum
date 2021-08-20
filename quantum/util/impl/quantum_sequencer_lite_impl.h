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

#include <quantum/util/quantum_drain_guard.h>
#include <quantum/quantum_promise.h>
#include <quantum/quantum_traits.h>
#include <quantum/impl/quantum_stl_impl.h>
#include <stdexcept>

namespace Bloomberg {
namespace quantum {

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::SequencerLite(Dispatcher& dispatcher,
    const typename SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::Configuration& configuration) :
    _dispatcher(dispatcher),
    _drain(false),
    _pendingTaskQueueMap(configuration.getBucketCount(),
              configuration.getHash(),
              configuration.getKeyEqual(),
              configuration.getAllocator()),
    _exceptionCallback(configuration.getExceptionCallback()),
    _taskStats(std::make_shared<SequenceKeyStatisticsWriter>())
{
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::addPendingTask(
    const std::shared_ptr<SequencerLiteTask<SequenceKey>>& task)
{
    bool empty = _universalTaskQueue._tasks.empty();
    _universalTaskQueue._tasks.push_back(task);
    if (not empty)
    {
        ++task->_pendingKeyCount;
    }
    _universalTaskQueue._stats->incrementPostedTaskCount();
    _universalTaskQueue._stats->incrementPendingTaskCount();

    return empty;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::addPendingTask(
    const SequenceKey& key,
    const std::shared_ptr<SequencerLiteTask<SequenceKey>>& task)
{
    typename PendingTaskQueueMap::iterator it = _pendingTaskQueueMap.find(key);
    if (it == _pendingTaskQueueMap.end())
    {
        it = _pendingTaskQueueMap.emplace(key, SequencerLiteKeyData<SequenceKey>()).first;
        // add each universal task to this queue before the task
        for(auto& universalTask : _universalTaskQueue._tasks)
        {
            it->second._tasks.push_back(universalTask);
            ++universalTask->_pendingKeyCount;
        }
    }
    bool empty = it->second._tasks.empty();
    it->second._tasks.push_back(task);
    if (not empty)
    {
        ++task->_pendingKeyCount;
    }
    if (not task->_universal)
    {
        task->_keyData.push_back(&it->second);
        it->second._stats->incrementPostedTaskCount();
        it->second._stats->incrementPendingTaskCount();
    }
    return empty;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
std::shared_ptr<SequencerLiteTask<SequenceKey>>
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::removePending(
    SequencerLiteKeyData<SequenceKey>& entry,
    const std::shared_ptr<SequencerLiteTask<SequenceKey>>& task)
{
    // delete this entry
    if (entry._tasks.empty() or
        entry._tasks.front() != task)
    {
        return std::shared_ptr<SequencerLiteTask<SequenceKey>>();
    }

    entry._tasks.pop_front();
    if (entry._tasks.empty())
    {
        return std::shared_ptr<SequencerLiteTask<SequenceKey>>();
    }
    std::shared_ptr<SequencerLiteTask<SequenceKey>> next = entry._tasks.front();
    // the new front has one less key to wait
    return 0 == --next->_pendingKeyCount
        ? next
        : std::shared_ptr<SequencerLiteTask<SequenceKey>>();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::removePending(
    const std::shared_ptr<SequencerLiteTask<SequenceKey>>& task)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (task->_universal)
    {
        // remove the task from all key queues
        for(auto& item : _pendingTaskQueueMap)
        {
            if (std::shared_ptr<SequencerLiteTask<SequenceKey>> nextTask = removePending(item.second, task))
            {
                scheduleTask(nextTask);
            }
        }

        // remove the task from the universal queue
        if (std::shared_ptr<SequencerLiteTask<SequenceKey>> nextTask = removePending(_universalTaskQueue, task))
        {
            scheduleTask(nextTask);
        }
    }
    else
    {
        // remove the task from its key queues only
        for(SequencerLiteKeyData<SequenceKey>* data : task->_keyData)
        {
            if (std::shared_ptr<SequencerLiteTask<SequenceKey>> nextTask = removePending(*data, task))
            {
                scheduleTask(nextTask);
            }
        }
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::scheduleTask(
    const std::shared_ptr<SequencerLiteTask<SequenceKey>>& task)
{
    // the task is at the head of its queue hence we can schedule it right away
    auto taskWrapper = [this, task](VoidContextPtr ctx)->int
    {
        int rc = -1;
        try
        {
            rc = task->_func(ctx);
        }
        catch(...)
        {
            if (_exceptionCallback)
            {
                _exceptionCallback(std::current_exception(), task->_opaque);
            }
        }

        // remove the task from the pending queues + schedule next tasks
        removePending(task);
        return rc;
    };

    if (task->_universal)
    {
        _universalTaskQueue._stats->decrementPendingTaskCount();
    }
    else
    {
        for(SequencerLiteKeyData<SequenceKey>* keyData : task->_keyData)
        {
            keyData->_stats->decrementPendingTaskCount();
        }
    }
    _taskStats->decrementPendingTaskCount();

    _dispatcher.post(task->_queueId, task->_isHighPriority, std::move(taskWrapper));
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template<class FUNC, class ...ARGS>
std::function<int(VoidContextPtr)> 
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::wrap(
    FUNC&& func, ARGS&&... args)
{
    return [f = func, a = std::make_tuple(std::forward<ARGS>(args)...)](VoidContextPtr ctx) -> int {
        return apply(
                [f, ctx](auto&&... args) { return f(ctx, std::forward<decltype(args)>(args)...); },
                std::move(a));
    };
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args)
{
    enqueueSingle(nullptr, (int)IQueue::QueueId::Any, false, sequenceKey, std::move(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    void* opaque, 
    int queueId, 
    bool isHighPriority, 
    const SequenceKey& sequenceKey, 
    FUNC&& func, 
    ARGS&&... args)
{
    enqueueSingle(opaque, queueId, isHighPriority, sequenceKey, std::move(func), std::forward<ARGS>(args)...);  
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueueSingle(
    void* opaque, 
    int queueId, 
    bool isHighPriority, 
    const SequenceKey& sequenceKey, 
    FUNC&& func, 
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("SequencerLite is disabled");
    }

    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerLiteTask<SequenceKey>>(
        wrap(std::move(func), std::forward<ARGS>(args)...),
        false,
        opaque,
        queueId,
        isHighPriority);

    std::lock_guard<std::mutex> lock(_mutex);

    _taskStats->incrementPostedTaskCount();
    _taskStats->incrementPendingTaskCount();

    if (addPendingTask(sequenceKey, task))
    {
        scheduleTask(task);
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const std::vector<SequenceKey>& sequenceKeys, 
    FUNC&& func, 
    ARGS&&... args)
{
    enqueueMultiple(nullptr, (int)IQueue::QueueId::Any, false, sequenceKeys, std::move(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    enqueueMultiple(opaque, queueId, isHighPriority, sequenceKeys, std::move(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueueMultiple(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("SequencerLite is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerLiteTask<SequenceKey>>(
        wrap(std::move(func), std::forward<ARGS>(args)...),
        false,
        opaque,
        queueId,
        isHighPriority);

    std::lock_guard<std::mutex> lock(_mutex);

    _taskStats->incrementPostedTaskCount();
    _taskStats->incrementPendingTaskCount();

    bool canSchedule = true;
    for(const SequenceKey& sequenceKey : sequenceKeys)
    {
        if (not addPendingTask(sequenceKey, task))
        {
            canSchedule = false;
        }
    }
    if (canSchedule)
    {
        scheduleTask(task);
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAll(FUNC&& func, ARGS&&... args)
{
    enqueueAllImpl(nullptr, (int)IQueue::QueueId::Any, false, std::move(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAll(
    void* opaque, 
    int queueId, 
    bool isHighPriority, 
    FUNC&& func, 
    ARGS&&... args)
{
    enqueueAllImpl(opaque, queueId, isHighPriority, std::move(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAllImpl(
    void* opaque, 
    int queueId, 
    bool isHighPriority, 
    FUNC&& func, 
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("SequencerLite is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerLiteTask<SequenceKey>>(
        wrap(std::move(func), std::forward<ARGS>(args)...),
        true,
        opaque,
        queueId,
        isHighPriority);

    std::lock_guard<std::mutex> lock(_mutex);

    _taskStats->incrementPostedTaskCount();
    _taskStats->incrementPendingTaskCount();

    bool canSchedule = addPendingTask(task);
    for(const auto& pendingItem : _pendingTaskQueueMap)
    {
        if (not addPendingTask(pendingItem.first, task))
        {
            canSchedule = false;
        }
    }
    if (canSchedule)
    {
        scheduleTask(task);
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::trimSequenceKeys()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for(typename PendingTaskQueueMap::iterator it = _pendingTaskQueueMap.begin(); it != _pendingTaskQueueMap.end(); )
    {
        if (it->second._tasks.empty())
            it = _pendingTaskQueueMap.erase(it);
        else
            ++it;
    }
    return _pendingTaskQueueMap.size();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics(const SequenceKey& sequenceKey)
{
    std::lock_guard<std::mutex> lock(_mutex);
    typename PendingTaskQueueMap::const_iterator it = _pendingTaskQueueMap.find(sequenceKey);
    if (it == _pendingTaskQueueMap.end())
        return SequenceKeyStatistics();
    return *it->second._stats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return *_universalTaskQueue._stats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::getTaskStatistics()
{
    return *_taskStats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::getSequenceKeyCount()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _pendingTaskQueueMap.size();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
SequencerLite<SequenceKey, Hash, KeyEqual, Allocator>::drain(std::chrono::milliseconds timeout,
                                                             bool isFinal)
{
    std::shared_ptr<Promise<int>> promise = std::make_shared<Promise<int>>();
    ThreadFuturePtr<int> future = promise->getIThreadFuture();

    //enqueue a universal task and wait
    enqueueAll([promise](VoidContextPtr ctx)->int{
        return promise->set(ctx, 0);
    });

    DrainGuard guard(_drain, !isFinal);
    return future->waitFor(timeout) == std::future_status::ready;
}

}}
