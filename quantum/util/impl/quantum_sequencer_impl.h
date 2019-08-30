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
#include <stdexcept>

namespace Bloomberg {
namespace quantum {
    
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Sequencer(Dispatcher& dispatcher,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Configuration& configuration) :
    _dispatcher(dispatcher),
    _drain(false),
    _controllerQueueId(configuration.getControlQueueId()),
    _universalContext(),
    _contexts(configuration.getBucketCount(),
              configuration.getHash(),
              configuration.getKeyEqual(),
              configuration.getAllocator()),
    _exceptionCallback(configuration.getExceptionCallback()),
    _taskStats(std::make_shared<SequenceKeyStatisticsWriter>())
{
    if (_controllerQueueId <= (int)IQueue::QueueId::Any || _controllerQueueId >= _dispatcher.getNumCoroutineThreads())
    {
        throw std::out_of_range("Allowed range is 0 <= controllerQueueId < _dispatcher.getNumCoroutineThreads()");
    }
}
    
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const SequenceKey& sequenceKey,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      singleSequenceKeyTaskScheduler<FUNC, ARGS...>,
                      nullptr,
                      (int)IQueue::QueueId::Any,
                      false,
                      *this,
                      SequenceKey(sequenceKey),
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
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
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      singleSequenceKeyTaskScheduler<FUNC, ARGS...>,
                      std::move(opaque),
                      std::move(queueId),
                      std::move(isHighPriority),
                      *this,
                      SequenceKey(sequenceKey),
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
}
 
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueue(
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      multiSequenceKeyTaskScheduler<FUNC, ARGS...>,
                      nullptr,
                      (int)IQueue::QueueId::Any,
                      false,
                      *this,
                      std::vector<SequenceKey>(sequenceKeys),
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
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
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      multiSequenceKeyTaskScheduler<FUNC, ARGS...>,
                      std::move(opaque),
                      std::move(queueId),
                      std::move(isHighPriority),
                      *this,
                      std::vector<SequenceKey>(sequenceKeys),
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
}
 
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::enqueueAll(FUNC&& func, ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      universalTaskScheduler<FUNC, ARGS...>,
                      nullptr,
                      (int)IQueue::QueueId::Any,
                      false,
                      *this,
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
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
    if (_drain)
    {
        throw std::runtime_error("Sequencer is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    _dispatcher.post(_controllerQueueId,
                      false,
                      universalTaskScheduler<FUNC, ARGS...>,
                      std::move(opaque),
                      std::move(queueId),
                      std::move(isHighPriority),
                      *this,
                      std::forward<FUNC>(func),
                      std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::trimSequenceKeys()
{
    auto trimFunc = [this](CoroContextPtr<size_t> ctx)->int
    {
        for (auto it = _contexts.begin(); it != _contexts.end();)
        {
            auto trimIt = it++;
            if (canTrimContext(ctx, trimIt->second._context))
            {
                _contexts.erase(trimIt);
            }
        }
        return ctx->set(_contexts.size());
    };
    return _dispatcher.post(_controllerQueueId, true, std::move(trimFunc))->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics(const SequenceKey& sequenceKey)
{
    auto statsFunc = [this, sequenceKey](CoroContextPtr<SequenceKeyStatistics> ctx)->int
    {
        typename ContextMap::iterator ctxIt = _contexts.find(sequenceKey);
        if (ctxIt == _contexts.end())
        {
            return ctx->set(SequenceKeyStatistics());
        }
        return ctx->set(SequenceKeyStatistics(*ctxIt->second._stats));
    };
    return _dispatcher.post(_controllerQueueId, true, std::move(statsFunc))->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequenceKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics()
{
    return *_universalContext._stats;
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
    auto statsFunc = [this](CoroContextPtr<size_t> ctx)->int
    {
        return ctx->set(_contexts.size());
    };
    return _dispatcher.post(_controllerQueueId, true, std::move(statsFunc))->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::waitForTwoDependents(
        VoidContextPtr ctx,
        void* opaque,
        Sequencer& sequencer,
        SequenceKeyData&& dependent,
        SequenceKeyData&& universalDependent,
        FUNC&& func,
        ARGS&&... args)
{
    // wait until all the dependents are done
    if (dependent._context)
    {
        dependent._context->wait(ctx);
    }
    if (universalDependent._context)
    {
        universalDependent._context->wait(ctx);
    }
    int rc = callPosted(ctx, opaque, sequencer, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
    // update task stats
    dependent._stats->decrementPendingTaskCount();
    sequencer._taskStats->decrementPendingTaskCount();
    return rc;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::waitForDependents(
        VoidContextPtr ctx,
        void* opaque,
        Sequencer& sequencer,
        std::vector<SequenceKeyData>&& dependents,
        SequenceKeyData&& universalDependent,
        FUNC&& func,
        ARGS&&... args)
{
    // wait until all the dependents are done
    for (const auto& dependent : dependents)
    {
        if (dependent._context)
        {
            dependent._context->wait(ctx);
        }
    }
    //wait until the universal dependent is done
    if (universalDependent._context)
    {
        universalDependent._context->wait(ctx);
    }
    int rc = callPosted(ctx, opaque, sequencer, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
    // update task stats
    for (const auto& dependent : dependents)
    {
        dependent._stats->decrementPendingTaskCount();
    }
    sequencer._taskStats->decrementPendingTaskCount();
    return rc;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::waitForUniversalDependent(
        VoidContextPtr ctx,
        void* opaque,
        Sequencer& sequencer,
        std::vector<SequenceKeyData>&& dependents,
        SequenceKeyData&& universalDependent,
        FUNC&& func,
        ARGS&&... args)
{
    // wait until all the dependents are done
    for (const auto& dependent : dependents)
    {
        if (dependent._context)
        {
            dependent._context->wait(ctx);
        }
    }
    //wait until the universal dependent is done
    if (universalDependent._context)
    {
        universalDependent._context->wait(ctx);
    }
    int rc = callPosted(ctx, opaque, sequencer, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
    // update task stats
    universalDependent._stats->decrementPendingTaskCount();
    sequencer._taskStats->decrementPendingTaskCount();
    return rc;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::singleSequenceKeyTaskScheduler(
        VoidContextPtr ctx,
        void* opaque,
        int queueId,
        bool isHighPriority,
        Sequencer& sequencer,
        SequenceKey&& sequenceKey,
        FUNC&& func,
        ARGS&&... args)
{
    // find the dependent
    typename ContextMap::iterator contextIt = sequencer._contexts.find(sequenceKey);
    if (contextIt == sequencer._contexts.end())
    {
        contextIt = sequencer._contexts.emplace(sequenceKey, SequenceKeyData()).first;
    }
    // update stats
    contextIt->second._stats->incrementPostedTaskCount();
    contextIt->second._stats->incrementPendingTaskCount();
    // update task stats
    sequencer._taskStats->incrementPostedTaskCount();
    sequencer._taskStats->incrementPendingTaskCount();
    
    // save the context as the last for this sequenceKey
    contextIt->second._context = ctx->post(
            std::move(queueId),
            std::move(isHighPriority),
            waitForTwoDependents<FUNC, ARGS...>,
            std::move(opaque),
            sequencer,
            SequenceKeyData(contextIt->second),
            SequenceKeyData(sequencer._universalContext),
            std::forward<FUNC>(func),
            std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::multiSequenceKeyTaskScheduler(
    VoidContextPtr ctx,
    void* opaque,
    int queueId,
    bool isHighPriority,
    Sequencer& sequencer,
    std::vector<SequenceKey>&& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    // construct the dependent collection
    std::vector<SequenceKeyData> dependents;
    dependents.reserve(sequenceKeys.size());
    dependents.push_back(sequencer._universalContext);
    for (const SequenceKey& sequenceKey : sequenceKeys)
    {
        auto taskIt = sequencer._contexts.find(sequenceKey);
        if (taskIt != sequencer._contexts.end())
        {
            // add the dependent and increment stats
            taskIt->second._stats->incrementPostedTaskCount();
            taskIt->second._stats->incrementPendingTaskCount();
            dependents.emplace_back(taskIt->second);
        }
    }
    // update task stats
    sequencer._taskStats->incrementPostedTaskCount();
    sequencer._taskStats->incrementPendingTaskCount();
    
    ICoroContextBasePtr newCtx = ctx->post(
            std::move(queueId),
            std::move(isHighPriority),
            waitForDependents<FUNC, ARGS...>,
            std::move(opaque),
            sequencer,
            std::move(dependents),
            SequenceKeyData(sequencer._universalContext),
            std::forward<FUNC>(func),
            std::forward<ARGS>(args)...);
    
    // save the context as the last for each sequenceKey
    for (const SequenceKey& sequenceKey : sequenceKeys)
    {
        sequencer._contexts[sequenceKey]._context = newCtx;
    }
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::universalTaskScheduler(
    VoidContextPtr ctx,
    void* opaque,
    int queueId,
    bool isHighPriority,
    Sequencer& sequencer,
    FUNC&& func,
    ARGS&&... args)
{
    // construct the dependent collection
    std::vector<SequenceKeyData> dependents;
    dependents.reserve(sequencer._contexts.size());
    for (auto ctxIt = sequencer._contexts.begin(); ctxIt != sequencer._contexts.end(); ++ctxIt)
    {
        // check if the context still has a pending task
        if (isPendingContext(ctx, ctxIt->second._context))
        {
            // we will need to wait on this context to finish its current running task
            dependents.emplace_back(ctxIt->second);
        }
    }
    // update the universal stats only
    sequencer._universalContext._stats->incrementPostedTaskCount();
    sequencer._universalContext._stats->incrementPendingTaskCount();
    // update task stats
    sequencer._taskStats->incrementPostedTaskCount();
    sequencer._taskStats->incrementPendingTaskCount();

    // post the task and save the context as the last for the universal sequenceKey
    sequencer._universalContext._context = ctx->post(
            std::move(queueId),
            std::move(isHighPriority),
            waitForUniversalDependent<FUNC, ARGS...>,
            std::move(opaque),
            sequencer,
            std::move(dependents),
            SequenceKeyData(sequencer._universalContext),
            std::forward<FUNC>(func),
            std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::callPosted(
        VoidContextPtr ctx,
        void* opaque,
        const Sequencer& sequencer,
        FUNC&& func,
        ARGS&&... args)
{
    // make sure the final action is eventually called
    try
    {
        std::forward<FUNC>(func)(ctx, std::forward<ARGS>(args)...);
        return ctx->set(Void{});
    }
    catch(std::exception& ex)
    {
        if (sequencer._exceptionCallback)
        {
            sequencer._exceptionCallback(std::current_exception(), opaque);
        }
    }
    catch(...)
    {
        if (sequencer._exceptionCallback)
        {
            sequencer._exceptionCallback(std::current_exception(), opaque);
        }
    }
    return -1; //error
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::canTrimContext(const ICoroContextBasePtr& ctx,
                                                                  const ICoroContextBasePtr& ctxToValidate)
{
    return !ctxToValidate || !ctxToValidate->valid() ||
           ctxToValidate->waitFor(ctx, std::chrono::milliseconds(0)) == std::future_status::ready;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::isPendingContext(const ICoroContextBasePtr& ctx,
                                                                    const ICoroContextBasePtr& ctxToValidate)
{
    return ctxToValidate && ctxToValidate->valid() &&
           ctxToValidate->waitFor(ctx, std::chrono::milliseconds(0)) == std::future_status::timeout;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::drain(std::chrono::milliseconds timeout,
                                                         bool isFinal)
{
    std::shared_ptr<Promise<int>> promise = std::make_shared<Promise<int>>();
    ThreadFuturePtr<int> future = promise->getIThreadFuture();
    
    //enqueue a universal task and wait
    enqueueAll([promise](VoidContextPtr)->int{
        return promise->set(0);
    });
    
    DrainGuard guard(_drain, !isFinal);
    if (timeout == std::chrono::milliseconds::zero())
    {
        future->wait();
    }
    else
    {
        future->waitFor(timeout);
    }
}


}}
