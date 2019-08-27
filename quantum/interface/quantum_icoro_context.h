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
#ifndef BLOOMBERG_QUANTUM_ICORO_CONTEXT_H
#define BLOOMBERG_QUANTUM_ICORO_CONTEXT_H

#include <quantum/quantum_functions.h>
#include <quantum/interface/quantum_icoro_context_base.h>
#include <quantum/interface/quantum_icoro_future.h>
#include <map>
#include <vector>

namespace Bloomberg {
namespace quantum {

template <class RET>
class Context;

//==============================================================================================
//                                  interface ICoroContext
//==============================================================================================
/// @interface ICoroContext.
/// @brief Exposes methods to manipulate the coroutine context.
/// @tparam RET The type of value returned via the promise associated with this context.
template <class RET>
struct ICoroContext : ICoroContextBase
{
    using ContextTag = CoroContextTag;
    using Ptr = std::shared_ptr<ICoroContext<RET>>;
    using Impl = Context<RET>;
    
    /// @brief Get the future value associated with this context.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return The future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       becomes invalidated (i.e. cannot be read again).
    template <class V = RET>
    NonBufferRetType<V> get(ICoroSync::Ptr sync);
    
    /// @brief Get a reference the future value associated with this context.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return A reference to the future value.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to get(), this function does
    ///       not invalidate the future and as such may be read again.
    template <class V = RET>
    const NonBufferRetType<V>& getRef(ICoroSync::Ptr sync) const;
    
    /// @brief Get the future value associated with the previous coroutine context in the continuation chain.
    /// @tparam OTHER_RET The type of the future value of the previous context.
    /// @return The previous future value.
    /// @note Contrary to get() or getRef() this function never blocks since the previous coroutine is guaranteed
    ///       to have completed. Thus the future, if any, is already set. Once this function returns, the future becomes
    ///       invalidated.
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getPrev();
    
    /// @brief Get a reference to future value associated with the previous coroutine context in the continuation chain.
    /// @tparam OTHER_RET The type of the future value of the previous context.
    /// @return A reference to the previous future value.
    /// @note This function does not block. Contrary to getPrev() this function will not invalidate the future,
    ///       thus it can be read again.
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getPrevRef();
    
    /// @brief Get the future value from the 'num-th' continuation context.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling get() or
    ///          getAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @tparam OTHER_RET The type of the future value associated with the 'num-th' context.
    /// @param[in] num The number indicating which future to wait on.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return The future value of the 'num-th' coroutine context.
    /// @note Blocks until the future is ready or until an exception is thrown. Once this function returns, the future
    ///       is invalidated (i.e. cannot be read again).
    template <class OTHER_RET>
    NonBufferRetType<OTHER_RET> getAt(int num, ICoroSync::Ptr sync);
    
    /// @brief Get a reference to the future value from the 'num-th' continuation context.
    /// @details Allowed range for num is [-1, total_continuations). -1 is equivalent of calling get() or
    ///          getAt(total_continuations-1) on the last context in the chain (i.e. the context which is returned
    ///          via end()). Position 0 represents the first future in the chain.
    /// @tparam OTHER_RET The type of the future value associated with the 'num-th' context.
    /// @param[in] num The number indicating which future to wait on.
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @return A reference to the future value of the 'num-th' coroutine context.
    /// @note Blocks until the future is ready or until an exception is thrown. Contrary to getAt() this function will
    ///       not invalidate the future and as such it can be read again.
    template <class OTHER_RET>
    const NonBufferRetType<OTHER_RET>& getRefAt(int num, ICoroSync::Ptr sync) const;
    
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
    /// @param[in] sync Pointer to the coroutine synchronization object.
    /// @param[out] isBufferClosed Indicates if this buffer is closed and no more Pull operations are allowed on it.
    /// @return The next value pulled out from the front of the buffer.
    /// @note Method available for buffered futures only. Blocks until one value is retrieved from the buffer.
    template <class V = RET>
    BufferRetType<V> pull(ICoroSync::Ptr sync, bool& isBufferClosed);
    
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
    
    //-----------------------------------------------------------------------------------------
    //                                      TASK CONTINUATIONS
    //-----------------------------------------------------------------------------------------
    /// @attention
    /// Continuation methods are typically chained in the following manner and must follow the relative placement
    /// below. postFirst() and end() are the only mandatory methods. onError() and finally() can be called at most once,
    /// whereas then() may be called zero or more times.
    ///
    /// @code
    ///    ICoroContext<RET>::Ptr ctx = ICoroContext::postFirst()->then()->...->then()->onError()->finally()->end();
    /// @endcode
    ///
    /// @note post() methods are standalone and do not allow continuations.
    //-----------------------------------------------------------------------------------------
    
    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available. Typically it will pick one which has the
    ///          smallest number of concurrent coroutines executing at the time of the post.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and returns immediately. The returned context cannot be used to chain
    ///       further coroutines.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto post(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto post2(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on the specified queue (thread) with high or low priority.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of post() above. Valid range is
    ///                    [0, numCoroutineThreads), IQueue::QueueId::Any or IQueue::QueueId::Same. When using
    ///                    IQueue::QueueId::Same as input, the current queueId of this execution context will be
    ///                    preserved for the posted coroutine thus enabling to write lock-free code.
    /// @param[in] isHighPriority If set to true, the coroutine will be scheduled to run immediately after the currently
    ///                           executing coroutine on 'queueId' has completed or has yielded.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and returns immediately. The returned context cannot be used to chain
    ///       further coroutines.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto post2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a coroutine to run asynchronously.
    /// @details This function is the head of a coroutine continuation chain and must be called only once in the chain.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and returns immediately. The returned context can be used to chain
    ///       further coroutines. Possible method calls following this are then(), onError(), finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst2(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a coroutine to run asynchronously.
    /// @details This function is the head of a coroutine continuation chain and must be called only once in the chain.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of post() above. Valid range is
    ///                    [0, numCoroutineThreads), IQueue::QueueId::Any or IQueue::QueueId::Same. When using
    ///                    IQueue::QueueId::Same as input, the current queueId of this execution context will be
    ///                    preserved for the posted coroutine thus enabling to write lock-free code.
    /// @param[in] isHighPriority If set to true, the coroutine will be scheduled to run immediately after the currently
    ///                           executing coroutine on 'queueId' has completed or has yielded.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and returns immediately. The returned context can be used to chain
    ///       further coroutines. Possible method calls following this are then(), onError(), finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a coroutine to run asynchronously.
    /// @details This function is optional for the continuation chain and may be called 0 or more times. If called,
    ///          it must follow postFirst() or another then() method.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and runs when all previous chained coroutines have completed.
    ///       The returned context can be used to chain further coroutines. Possible method calls following this
    ///       are then(), onError(), finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto then(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto then2(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a coroutine to run asynchronously. This is the error handler for a continuation chain and acts as
    ///        as a 'catch' clause.
    /// @details This function is optional for the continuation chain and may be called at most once. If called,
    ///          it must follow postFirst() or another then() method. This method will conditionally run if-and-only-if
    ///          any previous coroutines in the continuation chain return an error or throw. When a coroutine which is
    ///          part of a continuation chain has an error, all subsequent then() methods are skipped and if onError()
    ///          is provided it will be called. If there are no errors, this method is skipped.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note The function is non-blocking. The returned context can be used to chain further coroutines.
    ///       Possible method calls following this are finally() and end().
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto onError(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto onError2(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief Posts a coroutine to run asynchronously. This coroutine is always guaranteed to run.
    /// @details This function is optional for the continuation chain and may be called at most once. If called, it must
    ///          immediately precede the end() method. This method will run regardless if any preceding coroutines have
    ///          an error or not. It can be used for cleanup purposes, closing handles, terminating services, etc.
    /// @tparam OTHER_RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine context object.
    /// @note This function is non-blocking and returns immediately. After this coroutine, the end() method must be called.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto finally(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(coroResult(func))>::Ptr;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto finally2(FUNC&& func, ARGS&&... args)->typename ICoroContext<decltype(resultOf2(func))>::Ptr;
    
    /// @brief This is the last method in a continuation chain.
    /// @details This method effectively closes the continuation chain and posts the entire chain to be executed,
    ///          respecting the 'queueId' and priority specified at the beginning of the chain (see postFirst()).
    /// @return Pointer to this context.
    /// @note This method does not take any functions as parameter as it is strictly used for scheduling purposes.
    Ptr end();
    
    /// @brief Posts an IO method to run asynchronously on the IO thread pool.
    /// @tparam OTHER_RET Type of future returned by this function.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method, an std::function, a functor
    ///              generated via std::bind or a lambda. The signature of the callable object must strictly be
    ///              'int f(ThreadPromise<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine future object which may be used to retrieve the result of the IO operation.
    /// @note This method does not block. The passed function will not be wrapped in a coroutine.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo(FUNC&& func, ARGS&&... args)->CoroFuturePtr<decltype(ioResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler IO task signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo2(FUNC&& func, ARGS&&... args)->CoroFuturePtr<decltype(resultOf2(func))>;
    
    /// @brief Posts an IO function to run asynchronously on the IO thread pool.
    /// @details This method will post the function on the specified queue (thread) with high or low priority.
    /// @tparam OTHER_RET Type of future returned by this function.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method, an std::function, a functor
    ///              generated via std::bind or a lambda. The signature of the callable object must strictly be
    ///              'int f(ThreadPromise<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this function should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of postAsyncIo() above. Valid range is
    ///                    [0, numIoThreads) or IQueue::QueueId::Any. IQueue::QueueId::Same is disallowed.
    /// @param[in] isHighPriority If set to true, the function will be scheduled to run immediately after the currently
    ///                           executing function on 'queueId' has completed.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a coroutine future object which may be used to retrieve the result of the IO operation.
    /// @note This method does not block. The passed function will not be wrapped in a coroutine.
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->CoroFuturePtr<decltype(ioResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler IO task signature (see documentation).
    template <class OTHER_RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->CoroFuturePtr<decltype(resultOf2(func))>;
    
    /// @brief Applies the given unary function to all the elements in the range [first,last).
    ///        This function runs in parallel.
    /// @tparam OTHER_RET The return value of the unary function.
    /// @tparam INPUT_IT The type of iterator.
    /// @tparam FUNC A unary function of type 'RET(VoidContextPtr, *INPUT_IT)'.
    /// @param[in] first The first element in the range.
    /// @param[in] last The last element in the range (exclusive).
    /// @param[in] func The unary function.
    /// @return A vector of future values corresponding to the output of 'func' on every element in the range.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator
    /// @note Each func invocation will run inside its own coroutine instance.
    ///       Prefer this function over forEachBatch() if performing IO inside FUNC.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class OTHER_RET = Deprecated,
              class INPUT_IT,
              class FUNC,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto forEach(INPUT_IT first, INPUT_IT last, FUNC&& func)
        ->typename ICoroContext<std::vector<decltype(coroResult(func))>>::Ptr;
    
    /// @brief Same as forEach() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class OTHER_RET = Deprecated,
              class INPUT_IT,
              class FUNC>
    auto forEach(INPUT_IT first, size_t num, FUNC&& func)
        ->typename ICoroContext<std::vector<decltype(coroResult(func))>>::Ptr;
    
    /// @brief The batched version of forEach(). This function applies the given unary function
    ///        to all the elements in the range [first,last). This function runs serially with respect
    ///        to other functions in the same batch.
    /// @return A vector of value vectors (i.e. one per batch).
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator.
    /// @note The input range is split equally among coroutines and executed in batches. This function
    ///       achieves higher throughput rates than the non-batched mode, if FUNC is CPU-bound.
    template <class OTHER_RET = Deprecated,
              class INPUT_IT,
              class FUNC,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto forEachBatch(INPUT_IT first, INPUT_IT last, FUNC&& func)
        ->typename ICoroContext<std::vector<std::vector<decltype(coroResult(func))>>>::Ptr;
    
    /// @brief Same as forEachBatch() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class OTHER_RET = Deprecated,
              class INPUT_IT,
              class FUNC>
    auto forEachBatch(INPUT_IT first, size_t num, FUNC&& func)
        ->typename ICoroContext<std::vector<std::vector<decltype(coroResult(func))>>>::Ptr;
    
    /// @brief Implementation of map-reduce functionality.
    /// @tparam KEY The KEY type used for mapping and reducing.
    /// @tparam MAPPED_TYPE The output type after a map operation.
    /// @tparam REDUCED_TYPE The output type after a reduce operation.
    /// @tparam MAPPER_FUNC The mapper function having the signature
    ///         'std::vector<std::pair<KEY,MAPPED_TYPE>>(VoidContextPtr, *INPUT_IT)'
    /// @tparam REDUCER_FUNC The reducer function having the signature
    ///         'std::pair<KEY,REDUCED_TYPE>(VoidContextPtr, std::pair<KEY, std::vector<MAPPED_TYPE>>&&)'
    /// @tparam INPUT_IT The iterator type.
    /// @oaram[in] first The start iterator to a list of items to be processed in the range [first,last).
    /// @oaram[in] last The end iterator to a list of items (not inclusive).
    /// @oaram[in] mapper The mapper function.
    /// @oaram[in] reducer The reducer function.
    /// @return A future to a reduced map of values.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class KEY = Deprecated,
              class MAPPED_TYPE = Deprecated,
              class REDUCED_TYPE = Deprecated,
              class MAPPER_FUNC,
              class REDUCER_FUNC,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto mapReduce(INPUT_IT first,
                   INPUT_IT last,
                   MAPPER_FUNC mapper,
                   REDUCER_FUNC reducer)->
          typename ICoroContext<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>::Ptr;
  
    /// @brief Same as mapReduce() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class KEY = Deprecated,
              class MAPPED_TYPE = Deprecated,
              class REDUCED_TYPE = Deprecated,
              class MAPPER_FUNC,
              class REDUCER_FUNC,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto mapReduce(INPUT_IT first,
                   size_t num,
                   MAPPER_FUNC mapper,
                   REDUCER_FUNC reducer)->
          typename ICoroContext<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>::Ptr;
    
    /// @brief This version of mapReduce() runs both the mapper and the reducer functions in batches
    ///        for improved performance. This should be used in the case where the functions are
    ///        more CPU intensive with little or no IO.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator.
    template <class KEY = Deprecated,
              class MAPPED_TYPE = Deprecated,
              class REDUCED_TYPE = Deprecated,
              class MAPPER_FUNC,
              class REDUCER_FUNC,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto mapReduceBatch(INPUT_IT first,
                        INPUT_IT last,
                        MAPPER_FUNC mapper,
                        REDUCER_FUNC reducer)->
          typename ICoroContext<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>::Ptr;
    
    /// @brief Same as mapReduceBatch() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class KEY = Deprecated,
              class MAPPED_TYPE = Deprecated,
              class REDUCED_TYPE = Deprecated,
              class MAPPER_FUNC,
              class REDUCER_FUNC,
              class INPUT_IT,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto mapReduceBatch(INPUT_IT first,
                        size_t num,
                        MAPPER_FUNC mapper,
                        REDUCER_FUNC reducer)->
          typename ICoroContext<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>::Ptr;
};

template <class RET>
using CoroContext = ICoroContext<RET>;

template <class RET>
using CoroContextPtr = typename ICoroContext<RET>::Ptr;
using VoidCoroContextPtr = CoroContextPtr<Void>;
using VoidContextPtr = VoidCoroContextPtr; //shorthand version

template <class RET>
struct Traits::InnerType<std::shared_ptr<ICoroContext<RET>>> { using Type = RET;};
template <>
struct Traits::IsVoidContext<std::shared_ptr<ICoroContext<Void>>> : std::true_type {};

}}

#endif //BLOOMBERG_QUANTUM_ICORO_CONTEXT_H
