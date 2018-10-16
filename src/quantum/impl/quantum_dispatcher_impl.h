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
TaskDispatcher::TaskDispatcher(int numCoroutineThreads,
                               int numIoThreads,
                               bool pinCoroutineThreadsToCores) :
    _dispatcher(numCoroutineThreads, numIoThreads, pinCoroutineThreadsToCores),
    _drain(false),
    _terminated(ATOMIC_FLAG_INIT)
{}

inline
TaskDispatcher::TaskDispatcher(const Configuration& config) :
    _dispatcher(config),
    _drain(false),
    _terminated(ATOMIC_FLAG_INIT)
{}

inline
TaskDispatcher::~TaskDispatcher()
{
    drain();
    terminate();
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadContext<RET>::Ptr
TaskDispatcher::post(FUNC&& func,
                     ARGS&&... args)
{
    return postImpl<RET>((int)IQueue::QueueId::Any, false, ITask::Type::Standalone, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadContext<RET>::Ptr
TaskDispatcher::post(int queueId,
                     bool isHighPriority,
                     FUNC&& func,
                     ARGS&&... args)
{
    return postImpl<RET>(queueId, isHighPriority, ITask::Type::Standalone, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadContext<RET>::Ptr
TaskDispatcher::postFirst(FUNC&& func,
                          ARGS&&... args)
{
    return postImpl<RET>((int)IQueue::QueueId::Any, false, ITask::Type::First, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadContext<RET>::Ptr
TaskDispatcher::postFirst(int queueId,
                          bool isHighPriority,
                          FUNC&& func,
                          ARGS&&... args)
{
    return postImpl<RET>(queueId, isHighPriority, ITask::Type::First, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadFuture<RET>::Ptr
TaskDispatcher::postAsyncIo(FUNC&& func,
                            ARGS&&... args)
{
    return postAsyncIoImpl<RET>((int)IQueue::QueueId::Any, false, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadFuture<RET>::Ptr
TaskDispatcher::postAsyncIo(int queueId,
                            bool isHighPriority,
                            FUNC&& func,
                            ARGS&&... args)
{
    return postAsyncIoImpl<RET>(queueId, isHighPriority, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<typename ThreadContext<RET>::Ptr>
TaskDispatcher::forEach(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func)
{
    return forEach<RET>(first, std::distance(first, last), std::forward<UNARY_FUNC>(func));
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<RET>
TaskDispatcher::forEachSync(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func)
{
    return forEachSync<RET>(first, std::distance(first, last), std::forward<UNARY_FUNC>(func));
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<typename ThreadContext<RET>::Ptr>
TaskDispatcher::forEach(INPUT_IT first, size_t num, UNARY_FUNC&& func)
{
    std::vector<typename ThreadContext<RET>::Ptr> asyncResult;
    asyncResult.reserve(num);
    for (size_t i = 0; i < num; ++i, ++first)
    {
        //Run the function
        asyncResult.emplace_back(post<RET>([func, first](typename CoroContext<RET>::Ptr ctx)->int
        {
            return ctx->set(func(*first));
        }));
    }
    return asyncResult;
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<RET>
TaskDispatcher::forEachSync(INPUT_IT first, size_t num, UNARY_FUNC&& func)
{
    std::vector<RET> result;
    result.reserve(num);
    auto asyncResult = forEach<RET>(first, num, std::forward<UNARY_FUNC>(func));
    // Get the results
    for (auto&& tctx : asyncResult)
    {
        result.emplace_back(tctx->get());
    }
    return result;
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<typename ThreadContext<std::vector<RET>>::Ptr>
TaskDispatcher::forEachBatch(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func)
{
    return forEachBatch<RET>(first, std::distance(first, last), std::forward<UNARY_FUNC>(func));
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<RET>
TaskDispatcher::forEachBatchSync(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func)
{
    return forEachBatchSync<RET>(first, std::distance(first, last), std::forward<UNARY_FUNC>(func));
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<typename ThreadContext<std::vector<RET>>::Ptr>
TaskDispatcher::forEachBatch(INPUT_IT first, size_t num, UNARY_FUNC&& func)
{
    size_t numCoroThreads = getNumCoroutineThreads();
    size_t numPerBatch = num/numCoroThreads;
    size_t remainder = num%numCoroThreads;
    std::vector<typename ThreadContext<std::vector<RET>>::Ptr> batchedAsyncResult;
    batchedAsyncResult.reserve(numCoroThreads);
    
    // Post unto all the coroutine threads.
    for (size_t i = 0; i < numCoroThreads; ++i)
    {
        //get the begin and end iterators for each batch
        size_t num = (i < remainder) ? numPerBatch + 1 : numPerBatch;
        if (!num)
        {
            break; //nothing to do
        }
        batchedAsyncResult.emplace_back(post<std::vector<RET>>([func, first, num](typename CoroContext<std::vector<RET>>::Ptr ctx)->int
        {
            std::vector<RET> result;
            auto inputIt = first;
            for (size_t i = 0; i < num; ++i, ++inputIt)
            {
                result.emplace_back(func(*inputIt));
            }
            return ctx->set(std::move(result));
        }));
        std::advance(first, num);
    }
    return batchedAsyncResult;
}

template <class RET, class UNARY_FUNC, class INPUT_IT>
std::vector<RET>
TaskDispatcher::forEachBatchSync(INPUT_IT first, size_t num, UNARY_FUNC&& func)
{
    std::vector<RET> result;
    result.reserve(num);
    auto batchedAsyncResult = forEachBatch<RET>(first, num, std::forward<UNARY_FUNC>(func));
    // Get the results
    for (auto&& tctx : batchedAsyncResult)
    {
        std::vector<RET> v = tctx->get();
        result.insert(result.end(),
                      std::move_iterator<typename std::vector<RET>::iterator>(v.begin()),
                      std::move_iterator<typename std::vector<RET>::iterator>(v.end()));
    }
    return result;
}

inline
void TaskDispatcher::terminate()
{
    if (!_terminated.test_and_set())
    {
        _dispatcher.terminate();
    }
}

inline
size_t TaskDispatcher::size(IQueue::QueueType type,
                            int queueId) const
{
    return _dispatcher.size(type, queueId);
}

inline
bool TaskDispatcher::empty(IQueue::QueueType type,
                           int queueId) const
{
    return _dispatcher.empty(type, queueId);
}

inline
void TaskDispatcher::drain()
{
    _drain = true;
    
    //wait until all queues have completed their work
    YieldingThread yield;
    while (!empty())
    {
        yield();
    }
    
#ifdef __QUANTUM_PRINT_DEBUG
    std::lock_guard<std::mutex> guard(Util::LogMutex());
    std::cout << "All queues have drained." << std::endl;
#endif
    _drain = false;
}

inline
int TaskDispatcher::getNumCoroutineThreads() const
{
    return _dispatcher.getNumCoroutineThreads();
}

inline
int TaskDispatcher::getNumIoThreads() const
{
    return _dispatcher.getNumIoThreads();
}

inline
QueueStatistics TaskDispatcher::stats(IQueue::QueueType type,
                                      int queueId)
{
    return _dispatcher.stats(type, queueId);
}

inline
void TaskDispatcher::resetStats()
{
    _dispatcher.resetStats();
}

template <class RET, class FUNC, class ... ARGS>
typename ThreadContext<RET>::Ptr
TaskDispatcher::postImpl(int queueId,
                         bool isHighPriority,
                         ITask::Type type,
                         FUNC&& func,
                         ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Posting is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid coroutine queue id");
    }
    auto ctx = typename Context<RET>::Ptr(new Context<RET>(_dispatcher),
                                          Context<RET>::deleter);
    auto task = Task::Ptr(new Task(ctx,
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
typename ThreadFuture<RET>::Ptr
TaskDispatcher::postAsyncIoImpl(int queueId,
                               bool isHighPriority,
                               FUNC&& func,
                               ARGS&&... args)
{
    if (_drain)
    {
        throw std::runtime_error("Posting is disabled");
    }
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid IO queue id");
    }
    auto promise = typename Promise<RET>::Ptr(new Promise<RET>(), Promise<RET>::deleter);
    auto task = IoTask::Ptr(new IoTask(promise,
                                       queueId,
                                       isHighPriority,
                                       std::forward<FUNC>(func),
                                       std::forward<ARGS>(args)...),
                            IoTask::deleter);
    _dispatcher.postAsyncIo(task);
    return promise->getIThreadFuture();
}

}}
