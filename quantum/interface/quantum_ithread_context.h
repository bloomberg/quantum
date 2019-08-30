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
#ifndef BLOOMBERG_QUANTUM_ITHREAD_CONTEXT_H
#define BLOOMBERG_QUANTUM_ITHREAD_CONTEXT_H

#include <future>
#include <chrono>
#include <quantum/quantum_traits.h>
#include <quantum/interface/quantum_ithread_context_base.h>

namespace Bloomberg {
namespace quantum {

template <class RET>
class Context;

//==============================================================================================
//                                      interface IThreadContext
//==============================================================================================
/// @interface IThreadContext
/// @brief Exposes methods to manipulate the thread context.
/// @tparam RET The type of value returned via the promise associated with this context.
template <class RET>
struct IThreadContext : public IThreadContextBase
{
    using ContextTag = ThreadContextTag;
    using Ptr = std::shared_ptr<IThreadContext<RET>>;
    using Impl = Context<RET>;
    
    /// @brief Waits for the future associated with this context to be ready.
    /// @note Blocks until the future is ready or until an exception is thrown.
    virtual void wait() const = 0;
    
    /// @brief Waits for the future associated with this context to be ready for a maximum of 'timeMs' milliseconds.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitFor(std::chrono::milliseconds timeMs) const = 0;
    
    /// @brief Waits for the future in the 'num-th' continuation context to be ready.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling wait() or
    ///          waitAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @param[in] num The number indicating which future to wait on.
    /// @note Blocks until the value is ready or an exception is thrown.
    virtual void waitAt(int num) const = 0;
    
    /// @brief Waits for the future in the 'num-th' continuation context to be ready for a maximum of 'timeMs' milliseconds.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling wait() or
    ///          waitAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @param[in] num The number indicating which future to wait on.
    /// @param[in] timeMs The maximum amount of milliseconds to wait until the future value becomes ready.
    /// @return 'ready' if value was posted before duration expired or 'timeout' otherwise.
    /// @note Blocks until the value is ready, until 'timeMs' duration expires or until an exception is thrown.
    virtual std::future_status waitForAt(int num, std::chrono::milliseconds timeMs) const = 0;
    
    /// @brief Wait for all the futures in the continuation chain to be ready.
    /// @note Blocks until all future values are ready. If any future throws, the exception is swallowed.
    virtual void waitAll() const = 0;
    
    /// @brief Get the future value associated with this context.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       becomes invalidated (i.e. cannot be read again).
    template <class V = RET>
    NonBufferRetType<V> get();
    
    /// @brief Get a reference the future value associated with this context.
    /// @return A reference to the future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to get(), this function does
    ///       not invalidate the future and as such may be read again.
    template <class V = RET>
    const NonBufferRetType<V>& getRef() const;
    
    /// @brief Get the future value from the 'num-th' continuation context.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling get() or
    ///          getAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @tparam OTHER_RET The type of the future value associated with the 'num-th' context.
    /// @param[in] num The number indicating which future to wait on.
    /// @return The future value of the 'num-th' thread context.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       is invalidated (i.e. cannot be read again).
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getAt(int num);
    
    /// @brief Get a reference to the future value from the 'num-th' continuation context.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling get() or
    ///          getAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @tparam OTHER_RET The type of the future value associated with the 'num-th' context.
    /// @param[in] num The number indicating which future to wait on.
    /// @return A reference to the future value of the 'num-th' thread context.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to getAt() this function will
    ///       not invalidate the future and as such it can be read again.
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getRefAt(int num) const;
    
    /// @brief Set the promised value associated with this context.
    /// @tparam V Type of the promised value. This should be implicitly deduced by the compiler and should always == RET.
    /// @param[in] value A reference to the value (l-value or r-value).
    /// @note Never blocks.
    /// @return 0 on success
    template <class V, class = NonBufferType<RET,V>>
    int set(V&& value);
    
    /// @brief Push a single value into the promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[in] value Value to push at the end of the buffer.
    /// @note Method available for buffered futures only. Never blocks. Once the buffer is closed, no more Push
    ///       operations are allowed.
    template <class V, class = BufferType<RET,V>>
    void push(V&& value);
    
    /// @brief Pull a single value from the future buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @tparam V The type of value contained in Buffer.
    /// @param[out] isBufferClosed Indicates if this buffer is closed and no more Pull operations are allowed on it.
    /// @return The next value pulled out from the front of the buffer.
    /// @note Method available for buffered futures only. Blocks until one value is retrieved from the buffer.
    template <class V = RET>
    BufferRetType<V> pull(bool& isBufferClosed);
    
    /// @brief Close a promise buffer.
    /// @tparam BUF Represents a class of type Buffer.
    /// @note Once closed no more Pushes can be made into the buffer. The corresponding future can still Pull values until
    ///       the buffer is empty.
    /// @return 0 on success.
    template <class V = RET, class = BufferRetType<V>>
    int closeBuffer();
    
    /// @brief Returns the number of underlying coroutine threads as specified in the dispatcher constructor.
    ///        If -1 was passed than this number essentially indicates the number of cores.
    /// @return The number of threads.
    /// @note Each thread services its own queueId, therefore this number can be used when assigning coroutines
    ///       to a specific queue.
    int getNumCoroutineThreads() const;
    
    /// @brief Returns the number of underlying IO threads as specified in the dispatcher constructor.
    /// @return The number of threads.
    /// @note Each thread services its own queueId, therefore this number can be used when assigning IO tasks
    ///       to a specific queue.
    int getNumIoThreads() const;

    /// @brief Gets the range [minQueueId, maxQueueId] of coroutine queueIds covered by IQueue::QueueId::Any
    /// by the Dispatcher.
    /// @return queueIdRange The range of queueIds that IQueue::QueueId::Any covers
    const std::pair<int, int>& getCoroQueueIdRangeForAny() const;
    
    //=========================================================================================
    //                                      TASK CONTINUATIONS
    //=========================================================================================
    /// @attention
    /// Continuation methods are typically chained in the following manner and must follow the relative placement
    /// below. postFirst() and end() are the only mandatory methods. onError() and finally() can be called at most once,
    /// whereas then() may be called zero or more times.
    ///
    /// @code
    ///    IThreadContext<RET>::Ptr ctx = Dispatcher::postFirst()->then()->...->then()->onError()->finally()->end();
    /// @endcode
    //-----------------------------------------------------------------------------------------
    
    /// @brief Posts a function to run asynchronously.
    /// @details This function is optional for the continuation chain and may be called 0 or more times. If called,
    ///          it must follow postFirst() or another then() method.
    /// @tparam OTHER_RET Type of future returned by this function.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(ThreadContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and runs when all previous chained functions have completed.
    ///       The returned context can be used to chain further functions. Possible method calls following this
    ///       are then(), onError(), finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto then(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto then2(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a function to run asynchronously. This is the error handler for a continuation chain and acts as
    ///        as a 'catch' clause.
    /// @details This function is optional for the continuation chain and may be called at most once. If called,
    ///          it must follow postFirst() or another then() method. This method will conditionally run if-and-only-if
    ///          any previous functions in the continuation chain return an error or throw. When a function which is
    ///          part of a continuation chain has an error, all subsequent then() methods are skipped and if onError()
    ///          is provided it will be called. If there are no errors, this method is skipped.
    /// @tparam OTHER_RET Type of future returned by this function.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(ThreadContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note The function is non-blocking. The returned context can be used to chain further functions.
    ///       Possible method calls following this are finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto onError(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto onError2(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a function to run asynchronously. This function is always guaranteed to run.
    /// @details This function is optional for the continuation chain and may be called at most once. If called, it must
    ///          immediately precede the end() method. This method will run regardless if any preceding functions have
    ///          an error or not. It can be used for cleanup purposes, closing handles, terminating services, etc.
    /// @tparam OTHER_RET Type of future returned by this function.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately. After this function, the end() method must be called.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto finally(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto finally2(FUNC&& func, ARGS&&... args)->typename IThreadContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief This is the last method in a continuation chain.
    /// @details This method effectively closes the continuation chain and posts the entire chain to be executed,
    ///          respecting the 'queueId' and priority specified at the beginning of the chain (see postFirst()).
    /// @return Pointer to this context.
    /// @note This method does not take any functions as parameter as it is strictly used for scheduling purposes.
    Ptr end();
};

template <class RET>
using ThreadContext = IThreadContext<RET>;

template <class RET>
using ThreadContextPtr = typename IThreadContext<RET>::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ITHREAD_CONTEXT_H
