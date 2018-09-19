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
