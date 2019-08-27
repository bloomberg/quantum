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
#ifndef BLOOMBERG_QUANTUM_CONTEXT_H
#define BLOOMBERG_QUANTUM_CONTEXT_H

#include <quantum/quantum_promise.h>
#include <quantum/quantum_task.h>
#include <quantum/quantum_io_task.h>
#include <quantum/quantum_dispatcher_core.h>
#include <quantum/quantum_traits.h>
#include <iterator>

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
    friend struct Util;
    friend class Task;
    friend class Dispatcher;
    template <class OTHER_RET> friend class Context;
    
public:
    using Ptr = std::shared_ptr<Context<RET>>;
    using ThreadCtx = IThreadContext<RET>;
    using CoroCtx = ICoroContext<RET>;
    
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
    void setTask(ITask::Ptr task) final;
    ITask::Ptr getTask() const final;
    bool isBlocked() const final;
    bool isSleeping(bool updateTimer = false) final;

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
    std::future_status waitForAt(int num, std::chrono::milliseconds timeMs) const final;
    void wait() const final;
    std::future_status waitFor(std::chrono::milliseconds timeMs) const final;
    void waitAll() const final;

    //===================================
    //         ITHREADCONTEXT
    //===================================
    template <class V = RET>
    NonBufferRetType<V> get();
    template <class V = RET>
    const NonBufferRetType<V>& getRef() const;
    template <class V, class = NonBufferType<RET,V>>
    int set(V&& value);
    template <class V, class = BufferType<RET,V>>
    void push(V&& value);
    template <class V = RET>
    BufferRetType<V> pull(bool& isBufferClosed);
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getAt(int num);
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getRefAt(int num) const;
    
    //===================================
    //        ICOROCONTEXTBASE
    //===================================
    void waitAt(int num, ICoroSync::Ptr sync) const final;
    std::future_status waitForAt(int num, ICoroSync::Ptr sync, std::chrono::milliseconds timeMs) const final;
    void wait(ICoroSync::Ptr sync) const final;
    std::future_status waitFor(ICoroSync::Ptr sync, std::chrono::milliseconds timeMs) const final;
    void waitAll(ICoroSync::Ptr sync) const final;
    
    //===================================
    //         ICOROCONTEXT
    //===================================
    template <class V = RET>
    NonBufferRetType<V> get(ICoroSync::Ptr sync);
    template <class V = RET>
    const NonBufferRetType<V>& getRef(ICoroSync::Ptr sync) const;
    template <class V, class = NonBufferType<RET,V>>
    int set(ICoroSync::Ptr sync, V&& value);
    template <class V, class = BufferType<RET,V>>
    void push(ICoroSync::Ptr sync, V&& value);
    template <class V = RET>
    BufferRetType<V> pull(ICoroSync::Ptr sync, bool& isBufferClosed);
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getAt(int num, ICoroSync::Ptr sync);
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getRefAt(int num, ICoroSync::Ptr sync) const;
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getPrev(ICoroSync::Ptr sync);
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getPrevRef(ICoroSync::Ptr sync);
    
    //===================================
    //           ICOROSYNC
    //===================================
    void setYieldHandle(Traits::Yield& yield) final;
    Traits::Yield& getYieldHandle() final;
    void yield() final;
    std::atomic_int& signal() final;
    void sleep(const std::chrono::milliseconds& timeMs) final;
    void sleep(const std::chrono::microseconds& timeUs) final;
    
    //===================================
    //      MISC IMPLEMENTATIONS
    //===================================
    template <class V = RET, class = BufferRetType<V>>
    int closeBuffer();
    int getNumCoroutineThreads() const;
    int getNumIoThreads() const;
    const std::pair<int, int>& getCoroQueueIdRangeForAny() const;
    
    //===================================
    //        TASK CONTINUATIONS
    //===================================
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    post(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    post2(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    post2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    postFirst(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    postFirst2(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    postFirst2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    then(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    then2(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    onError(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    onError2(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    finally(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    finally2(FUNC&& func, ARGS&&... args);
    
    Ptr end();
    
    //===================================
    //           BLOCKING IO
    //===================================
    template <class OTHER_RET, class FUNC, class ... ARGS>
    CoroFuturePtr<OTHER_RET>
    postAsyncIo(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    CoroFuturePtr<OTHER_RET>
    postAsyncIo2(FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    CoroFuturePtr<OTHER_RET>
    postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    CoroFuturePtr<OTHER_RET>
    postAsyncIo2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    //===================================
    //           FOR EACH
    //===================================
    template <class OTHER_RET, class INPUT_IT, class FUNC, class = Traits::IsInputIterator<INPUT_IT>>
    typename Context<std::vector<OTHER_RET>>::Ptr
    forEach(INPUT_IT first, INPUT_IT last, FUNC&& func);
    
    template <class OTHER_RET, class INPUT_IT, class FUNC>
    typename Context<std::vector<OTHER_RET>>::Ptr
    forEach(INPUT_IT first, size_t num, FUNC&& func);
    
    template <class OTHER_RET, class INPUT_IT, class FUNC, class = Traits::IsInputIterator<INPUT_IT>>
    typename Context<std::vector<std::vector<OTHER_RET>>>::Ptr
    forEachBatch(INPUT_IT first, INPUT_IT last, FUNC&& func);

    template <class OTHER_RET, class INPUT_IT, class FUNC>
    typename Context<std::vector<std::vector<OTHER_RET>>>::Ptr
    forEachBatch(INPUT_IT first, size_t num, FUNC&& func);
    
    //===================================
    //           MAP REDUCE
    //===================================
    template <class KEY,
              class MAPPED_TYPE,
              class REDUCED_TYPE,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    typename Context<std::map<KEY, REDUCED_TYPE>>::Ptr
    mapReduce(INPUT_IT first,
              INPUT_IT last,
              Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT> mapper,
              Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE> reducer);
    
    template <class KEY,
              class MAPPED_TYPE,
              class REDUCED_TYPE,
              class INPUT_IT>
    typename Context<std::map<KEY, REDUCED_TYPE>>::Ptr
    mapReduce(INPUT_IT first,
              size_t num,
              Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT> mapper,
              Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE> reducer);
    
    template <class KEY,
              class MAPPED_TYPE,
              class REDUCED_TYPE,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    typename Context<std::map<KEY, REDUCED_TYPE>>::Ptr
    mapReduceBatch(INPUT_IT first,
                   INPUT_IT last,
                   Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT> mapper,
                   Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE> reducer);
    
    template <class KEY,
              class MAPPED_TYPE,
              class REDUCED_TYPE,
              class INPUT_IT>
    typename Context<std::map<KEY, REDUCED_TYPE>>::Ptr
    mapReduceBatch(INPUT_IT first,
                   size_t num,
                   Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT> mapper,
                   Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE> reducer);
    
    //===================================
    //           NEW / DELETE
    //===================================
    static void* operator new(size_t size);
    static void operator delete(void* p);
    static void deleter(Context<RET>* p);
    
private:
    explicit Context(DispatcherCore& dispatcher);
    
    template <class OTHER_RET>
    Context(Context<OTHER_RET>& other);
    
    Context(IContextBase& other);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    thenImpl(ITask::Type type, FUNC&& func, ARGS&&... args);

    template <class OTHER_RET, class FUNC, class ... ARGS>
    typename Context<OTHER_RET>::Ptr
    postImpl(int queueId, bool isHighPriority, ITask::Type type, FUNC&& func, ARGS&&... args);
    
    template <class OTHER_RET, class FUNC, class ... ARGS>
    CoroFuturePtr<OTHER_RET>
    postAsyncIoImpl(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    int index(int num) const;
    
    void validateTaskType(ITask::Type type) const; //throws
    
    void validateContext(ICoroSync::Ptr sync) const; //throws
    
    //Members
    ITask::Ptr                          _task;
    std::vector<IPromiseBase::Ptr>      _promises;
    DispatcherCore*                     _dispatcher;
    std::atomic_bool                    _terminated;
    std::atomic_int                     _signal;
    Traits::Yield*                      _yield;
    std::chrono::microseconds           _sleepDuration;
    std::chrono::steady_clock::time_point  _sleepTimestamp;
};

template <class RET>
using ContextPtr = typename Context<RET>::Ptr;

}}

#include <quantum/interface/quantum_icontext.h>
#include <quantum/impl/quantum_context_impl.h>

#endif //BLOOMBERG_QUANTUM_CONTEXT_H
