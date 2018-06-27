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
#ifndef QUANTUM_CONTEXT_H
#define QUANTUM_CONTEXT_H

#include <quantum/quantum_promise.h>
#include <quantum/quantum_task.h>
#include <quantum/quantum_io_task.h>
#include <quantum/quantum_dispatcher_core.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class Context
//==============================================================================================
/// @class Context
/// @brief Concrete class representing a coroutine or a thread context.
/// @note For internal use only. See interfaces ICoroContext and IThreadContext for usage details.
template <class RET>
class Context : public IThreadContext<RET>,
                public ICoroContext<RET>,
                public ITaskAccessor,
                public std::enable_shared_from_this<Context<RET>>
{
    friend class Util;
    friend class Task;
    friend class TaskDispatcher;
    template <class OTHER_RET> friend class Context;
    
public:
    using ptr = std::shared_ptr<Context<RET>>;
    using thread_ctx = IThreadContext<RET>;
    using coro_ctx = ICoroContext<RET>;
    
    //===================================
    //              D'TOR
    //===================================
    ~Context();

    //===================================
    //           ITERMINATE
    //===================================
    void terminate() final;
    
    //===================================
    //         ITASKACCESSOR
    //===================================
    void setTask(ITask::ptr task) final;
    ITask::ptr getTask() const final;
    bool isBlocked() final;

    //===================================
    //         ICONTEXTBASE
    //===================================
    bool valid() const final;
    bool validAt(int num) const final;
    int setException(std::exception_ptr ex) final;
    
    //===================================
    //        ITHREADCONTEXTBASE
    //===================================
    void waitAt(int num) const final;
    std::future_status waitForAt(int num, size_t timeMs) const final;
    void wait() const final;
    std::future_status waitFor(size_t timeMs) const final;
    void waitAll() const final;

    //===================================
    //         ITHREADCONTEXT
    //===================================
    RET get() final;
    const RET& getRef() const final;
    
    //===================================
    //        ICOROCONTEXTBASE
    //===================================
    void waitAt(int num, ICoroSync::ptr sync) const final;
    std::future_status waitForAt(int num, ICoroSync::ptr sync, size_t timeMs) const final;
    void wait(ICoroSync::ptr sync) const final;
    std::future_status waitFor(ICoroSync::ptr sync, size_t timeMs) const final;
    void waitAll(ICoroSync::ptr sync) const final;
    
    //===================================
    //         ICOROCONTEXT
    //===================================
    RET get(ICoroSync::ptr sync);
    const RET& getRef(ICoroSync::ptr sync) const final;
    
    //===================================
    //           ICOROSYNC
    //===================================
    void setYieldHandle(Traits::yield_t& yield) final;
    Traits::yield_t& getYieldHandle() final;
    void yield() final;
    std::atomic_int& signal() final;
    void sleep(size_t timeMs) final;
    
    //===================================
    //   NON-VIRTUAL IMPLEMENTATIONS
    //===================================
    template <class V = RET>
    int set(V&& value);
    
    template <class V = RET>
    int set(ICoroSync::ptr sync, V&& value);
    
    template <class BUF = RET, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    void push(V &&value);
    
    template <class BUF = RET, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    void push(ICoroSync::ptr sync, V &&value);
    
    template <class BUF = RET, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    V pull(bool& isBufferClosed);
    
    template <class BUF = RET, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    V pull(ICoroSync::ptr sync, bool& isBufferClosed);
    
    template <class BUF = RET, class = std::enable_if_t<Traits::IsBuffer<BUF>::value>>
    int closeBuffer();
    
    template <class OTHER_RET>
    OTHER_RET getAt(int num);
    
    template <class OTHER_RET>
    const OTHER_RET& getRefAt(int num) const;
    
    template <class OTHER_RET>
    OTHER_RET getAt(int num, ICoroSync::ptr sync);

    template <class OTHER_RET>
    const OTHER_RET& getRefAt(int num, ICoroSync::ptr sync) const;
    
    template <class OTHER_RET>
    OTHER_RET getPrev(ICoroSync::ptr sync);
    
    template <class OTHER_RET>
    const OTHER_RET& getPrevRef(ICoroSync::ptr sync);
    
    //===================================
    //        TASK CONTINUATIONS
    //===================================
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    post(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    postFirst(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    then(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    onError(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    finally(FUNC&& func, ARGS&&... args);
    
    ptr end();
    
    //===================================
    //           BLOCKING IO
    //===================================
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename ICoroFuture<OTHER_RET>::ptr
    postAsyncIo(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename ICoroFuture<OTHER_RET>::ptr
    postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
private:
    explicit Context(DispatcherCore& dispatcher);
    
    template <class OTHER_RET>
    Context(Context<OTHER_RET>& other);
    
    Context(IContextBase& other);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    thenImpl(ITask::Type type, FUNC&& func, ARGS&&... args);

    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::ptr
    postImpl(int queueId, bool isHighPriority, ITask::Type type, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET = int, class FUNC, class ... ARGS>
    typename ICoroFuture<OTHER_RET>::ptr
    postAsyncIoImpl(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    int index(int num) const;
    
    void validateTaskType(ITask::Type type) const; //throws
    
    void validateContext(ICoroSync::ptr sync) const; //throws
    
    //Members
    ITask::ptr                          _task;
    std::vector<IPromiseBase::ptr>      _promises;
    DispatcherCore*                     _dispatcher;
    std::atomic_flag                    _terminated;
    std::atomic_int                     _signal;
    Traits::yield_t*                    _yield;
};

}}

#include <quantum/interface/quantum_icontext.h>
#include <quantum/impl/quantum_context_impl.h>

#endif //QUANTUM_CONTEXT_H
