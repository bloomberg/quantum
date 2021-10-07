/*
** Copyright 2021 Bloomberg Finance L.P.
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
#include <quantum/quantum_local.h>
#include <quantum/quantum_promise.h>
#include <quantum/quantum_traits.h>
#include <quantum/impl/quantum_stl_impl.h>
#include <stdexcept>
#include <functional>

namespace Bloomberg {
namespace quantum {
namespace experimental {

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Sequencer(Dispatcher& dispatcher,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Configuration& configuration) :
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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::addPendingTask(
    const std::shared_ptr<SequencerTask<SequenceKey>>& task)
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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::addPendingTask(
    const SequenceKey& key,
    const std::shared_ptr<SequencerTask<SequenceKey>>& task)
{
    typename PendingTaskQueueMap::iterator it = _pendingTaskQueueMap.find(key);
    if (it == _pendingTaskQueueMap.end())
    {
        it = _pendingTaskQueueMap.emplace(key, SequencerKeyData<SequenceKey>()).first;
        // add each universal task to this queue before the task
        bool first = true;
        for(auto& universalTask : _universalTaskQueue._tasks)
        {
            it->second._tasks.push_back(universalTask);
            // increment the pending count of the universal task unless it's first in the queue
            if (not first)
            {
                ++universalTask->_pendingKeyCount;
            }
            first = false;
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
std::shared_ptr<SequencerTask<SequenceKey>>
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::removeCompleted(
    SequencerKeyData<SequenceKey>& entry,
    const std::shared_ptr<SequencerTask<SequenceKey>>& task)
{
    // Regular tasks tasks:
    // * entry._tasks.empty() means that there's a bug somewhere: when we intend to delete a pending task,
    // the task queue must not be empty
    // * entry._tasks.front() != task means that there's a bug somewhere:
    // only completed tasks are removed from the queue, and such tasks must sit at the head of the queue
    // Universal tasks: because we do not track the queues where universal tasks are enqueued,
    // entry._tasks.empty() or entry._tasks.front() != task just means that the task was never enqueued
    // to this queue.
    if (entry._tasks.empty() or
        entry._tasks.front() != task)
    {
        return nullptr;
    }

    entry._tasks.pop_front();
    if (entry._tasks.empty())
    {
        // no more tasks in the queue
        return nullptr;
    }
    std::shared_ptr<SequencerTask<SequenceKey>> next = entry._tasks.front();
    // decrementing _pendingKeyCount because the task next is now the head of the queue.
    // As a result, the next task has one less pending dependent to wait for
    return 0 == --next->_pendingKeyCount
        ? next
        : nullptr;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::removeCompletedAndScheduleNext(
    VoidContextPtr ctx,
    const std::shared_ptr<SequencerTask<SequenceKey>>& task)
{
    Mutex::Guard lock(ctx, _mutex);
    if (task->_universal)
    {
        // remove the task from all key queues
        for(auto& item : _pendingTaskQueueMap)
        {
            if (auto nextTask = removeCompleted(item.second, task))
            {
                scheduleTask(nextTask);
            }
        }

        // remove the task from the universal queue
        if (auto nextTask = removeCompleted(_universalTaskQueue, task))
        {
            scheduleTask(nextTask);
        }
    }
    else
    {
        // remove the task from its key queues only
        for(SequencerKeyData<SequenceKey>* data : task->_keyData)
        {
            if (auto nextTask = removeCompleted(*data, task))
            {
                scheduleTask(nextTask);
            }
        }
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::executePending(
    VoidContextPtr ctx,
    Sequencer* sequencer,
    std::shared_ptr<SequencerTask<SequenceKey>> task)
{
    // passing Sequencer pointer into the static executePending instead of
    // making executePending non-static because executePending is passed to
    // dispatcher as a coroutine function, but dispatcher.post(...) doesn't support
    // results of std::bind(...)
    int rc = -1;
    try
    {
        rc = task->_func(ctx);
    }
    catch(const boost::coroutines2::detail::forced_unwind&)
    {
        // quantum context switch
        throw;
    }
    catch(...)
    {
        if (sequencer->_exceptionCallback)
        {
            sequencer->_exceptionCallback(std::current_exception(), task->_opaque);
        }
    }

    // remove the task from the pending queues + schedule next tasks
    sequencer->removeCompletedAndScheduleNext(ctx, task);
    return rc;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::scheduleTask(
    const std::shared_ptr<SequencerTask<SequenceKey>>& task)
{
    if (task->_universal)
    {
        _universalTaskQueue._stats->decrementPendingTaskCount();
    }
    else
    {
        for(SequencerKeyData<SequenceKey>* keyData : task->_keyData)
        {
            keyData->_stats->decrementPendingTaskCount();
        }
    }
    _taskStats->decrementPendingTaskCount();

    _dispatcher.post(
        task->_queueId,
        task->_isHighPriority,
        &Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::executePending,
        this,
        std::shared_ptr<SequencerTask<SequenceKey>>(task));
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args)
{
    enqueueSingle(nullptr, (int)IQueue::QueueId::Any, false, sequenceKey, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const SequenceKey& sequenceKey,
    FUNC&& func,
    ARGS&&... args)
{
    enqueueSingle(opaque, queueId, isHighPriority, sequenceKey, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueSingle(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const SequenceKey& sequenceKey,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }

    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerTask<SequenceKey>>(
        makeCapture<int>(std::forward<FUNC>(func), std::forward<ARGS>(args)...),
        false,
        opaque,
        queueId,
        isHighPriority);

    Mutex::Guard lock(local::context(), _mutex);

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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    enqueueMultiple(nullptr, (int)IQueue::QueueId::Any, false, sequenceKeys, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    enqueueMultiple(opaque, queueId, isHighPriority, sequenceKeys, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueMultiple(
    void* opaque,
    int queueId,
    bool isHighPriority,
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerTask<SequenceKey>>(
        makeCapture<int>(std::forward<FUNC>(func), std::forward<ARGS>(args)...),
        false,
        opaque,
        queueId,
        isHighPriority);

    Mutex::Guard lock(local::context(), _mutex);

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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAll(FUNC&& func, ARGS&&... args)
{
    enqueueAllImpl(nullptr, (int)IQueue::QueueId::Any, false, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAll(
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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAllImpl(
    void* opaque,
    int queueId,
    bool isHighPriority,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::out_of_range(std::string{"Invalid IO queue id: "} + std::to_string(queueId));
    }

    auto task = std::make_shared<SequencerTask<SequenceKey>>(
        makeCapture<int>(std::forward<FUNC>(func), std::forward<ARGS>(args)...),
        true,
        opaque,
        queueId,
        isHighPriority);

    Mutex::Guard lock(local::context(), _mutex);

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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::trimSequenceKeys()
{
    Mutex::Guard lock(local::context(), _mutex);
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
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics(const SequenceKey& sequenceKey)
{
    Mutex::Guard lock(local::context(), _mutex);
    typename PendingTaskQueueMap::const_iterator it = _pendingTaskQueueMap.find(sequenceKey);
    if (it == _pendingTaskQueueMap.end())
        return SequenceKeyStatistics();
    return *it->second._stats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics()
{
    Mutex::Guard lock(local::context(), _mutex);
    return *_universalTaskQueue._stats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getTaskStatistics()
{
    return *_taskStats;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getSequenceKeyCount()
{
    Mutex::Guard lock(local::context(), _mutex);
    return _pendingTaskQueueMap.size();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::drain(std::chrono::milliseconds timeout,
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

}}}
