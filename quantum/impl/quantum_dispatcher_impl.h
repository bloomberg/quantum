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

#include <quantum/util/quantum_util.h>

namespace Bloomberg {
namespace quantum {

inline
Dispatcher::Dispatcher(const Configuration& config) :
    _dispatcher(config),
    _drain(false),
    _terminated(false)
{}

inline
Dispatcher::~Dispatcher()
{
    drain(std::chrono::milliseconds::zero(), true);
    terminate();
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::post(FUNC&& func,
                 ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>
{
    using Ret = decltype(coroResult(func));
    return postImpl<Ret>((int)IQueue::QueueId::Any,
                         false,
                         ITask::Type::Standalone,
                         std::forward<FUNC>(func),
                         std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::post2(FUNC&& func,
                  ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postImpl<Ret>((int)IQueue::QueueId::Any,
                          false,
                          ITask::Type::Standalone,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::post(int queueId,
                 bool isHighPriority,
                 FUNC&& func,
                 ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>
{
    using Ret = decltype(coroResult(func));
    return postImpl<Ret>(queueId,
                         isHighPriority,
                         ITask::Type::Standalone,
                         std::forward<FUNC>(func),
                         std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::post2(int queueId,
                  bool isHighPriority,
                  FUNC&& func,
                  ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postImpl<Ret>(queueId,
                          isHighPriority,
                          ITask::Type::Standalone,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postFirst(FUNC&& func,
                      ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>
{
    using Ret = decltype(coroResult(func));
    return postImpl<Ret>((int)IQueue::QueueId::Any,
                         false, ITask::Type::First,
                         std::forward<FUNC>(func),
                         std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postFirst2(FUNC&& func,
                       ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postImpl<Ret>((int)IQueue::QueueId::Any,
                          false,
                          ITask::Type::First,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postFirst(int queueId,
                      bool isHighPriority,
                      FUNC&& func,
                      ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>
{
    using Ret = decltype(coroResult(func));
    return postImpl<Ret>(queueId,
                         isHighPriority,
                         ITask::Type::First,
                         std::forward<FUNC>(func),
                         std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postFirst2(int queueId,
                       bool isHighPriority,
                       FUNC&& func,
                       ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postImpl<Ret>(queueId,
                          isHighPriority,
                          ITask::Type::First,
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postAsyncIo(FUNC&& func,
                        ARGS&&... args)->ThreadFuturePtr<decltype(ioResult(func))>
{
    using Ret = decltype(ioResult(func));
    return postAsyncIoImpl<Ret>((int)IQueue::QueueId::Any,
                                false,
                                std::forward<FUNC>(func),
                                std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postAsyncIo2(FUNC&& func,
                         ARGS&&... args)->ThreadFuturePtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postAsyncIoImpl<Ret>((int)IQueue::QueueId::Any,
                                 false, std::forward<FUNC>(func),
                                 std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postAsyncIo(int queueId,
                        bool isHighPriority,
                        FUNC&& func,
                        ARGS&&... args)->ThreadFuturePtr<decltype(ioResult(func))>
{
    using Ret = decltype(ioResult(func));
    return postAsyncIoImpl<Ret>(queueId,
                                isHighPriority,
                                std::forward<FUNC>(func),
                                std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
auto
Dispatcher::postAsyncIo2(int queueId,
                         bool isHighPriority,
                         FUNC&& func,
                         ARGS&&... args)->ThreadFuturePtr<decltype(resultOf2(func))>
{
    using Ret = decltype(resultOf2(func));
    return postAsyncIoImpl<Ret>(queueId,
                                 isHighPriority,
                                 std::forward<FUNC>(func),
                                 std::forward<ARGS>(args)...);
}

template <class RET, class INPUT_IT, class FUNC, class>
auto
Dispatcher::forEach(INPUT_IT first,
                    INPUT_IT last,
                    FUNC&& func)->ThreadContextPtr<std::vector<decltype(coroResult(func))>>
{
    return forEach(first, std::distance(first, last), std::forward<FUNC>(func));
}

template <class RET, class INPUT_IT, class FUNC>
auto
Dispatcher::forEach(INPUT_IT first,
                    size_t num,
                    FUNC&& func)->ThreadContextPtr<std::vector<decltype(coroResult(func))>>
{
    using Ret = decltype(coroResult(func));
    return post2(Util::forEachCoro<Ret, INPUT_IT, FUNC&&>,
                 INPUT_IT{first},
                 size_t{num},
                 std::forward<FUNC>(func));
}

template <class RET, class INPUT_IT, class FUNC, class>
auto
Dispatcher::forEachBatch(INPUT_IT first,
                         INPUT_IT last,
                         FUNC&& func)->ThreadContextPtr<std::vector<std::vector<decltype(coroResult(func))>>>
{
    return forEachBatch(first, std::distance(first, last), std::forward<FUNC>(func));
}

template <class RET, class INPUT_IT, class FUNC>
auto
Dispatcher::forEachBatch(INPUT_IT first,
                         size_t num,
                         FUNC&& func)->ThreadContextPtr<std::vector<std::vector<decltype(coroResult(func))>>>
{
    using Ret = decltype(coroResult(func));
    return post2(Util::forEachBatchCoro<Ret, INPUT_IT, FUNC&&>,
                 INPUT_IT{first},
                 size_t{num},
                 std::forward<FUNC>(func),
                 getNumCoroutineThreads());
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class MAPPER_FUNC,
          class REDUCER_FUNC,
          class INPUT_IT,
          class>
auto
Dispatcher::mapReduce(INPUT_IT first,
                      INPUT_IT last,
                      MAPPER_FUNC mapper,
                      REDUCER_FUNC reducer)->
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>
{
    return mapReduce(first, std::distance(first, last), std::move(mapper), std::move(reducer));
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class MAPPER_FUNC,
          class REDUCER_FUNC,
          class INPUT_IT,
          class>
auto
Dispatcher::mapReduce(INPUT_IT first,
                      size_t num,
                      MAPPER_FUNC mapper,
                      REDUCER_FUNC reducer)->
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>
{
    using Key = decltype(mappedKeyOf(mapper));
    using MappedType = decltype(mappedTypeOf(mapper));
    using ReducedType = decltype(reducedTypeOf(reducer));
    return post2(Util::mapReduceCoro<Key, MappedType, ReducedType, INPUT_IT>,
                 INPUT_IT{first},
                 size_t{num},
                 Functions::MapFunc<Key, MappedType, INPUT_IT>{std::move(mapper)},
                 Functions::ReduceFunc<Key, MappedType, ReducedType>{std::move(reducer)});
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class MAPPER_FUNC,
          class REDUCER_FUNC,
          class INPUT_IT,
          class>
auto
Dispatcher::mapReduceBatch(INPUT_IT first,
                           INPUT_IT last,
                           MAPPER_FUNC mapper,
                           REDUCER_FUNC reducer)->
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>
{
    return mapReduceBatch(first, std::distance(first, last), std::move(mapper), std::move(reducer));
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class MAPPER_FUNC,
          class REDUCER_FUNC,
          class INPUT_IT,
          class>
auto
Dispatcher::mapReduceBatch(INPUT_IT first,
                           size_t num,
                           MAPPER_FUNC mapper,
                           REDUCER_FUNC reducer)->
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>
{
    using Key = decltype(mappedKeyOf(mapper));
    using MappedType = decltype(mappedTypeOf(mapper));
    using ReducedType = decltype(reducedTypeOf(reducer));
    return post2(Util::mapReduceBatchCoro<Key, MappedType, ReducedType, INPUT_IT>,
                 INPUT_IT{first},
                 size_t{num},
                 Functions::MapFunc<Key, MappedType, INPUT_IT>{std::move(mapper)},
                 Functions::ReduceFunc<Key, MappedType, ReducedType>{std::move(reducer)});
}

inline
void Dispatcher::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        _dispatcher.terminate();
    }
}

inline
size_t Dispatcher::size(IQueue::QueueType type,
                        int queueId) const
{
    return _dispatcher.size(type, queueId);
}

inline
bool Dispatcher::empty(IQueue::QueueType type,
                       int queueId) const
{
    return _dispatcher.empty(type, queueId);
}

inline
void Dispatcher::drain(std::chrono::milliseconds timeout,
                       bool isFinal)
{
    DrainGuard guard(_drain, !isFinal);
    
    auto start = std::chrono::steady_clock::now();
    
    //wait until all queues have completed their work
    YieldingThread yield;
    while (!empty())
    {
        yield();
        
        //check remaining time
        if (timeout != std::chrono::milliseconds::zero())
        {
            auto present = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(present-start) > timeout)
            {
                //timeout reached
                break;
            }
        }
    }
    
#ifdef __QUANTUM_PRINT_DEBUG
    std::lock_guard<std::mutex> guard(Util::LogMutex());
    std::cout << "All queues have drained." << std::endl;
#endif
}

inline
int Dispatcher::getNumCoroutineThreads() const
{
    return _dispatcher.getNumCoroutineThreads();
}

inline
int Dispatcher::getNumIoThreads() const
{
    return _dispatcher.getNumIoThreads();
}

inline
const std::pair<int, int>& Dispatcher::getCoroQueueIdRangeForAny() const
{
    return _dispatcher.getCoroQueueIdRangeForAny();
}

inline
QueueStatistics Dispatcher::stats(IQueue::QueueType type,
                                  int queueId)
{
    return _dispatcher.stats(type, queueId);
}

inline
void Dispatcher::resetStats()
{
    _dispatcher.resetStats();
}

template <class RET, class FUNC, class ... ARGS>
ThreadContextPtr<RET>
Dispatcher::postImpl(int queueId,
                     bool isHighPriority,
                     ITask::Type type,
                     FUNC&& func,
                     ARGS&&... args)
{
    using FirstArg = decltype(firstArgOf(func));
    if (_drain || _terminated)
    {
        throw std::runtime_error("Posting is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid coroutine queue id");
    }
    auto ctx = ContextPtr<RET>(new Context<RET>(_dispatcher),
                               Context<RET>::deleter);
    auto task = Task::Ptr(new Task(Traits::IsVoidContext<FirstArg>{},
                                   ctx,
                                   queueId,
                                   isHighPriority,
                                   type,
                                   std::forward<FUNC>(func),
                                   std::forward<ARGS>(args)...),
                          Task::deleter);
    ctx->setTask(task);
    if (type == ITask::Type::Standalone)
    {
        _dispatcher.post(task);
    }
    return std::static_pointer_cast<IThreadContext<RET>>(ctx);
}

template <class RET, class FUNC, class ... ARGS>
ThreadFuturePtr<RET>
Dispatcher::postAsyncIoImpl(int queueId,
                            bool isHighPriority,
                            FUNC&& func,
                            ARGS&&... args)
{
    using FirstArg = decltype(firstArgOf(func));
    if (_drain || _terminated)
    {
        throw std::runtime_error("Posting is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    auto promise = PromisePtr<RET>(new Promise<RET>(), Promise<RET>::deleter);
    auto task = IoTask::Ptr(new IoTask(Traits::IsThreadPromise<FirstArg>{},
                                       promise,
                                       queueId,
                                       isHighPriority,
                                       std::forward<FUNC>(func),
                                       std::forward<ARGS>(args)...),
                            IoTask::deleter);
    _dispatcher.postAsyncIo(task);
    return promise->getIThreadFuture();
}

}}
