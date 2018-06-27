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

//==============================================================================================
//                       class IThreadContext (fwd to implementation)
//==============================================================================================
template <class RET>
template <class V>
int IThreadContext<RET>::set(V&& value)
{
    return static_cast<impl*>(this)->template set(std::forward<V>(value));
}

template <class RET>
template <class OTHER_RET>
OTHER_RET IThreadContext<RET>::getAt(int num)
{
    return static_cast<impl*>(this)->template getAt<OTHER_RET>(num);
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& IThreadContext<RET>::getRefAt(int num) const
{
    return static_cast<const impl*>(this)->template getRefAt<OTHER_RET>(num);
}

template <class RET>
template <class BUF, class V>
void IThreadContext<RET>::push(V &&value)
{
    static_cast<impl*>(this)->template push<BUF>(std::forward<V>(value));
}

template <class RET>
template <class BUF, class V>
V IThreadContext<RET>::pull(bool& isBufferClosed)
{
    return static_cast<impl*>(this)->template pull<BUF>(isBufferClosed);
}

template <class RET>
template <class BUF, class>
int IThreadContext<RET>::closeBuffer()
{
    return static_cast<impl*>(this)->template closeBuffer<BUF>();
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename IThreadContext<OTHER_RET>::ptr
IThreadContext<RET>::then(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template then<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename IThreadContext<OTHER_RET>::ptr
IThreadContext<RET>::onError(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template onError<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename IThreadContext<OTHER_RET>::ptr
IThreadContext<RET>::finally(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template finally<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
typename IThreadContext<RET>::ptr
IThreadContext<RET>::end()
{
    return static_cast<impl*>(this)->end();
}

//==============================================================================================
//                       class ICoroContext (fwd to implementation)
//==============================================================================================
template <class RET>
template <class V>
int ICoroContext<RET>::set(V&& value)
{
    std::shared_ptr<impl> ctx = static_cast<impl*>(this)->shared_from_this();
    return ctx->template set(ctx, std::forward<V>(value));
}

template <class RET>
template <class OTHER_RET>
OTHER_RET ICoroContext<RET>::getPrev()
{
    std::shared_ptr<impl> ctx = static_cast<impl*>(this)->shared_from_this();
    return ctx->template getPrev<OTHER_RET>(ctx);
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& ICoroContext<RET>::getPrevRef()
{
    std::shared_ptr<impl> ctx = static_cast<impl*>(this)->shared_from_this();
    return ctx->template getPrevRef<OTHER_RET>(ctx);
}

template <class RET>
template <class OTHER_RET>
OTHER_RET ICoroContext<RET>::getAt(int num, ICoroSync::ptr sync)
{
    return static_cast<impl*>(this)->template getAt<OTHER_RET>(num, sync);
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& ICoroContext<RET>::getRefAt(int num, ICoroSync::ptr sync) const
{
    return static_cast<const impl*>(this)->template getRefAt<OTHER_RET>(num, sync);
}

template <class RET>
template <class BUF, class V>
void ICoroContext<RET>::push(V &&value)
{
    std::shared_ptr<impl> ctx = static_cast<impl*>(this)->shared_from_this();
    ctx->template push<BUF>(ctx, std::forward<V>(value));
}

template <class RET>
template <class BUF, class V>
V ICoroContext<RET>::pull(ICoroSync::ptr sync, bool& isBufferClosed)
{
    return static_cast<impl*>(this)->template pull<BUF>(sync, isBufferClosed);
}

template <class RET>
template <class BUF, class>
int ICoroContext<RET>::closeBuffer()
{
    return static_cast<impl*>(this)->template closeBuffer<BUF>();
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::post(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template post<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template post<OTHER_RET>(queueId, isHighPriority, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::postFirst(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template postFirst<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template postFirst<OTHER_RET>(queueId, isHighPriority, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::then(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template then<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::onError(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template onError<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroContext<OTHER_RET>::ptr
ICoroContext<RET>::finally(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template finally<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
typename ICoroContext<RET>::ptr
ICoroContext<RET>::end()
{
    return static_cast<impl*>(this)->end();
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroFuture<OTHER_RET>::ptr
ICoroContext<RET>::postAsyncIo(FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template postAsyncIo<OTHER_RET>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroFuture<OTHER_RET>::ptr
ICoroContext<RET>::postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return static_cast<impl*>(this)->template postAsyncIo<OTHER_RET>(queueId, isHighPriority, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

//==============================================================================================
//                                     class Context
//==============================================================================================
template <class RET>
Context<RET>::Context(DispatcherCore& dispatcher) :
    _promises(1, std::make_shared<Promise<RET>>()),
    _dispatcher(&dispatcher),
    _terminated(ATOMIC_FLAG_INIT),
    _signal(-1),
    _yield(nullptr)
{}

template <class RET>
template <class OTHER_RET>
Context<RET>::Context(Context<OTHER_RET>& other) :
    _promises(other._promises),
    _dispatcher(other._dispatcher),
    _terminated(ATOMIC_FLAG_INIT),
    _signal(-1),
    _yield(nullptr)
{
    _promises.emplace_back(std::make_shared<Promise<RET>>()); //append a new promise
}

template <class RET>
Context<RET>::~Context()
{
    terminate();
}

template <class RET>
void Context<RET>::terminate()
{
    if (!_terminated.test_and_set())
    {
        _promises.back()->terminate();
        
        //unlink task ptr
        _task.reset();
    }
}

template <class RET>
bool Context<RET>::validAt(int num) const
{
    return _promises[index(num)]->valid();
}

template <class RET>
bool Context<RET>::valid() const
{
    return validAt(-1);
}

template <class RET>
int Context<RET>::setException(std::exception_ptr ex)
{
    return _promises.back()->setException(ex);
}

template <class RET>
bool Context<RET>::isBlocked()
{
    return _signal == 0;
}

template <class RET>
int Context<RET>::index(int num) const
{
    if ((num < -1) || (num >= (int)_promises.size()))
    {
        ThrowFutureException(FutureState::NoState);
    }
    return (num == -1) ? _promises.size() - 1 : num;
}

template <class RET>
void Context<RET>::validateTaskType(ITask::Type type) const
{
    if (!_task)
    {
        throw std::runtime_error("Invalid task pointer");
    }
    
    bool isValid = true;
    switch (type)
    {
        case ITask::Type::Continuation:
        case ITask::Type::ErrorHandler:
            if ((_task->getType() != ITask::Type::First) &&
                (_task->getType() != ITask::Type::Continuation))
            {
                isValid = false;
            }
            break;
        case ITask::Type::Final:
            if ((_task->getType() != ITask::Type::First) &&
                (_task->getType() != ITask::Type::Continuation) &&
                (_task->getType() != ITask::Type::ErrorHandler))
            {
                isValid = false;
            }
            break;
        case ITask::Type::Termination:
            if ((_task->getType() != ITask::Type::First) &&
                (_task->getType() != ITask::Type::Continuation) &&
                (_task->getType() != ITask::Type::ErrorHandler) &&
                (_task->getType() != ITask::Type::Final))
            {
                isValid = false;
            }
            break;
        default:
            break;
    }
    
    if (!isValid)
    {
        throw std::runtime_error("Restricted continuation method");
    }
}

template <class RET>
void Context<RET>::validateContext(ICoroSync::ptr sync) const
{
    if (static_cast<const ICoroSync*>(this) == sync.get())
    {
        throw std::runtime_error("Must use different synchronization object");
    }
}

template <class RET>
void Context<RET>::setTask(ITask::ptr task)
{
    _task = task;
}

template <class RET>
ITask::ptr Context<RET>::getTask() const
{
    return _task;
}

template <class RET>
void Context<RET>::setYieldHandle(Traits::yield_t& yield)
{
    _yield = &yield;
}

template <class RET>
Traits::yield_t& Context<RET>::getYieldHandle()
{
    if (!_yield) throw std::runtime_error("Yield handle is null");
    return *_yield;
}

template <class RET>
void Context<RET>::yield()
{
    getYieldHandle()();
}

template <class RET>
std::atomic_int& Context<RET>::signal()
{
    return _signal;
}

template <class RET>
void Context<RET>::sleep(size_t timeMs)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::milliseconds(timeMs);
    
    //wait until signalled or times out
    while (1)
    {
        yield();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-start) >= end)
        {
            break; //timeout expired
        }
    }
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::thenImpl(ITask::Type type, FUNC&& func, ARGS&&... args)
{
    auto ctx = std::shared_ptr<Context<OTHER_RET>>(new Context<OTHER_RET>(*this));
    auto task = std::make_shared<Task>(ctx,
                                       _task->getQueueId(),      //keep current queueId
                                       _task->isHighPriority(),  //keep current priority
                                       type,
                                       std::forward<FUNC>(func),
                                       std::forward<ARGS>(args)...);
    ctx->setTask(task);
    
    //Chain tasks
    std::static_pointer_cast<ITaskContinuation>(_task)->setNextTask(task);
    task->setPrevTask(std::static_pointer_cast<ITaskContinuation>(_task));
    return ctx;
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::then(FUNC&& func, ARGS&&... args)
{
    //Previous task must either be First or Continuation types
    validateTaskType(ITask::Type::Continuation);
    return thenImpl<OTHER_RET, FUNC, ARGS...>(ITask::Type::Continuation, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::onError(FUNC&& func, ARGS&&... args)
{
    validateTaskType(ITask::Type::ErrorHandler);
    return thenImpl<OTHER_RET, FUNC, ARGS...>(ITask::Type::ErrorHandler, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::finally(FUNC&& func, ARGS&&... args)
{
    validateTaskType(ITask::Type::Final);
    return thenImpl<OTHER_RET, FUNC, ARGS...>(ITask::Type::Final, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
typename Context<RET>::ptr
Context<RET>::end()
{
    validateTaskType(ITask::Type::Termination);
    //Async post to next available queue since the previous task in the chain already terminated
    auto task = std::static_pointer_cast<Task>(std::static_pointer_cast<ITaskContinuation>(getTask())->getFirstTask());
    _dispatcher->post(task);
    return dynamic_cast<Context<RET>*>(this)->shared_from_this();
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroFuture<OTHER_RET>::ptr
Context<RET>::postAsyncIo(FUNC&& func, ARGS&&... args)
{
    return postAsyncIoImpl<OTHER_RET>((int)IQueue::QueueId::Any, false, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroFuture<OTHER_RET>::ptr
Context<RET>::postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return postAsyncIoImpl<OTHER_RET>(queueId, isHighPriority, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename ICoroFuture<OTHER_RET>::ptr
Context<RET>::postAsyncIoImpl(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    if (queueId < (int)IQueue::QueueId::Any)
    {
        throw std::runtime_error("Invalid queue id");
    }
    auto promise = std::make_shared<Promise<OTHER_RET>>();
    auto task = std::make_shared<IoTask>(promise,
                                         queueId,
                                         isHighPriority,
                                         std::forward<FUNC>(func),
                                         std::forward<ARGS>(args)...);
    _dispatcher->postAsyncIo(task);
    return promise->getICoroFuture();
}

template <class RET>
template <class V>
int Context<RET>::set(V&& value)
{
    return std::static_pointer_cast<Promise<RET>>(_promises.back())->set(std::forward<V>(value));
}

template <class RET>
template <class BUF, class V>
void Context<RET>::push(V &&value)
{
    std::static_pointer_cast<Promise<RET>>(_promises.back())->template push<BUF>(std::forward<V>(value));
}

template <class RET>
template <class BUF, class V>
void Context<RET>::push(ICoroSync::ptr sync, V &&value)
{
    std::static_pointer_cast<Promise<RET>>(_promises.back())->template push<BUF>(sync, std::forward<V>(value));
}

template <class RET>
template <class BUF, class V>
V Context<RET>::pull(bool& isBufferClosed)
{
    return std::static_pointer_cast<Promise<RET>>(_promises.back())->getIThreadFuture()->template pull<BUF>(isBufferClosed);
}

template <class RET>
template <class BUF, class V>
V Context<RET>::pull(ICoroSync::ptr sync, bool& isBufferClosed)
{
    return std::static_pointer_cast<Promise<RET>>(_promises.back())->getICoroFuture()->template pull<BUF>(sync, isBufferClosed);
}

template <class RET>
template <class BUF, class>
int Context<RET>::closeBuffer()
{
    return std::static_pointer_cast<Promise<RET>>(_promises.back())->template closeBuffer<BUF>();
}

template <class RET>
template <class OTHER_RET>
OTHER_RET Context<RET>::getAt(int num)
{
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(num)])->getIThreadFuture()->get();
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& Context<RET>::getRefAt(int num) const
{
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(num)])->getIThreadFuture()->getRef();
}

template <class RET>
RET Context<RET>::get()
{
    return getAt<RET>(-1);
}

template <class RET>
const RET& Context<RET>::getRef() const
{
    return getRefAt<RET>(-1);
}

template <class RET>
void Context<RET>::waitAt(int num) const
{
    _promises[index(num)]->getIThreadFutureBase()->wait();
}

template <class RET>
std::future_status Context<RET>::waitForAt(int num, size_t timeMs) const
{
    return _promises[index(num)]->getIThreadFutureBase()->waitFor(timeMs);
}

template <class RET>
void Context<RET>::wait() const
{
    waitAt(-1);
}

template <class RET>
std::future_status Context<RET>::waitFor(size_t timeMs) const
{
    return waitForAt(-1, timeMs);
}

template <class RET>
void Context<RET>::waitAll() const
{
    for (auto&& promise : _promises)
    {
        try
        {
            promise->getIThreadFutureBase()->wait();
        }
        catch(...) //catch all broken promises or any other exception
        {}
    }
}

template <class RET>
template <class V>
int Context<RET>::set(ICoroSync::ptr sync, V&& value)
{
    return std::static_pointer_cast<Promise<RET>>(_promises.back())->set(sync, std::forward<V>(value));
}

template <class RET>
template <class OTHER_RET>
OTHER_RET Context<RET>::getAt(int num,
                              ICoroSync::ptr sync)
{
    validateContext(sync);
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(num)])->getICoroFuture()->get(sync);
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& Context<RET>::getRefAt(int num,
                                        ICoroSync::ptr sync) const
{
    validateContext(sync);
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(num)])->getICoroFuture()->getRef(sync);
}

template <class RET>
RET Context<RET>::get(ICoroSync::ptr sync)
{
    return getAt<RET>(-1, sync);
}

template <class RET>
const RET& Context<RET>::getRef(ICoroSync::ptr sync) const
{
    return getRefAt<RET>(-1, sync);
}

template <class RET>
template <class OTHER_RET>
OTHER_RET Context<RET>::getPrev(ICoroSync::ptr sync)
{
    if (_promises.size() < 2)
    {
        ThrowFutureException(FutureState::NoState);
    }
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(_promises.size()-2)])->getICoroFuture()->get(sync);
}

template <class RET>
template <class OTHER_RET>
const OTHER_RET& Context<RET>::getPrevRef(ICoroSync::ptr sync)
{
    if (_promises.size() < 2)
    {
        ThrowFutureException(FutureState::NoState);
    }
    return std::static_pointer_cast<Promise<OTHER_RET>>(_promises[index(_promises.size()-2)])->getICoroFuture()->getRef(sync);
}

template <class RET>
void Context<RET>::waitAt(int num,
                          ICoroSync::ptr sync) const
{
    validateContext(sync);
    _promises[index(num)]->getICoroFutureBase()->wait(sync);
}

template <class RET>
std::future_status Context<RET>::waitForAt(int num,
                                           ICoroSync::ptr sync,
                                           size_t timeMs) const
{
    validateContext(sync);
    return _promises[index(num)]->getICoroFutureBase()->waitFor(sync, timeMs);
}

template <class RET>
void Context<RET>::wait(ICoroSync::ptr sync) const
{
    waitAt(-1, sync);
}

template <class RET>
std::future_status Context<RET>::waitFor(ICoroSync::ptr sync,
                                         size_t timeMs) const
{
    return waitForAt(-1, sync, timeMs);
}

template <class RET>
void Context<RET>::waitAll(ICoroSync::ptr sync) const
{
    for (auto&& promise : _promises)
    {
        try
        {
            promise->getICoroFutureBase()->wait(sync);
        }
        catch(...) //catch all broken promises or any other exception
        {
        }
    }
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::post(FUNC&& func, ARGS&&... args)
{
    return postImpl<OTHER_RET>((int)IQueue::QueueId::Any, false, ITask::Type::Standalone, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return postImpl<OTHER_RET>(queueId, isHighPriority, ITask::Type::Standalone, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::postFirst(FUNC&& func, ARGS&&... args)
{
    return postImpl<OTHER_RET>((int)IQueue::QueueId::Any, false, ITask::Type::First, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
{
    return postImpl<OTHER_RET>(queueId, isHighPriority, ITask::Type::First, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

template <class RET>
template <class OTHER_RET, class FUNC, class ... ARGS>
typename Context<OTHER_RET>::ptr
Context<RET>::postImpl(int queueId, bool isHighPriority, ITask::Type type, FUNC&& func, ARGS&&... args)
{
    if (queueId < (int)IQueue::QueueId::Same)
    {
        throw std::runtime_error("Invalid queue id");
    }
    auto ctx = std::shared_ptr<Context<OTHER_RET>>(new Context<OTHER_RET>(*_dispatcher));
    auto task = std::make_shared<Task>(ctx,
                                       (queueId == (int)IQueue::QueueId::Same) ? _task->getQueueId() : queueId,
                                       isHighPriority,
                                       type,
                                       std::forward<FUNC>(func),
                                       std::forward<ARGS>(args)...);
    ctx->setTask(task);
    if (type == ITask::Type::Standalone)
    {
        _dispatcher->post(task);
    }
    return ctx;
}


}}






