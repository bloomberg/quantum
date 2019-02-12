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

#include <stdexcept>

namespace Bloomberg {
namespace quantum {
    
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Sequencer(Dispatcher& dispatcher,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::Configuration& configuration) :
    _dispatcher(dispatcher),
    _controllerQueueId(configuration.getControlQueueId()),
    _universalContext(),
    _contexts(configuration.getBucketCount(),
              configuration.getHash(),
              configuration.getKeyEqual(),
              configuration.getAllocator()),
    _exceptionCallback(configuration.getExceptionCallback())
{
    if (_controllerQueueId <= (int)IQueue::QueueId::Any || _controllerQueueId >= _dispatcher.getNumCoroutineThreads())
    {
        throw std::out_of_range("Allowed range is 0 <= controllerQueueId < _dispatcher.getNumCoroutineThreads()");
    }
}
    
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::post(
    const SequenceKey& sequenceKey,
    FUNC&& func,
    ARGS&&... args)
{
    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &singleSequenceKeyTaskScheduler<FUNC, ARGS...>,
                          (int)IQueue::QueueId::Any,
                          false,
                          nullptr,
                          _exceptionCallback,
                          SequenceKey(sequenceKey),
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::postEx(
    int queueId,
    bool isHighPriority,
    void* opaque,
    const SequenceKey& sequenceKey,
    FUNC&& func,
    ARGS&&... args)
{
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }

    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &singleSequenceKeyTaskScheduler<FUNC, ARGS...>,
                          queueId,
                          isHighPriority,
                          std::move(opaque),
                          _exceptionCallback,
                          SequenceKey(sequenceKey),
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}
 
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::post(
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &multiSequenceKeyTaskScheduler<FUNC, ARGS...>,
                          (int)IQueue::QueueId::Any,
                          false,
                          nullptr,
                          _exceptionCallback,
                          std::vector<SequenceKey>(sequenceKeys),
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::postEx(
    int queueId,
    bool isHighPriority,
    void* opaque,
    const std::vector<SequenceKey>& sequenceKeys,
    FUNC&& func,
    ARGS&&... args)
{
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &multiSequenceKeyTaskScheduler<FUNC, ARGS...>,
                          queueId,
                          isHighPriority,
                          std::move(opaque),
                          _exceptionCallback,
                          std::vector<SequenceKey>(sequenceKeys),
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}
 
template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::postAll(FUNC&& func, ARGS&&... args)
{
    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &universalTaskScheduler<FUNC, ARGS...>,
                          (int)IQueue::QueueId::Any,
                          false,
                          nullptr,
                          _exceptionCallback,
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::postAllEx(
    int queueId,
    bool isHighPriority,
    void* opaque,
    FUNC&& func,
    ARGS&&... args)
{
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    _dispatcher.post<int>(_controllerQueueId,
                          false,
                          &universalTaskScheduler<FUNC, ARGS...>,
                          queueId,
                          isHighPriority,
                          std::move(opaque),
                          _exceptionCallback,
                          _contexts,
                          _universalContext,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::trimSequenceKeys()
{
    auto trimFunc = [this](CoroContextPtr<size_t> ctx)->int
    {
        for (typename ContextMap::iterator it = _contexts.begin(); it != _contexts.end();)
        {
            typename ContextMap::iterator thisIt = it++;
            if (canTrimContext(ctx, thisIt->second.context))
            {
                _contexts.erase(thisIt);
            }
        }
        return ctx->set(_contexts.size());
    };
    return _dispatcher.post<size_t>(_controllerQueueId, true, trimFunc)->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequencerKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics(const SequenceKey& sequenceKey)
{
    auto statsFunc = [this, sequenceKey](CoroContextPtr<SequencerKeyStatistics> ctx)->int
    {
        SequencerKeyStatisticsWriter stats;
        typename ContextMap::iterator ctxIt = _contexts.find(sequenceKey);
        if (ctxIt == _contexts.end())
        {
            return ctx->set(SequencerKeyStatistics());
        }
        return ctx->set(SequencerKeyStatistics(ctxIt->second.stats));
    };
    return _dispatcher.post<SequencerKeyStatistics>(_controllerQueueId, true, statsFunc)->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequencerKeyStatistics
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getStatistics()
{
    auto statsFunc = [this](CoroContextPtr<SequencerKeyStatistics> ctx)->int
    {
        return ctx->set(SequencerKeyStatistics(_universalContext.stats));
    };
    return _dispatcher.post<SequencerKeyStatistics>(_controllerQueueId, true, statsFunc)->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::getSequenceKeyCount()
{
    auto statsFunc = [this](CoroContextPtr<size_t> ctx)->int
    {
        return ctx->set(_contexts.size());
    };
    return _dispatcher.post<size_t>(_controllerQueueId, true, statsFunc)->get();
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::waitForTwoDependents(
    CoroContextPtr<int> ctx,
    ICoroContextBasePtr dependent,
    ICoroContextBasePtr universalContext,
    SequencerKeyStatisticsWriter& stats,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    FUNC&& func,
    ARGS&&... args)
{
    // wait until all the dependents are done
    if (dependent)
    {
        dependent->wait(ctx);
    }
    if (universalContext)
    {
        universalContext->wait(ctx);
    }

    // statistics update action to be called after the func, even if func throws
    auto updateStatsAction = [&stats]()
    {
        // update the dependent's stats only, because the task is associated with the non-universal key
        stats.decrementPendingTaskCount();
    };
    callPosted(ctx, 
               updateStatsAction, 
               opaque, 
               exceptionCallback, 
               std::forward<FUNC>(func), 
               std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::waitForDependents(
    CoroContextPtr<int> ctx,
    std::vector<std::pair<ICoroContextBasePtr, SequencerKeyStatisticsWriter*>>&& dependents,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    FUNC&& func,
    ARGS&&... args)
{
    // wait until all the dependents are done
    for (const auto& dependent : dependents)
    {
        if (dependent.first)
        {
            dependent.first->wait(ctx);
        }
    }

    // statistics update action to be called after the func, even if func throws
    auto updateStatsAction = [&dependents]()
    {
        // update the pending statistics for the passed tasks
        for (auto& dependent : dependents)
        {
            if (dependent.second)
            {
                dependent.second->decrementPendingTaskCount();
            }
        }
    };
    callPosted(ctx, 
               updateStatsAction, 
               opaque, 
               exceptionCallback, 
               std::forward<FUNC>(func), 
               std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::singleSequenceKeyTaskScheduler(
    CoroContextPtr<int> ctx,
    int queueId,
    bool isHighPriority,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    SequenceKey&& sequenceKey,
    ContextMap& contexts,
    SequencerKeyData& universalContext,
    FUNC&& func,
    ARGS&&... args)
{
    // find the dependent
    typename ContextMap::iterator contextIt = contexts.find(sequenceKey);
    if (contextIt == contexts.end())
    {
        contextIt = contexts.emplace(sequenceKey, SequencerKeyData()).first;
    }
    contextIt->second.stats.incrementPostedTaskCount();
    contextIt->second.stats.incrementPendingTaskCount();

    // save the context as the last for this sequenceKey
    contextIt->second.context = ctx->post<int>(queueId,
                                               isHighPriority,
                                               &waitForTwoDependents<FUNC, ARGS...>,
                                               ICoroContextBasePtr(contextIt->second.context),
                                               ICoroContextBasePtr(universalContext.context),
                                               contextIt->second.stats,
                                               std::move(opaque),
                                               exceptionCallback,
                                               std::forward<FUNC>(func),
                                               std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::multiSequenceKeyTaskScheduler(
    CoroContextPtr<int> ctx,
    int queueId,
    bool isHighPriority,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    std::vector<SequenceKey>&& sequenceKeys,
    ContextMap& contexts,
    SequencerKeyData& universalContext,
    FUNC&& func,
    ARGS&&... args)
{
    // construct the dependent collection
    std::vector<std::pair<ICoroContextBasePtr, SequencerKeyStatisticsWriter*>> dependents;
    dependents.reserve(sequenceKeys.size() + 1);
    dependents.push_back(std::make_pair(universalContext.context, nullptr));
    for (const SequenceKey& sequenceKey : sequenceKeys)
    {
        auto taskIt = contexts.find(sequenceKey);
        if (taskIt != contexts.end())
        {
            // update the depedent stats only
            taskIt->second.stats.incrementPostedTaskCount();
            taskIt->second.stats.incrementPendingTaskCount();
            // pass the pointer to the stats to the wait function so that it updates them too
            dependents.push_back(std::make_pair(taskIt->second.context, &taskIt->second.stats));
        }
    }
    ICoroContextBasePtr newCtx = ctx->post<int>(queueId,
                                                isHighPriority,
                                                &waitForDependents<FUNC, ARGS...>,
                                                std::move(dependents),
                                                std::move(opaque),
                                                exceptionCallback,
                                                std::forward<FUNC>(func),
                                                std::forward<ARGS>(args)...);
    // save the context as the last for each sequenceKey
    for (const SequenceKey& sequenceKey : sequenceKeys)
    {
        contexts[sequenceKey].context = newCtx;
    }
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FUNC, class ... ARGS>
int
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::universalTaskScheduler(
    CoroContextPtr<int> ctx,
    int queueId,
    bool isHighPriority,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    ContextMap& contexts,
    SequencerKeyData& universalContext,
    FUNC&& func,
    ARGS&&... args)
{
    // construct the dependent collection
    std::vector<std::pair<ICoroContextBasePtr, SequencerKeyStatisticsWriter*>> dependents;
    dependents.reserve(contexts.size() + 1);
    dependents.push_back(std::make_pair(universalContext.context, &universalContext.stats));
    for(typename ContextMap::iterator ctxIt = contexts.begin(); ctxIt != contexts.end(); )
    {
        typename ContextMap::iterator thisIt = ctxIt++;
        // since we're accessing all the contexts here, check if can trim some of them
        if (canTrimContext(ctx, thisIt->second.context))
        {
            contexts.erase(thisIt);
        }
        else
        {
            dependents.push_back(std::make_pair(thisIt->second.context, nullptr));
        }
    }
    // update the universal stats only
    universalContext.stats.incrementPostedTaskCount();
    universalContext.stats.incrementPendingTaskCount();

    // post the task and save the context as the last for the unviersal sequenceKey
    universalContext.context = ctx->post<int>(queueId,
                                              isHighPriority,
                                              &waitForDependents<FUNC, ARGS...>,
                                              std::move(dependents),
                                              std::move(opaque),
                                              exceptionCallback,
                                              std::forward<FUNC>(func),
                                              std::forward<ARGS>(args)...);
    return 0;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
template <class FINAL_ACTION, class FUNC, class ... ARGS>
void
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::callPosted(
    CoroContextPtr<int> ctx,
    const FINAL_ACTION& finalAction,
    void* opaque,
    const typename Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
        exceptionCallback,
    FUNC&& func,
    ARGS&&... args)
{
    auto finalActionWrapper = [&finalAction](int*)
    {
        // this should not throw anything outside
        try
        {
            finalAction();
        }
        catch(...) {}
    };
    // make sure the final action is eventually called 
    int dummy = 0;
    std::unique_ptr<int, decltype(finalActionWrapper)&> finalActionCaller(&dummy, finalActionWrapper);
    try
    {
        func(ctx, std::forward<ARGS>(args)...);
    }
    catch(std::exception& ex)
    {
        if (exceptionCallback)
        {
            exceptionCallback(std::current_exception(), opaque);
        }
        throw;
    }
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
bool
Sequencer<SequenceKey, Hash, KeyEqual, Allocator>::canTrimContext(const ICoroContextBasePtr& ctx,
                                                                  const ICoroContextBasePtr& ctxToValidate)
{
    return !ctxToValidate || !ctxToValidate->valid() ||
        ctxToValidate->waitFor(ctx, std::chrono::milliseconds(0)) == std::future_status::ready;
}


}}
