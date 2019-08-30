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
#ifndef BLOOMBERG_QUANTUM_DISPATCHER_H
#define BLOOMBERG_QUANTUM_DISPATCHER_H

#include <quantum/quantum_context.h>
#include <quantum/quantum_configuration.h>
#include <quantum/quantum_macros.h>
#include <quantum/util/quantum_drain_guard.h>
#include <iterator>
#include <chrono>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class Dispatcher
//==============================================================================================
/// @class Dispatcher.
/// @brief Parallel execution engine used to run coroutines or IO tasks asynchronously.
///        This class is the main entry point into the library.
class Dispatcher : public ITerminate
{
public:
    using ContextTag = ThreadContextTag;
    
    /// @brief Constructor.
    /// @oaram[in] config The configuration for the Quantum dispatcher.
    explicit Dispatcher(const Configuration& config);
    
    /// @brief Destructor.
    /// @details Destroys the task dispatcher object. This will wait until all coroutines complete, signal
    ///          all worker threads (coroutine and IO) to exit and join them.
    ~Dispatcher();
    
    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available. Typically it will pick one which has the
    ///          smallest number of concurrent coroutines executing at the time of the post.
    /// @tparam RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread context object.
    /// @note This function is non-blocking and returns immediately. The returned thread context cannot be used to chain
    ///       further coroutines.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto post(FUNC&& func, ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto post2(FUNC&& func, ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>;
    
    /// @brief Post a coroutine to run asynchronously on a specific queue (thread).
    /// @tparam RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of post() above. Valid range is
    ///                    [0, numCoroutineThreads) or IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the coroutine will be scheduled to run immediately after the currently
    ///                           executing coroutine on 'queueId' has completed or has yielded.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread context object.
    /// @note This function is non-blocking and returns immediately. The returned thread context cannot be used to chain
    ///       further coroutines.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadContextPtr<decltype(coroResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto post2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadContextPtr<decltype(resultOf2(func))>;
    
    /// @brief Post the first coroutine in a continuation chain to run asynchronously.
    /// @tparam RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread context object.
    /// @note This function is non-blocking and returns immediately. The returned context can be used to chain other
    ///       coroutines which will run sequentially.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst(FUNC&& func, ARGS&&... args)->ThreadContextPtr<decltype(coroResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst2(FUNC&& func, ARGS&&... args)->ThreadContextPtr<decltype(resultOf2(func))>;
    
    /// @brief Post the first coroutine in a continuation chain to run asynchronously on a specific queue (thread).
    /// @tparam RET Type of future returned by this coroutine.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine. Can be a standalone function, a method,
    ///              an std::function, a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(CoroContext<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of post() above. Valid range is
    ///                    [0, numCoroutineThreads) or IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the coroutine will be scheduled to run immediately after the currently
    ///                           executing coroutine on 'queueId' has completed or has yielded.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread context object.
    /// @note This function is non-blocking and returns immediately. The returned context can be used to chain other
    ///       coroutines which will run sequentially.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadContextPtr<decltype(coroResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler coroutine signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postFirst2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadContextPtr<decltype(resultOf2(func))>;
    
    /// @brief Post a blocking IO (or long running) task to run asynchronously on the IO thread pool.
    /// @tparam RET Type of future returned by this task.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method, an std::function,
    ///              a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(ThreadPromise<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread future object.
    /// @note This function is non-blocking and returns immediately. The passed function will not be wrapped in a coroutine.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo(FUNC&& func, ARGS&&... args)->ThreadFuturePtr<decltype(ioResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler IO task signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo2(FUNC&& func, ARGS&&... args)->ThreadFuturePtr<decltype(resultOf2(func))>;
    
    /// @brief Post a blocking IO (or long running) task to run asynchronously on a specific thread in the IO thread pool.
    /// @tparam RET Type of future returned by this task.
    /// @tparam FUNC Callable object type. Can be a standalone function, a method, an std::function,
    ///              a functor generated via std::bind or a lambda. The signature of the callable
    ///              object must strictly be 'int f(ThreadPromise<RET>::Ptr, ...)'.
    /// @tparam ARGS Argument types passed to FUNC.
    /// @param[in] queueId Id of the queue where this task should run. Note that the user can specify IQueue::QueueId::Any
    ///                    as a value, which is equivalent to running the simpler version of postAsyncIo() above. Valid range is
    ///                    [0, numCoroutineThreads) or IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the task will be scheduled to run immediately.
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @return A pointer to a thread future object.
    /// @note This function is non-blocking and returns immediately. The passed function will not be wrapped in a coroutine.
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadFuturePtr<decltype(ioResult(func))>;
    
    /// @brief Version 2 of the API which supports a simpler IO task signature (see documentation).
    template <class RET = Deprecated, class FUNC, class ... ARGS>
    auto postAsyncIo2(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args)
        ->ThreadFuturePtr<decltype(resultOf2(func))>;
    
    /// @brief Applies the given unary function to all the elements in the range [first,last).
    ///        This function runs in parallel.
    /// @tparam RET The return value of the unary function.
    /// @tparam INPUT_IT The type of iterator.
    /// @tparam FUNC A unary function of type 'RET(VoidContextPtr, *INPUT_IT)'.
    /// @oaram[in] first The first element in the range.
    /// @oaram[in] last The last element in the range (exclusive).
    /// @oaram[in] func The unary function.
    /// @return A vector of future values corresponding to the output of 'func' on every element in the range.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator
    /// @note Each func invocation will run inside its own coroutine instance.
    ///       Prefer this function over forEachBatch() if performing IO inside FUNC.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class RET = Deprecated,
              class INPUT_IT,
              class FUNC,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto forEach(INPUT_IT first, INPUT_IT last, FUNC&& func)
        ->ThreadContextPtr<std::vector<decltype(coroResult(func))>>;
    
    /// @brief Same as forEach() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class RET = Deprecated,
              class INPUT_IT,
              class FUNC>
    auto forEach(INPUT_IT first, size_t num, FUNC&& func)
        ->ThreadContextPtr<std::vector<decltype(coroResult(func))>>;
    
    /// @brief The batched version of forEach(). This function applies the given unary function
    ///        to all the elements in the range [first,last). This function runs serially with respect
    ///        to other functions in the same batch.
    /// @return A vector of value vectors (i.e. one per batch).
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator.
    /// @note The input range is split equally among coroutines and executed in batches. This function
    ///       achieves higher throughput rates than the non-batched mode, if FUNC is CPU-bound.
    template <class RET = Deprecated,
              class INPUT_IT,
              class FUNC,
              class = Traits::IsInputIterator<INPUT_IT>>
    auto forEachBatch(INPUT_IT first, INPUT_IT last, FUNC&& func)
        ->ThreadContextPtr<std::vector<std::vector<decltype(coroResult(func))>>>;
    
    /// @brief Same as forEachBatch() but takes a length as second argument in case INPUT_IT
    ///        is not a random access iterator.
    template <class RET = Deprecated,
              class INPUT_IT,
              class FUNC>
    auto forEachBatch(INPUT_IT first, size_t num, FUNC&& func)
        ->ThreadContextPtr<std::vector<std::vector<decltype(coroResult(func))>>>;
    
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
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>;
  
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
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>;
    
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
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>;
    
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
          ThreadContextPtr<std::map<decltype(mappedKeyOf(mapper)), decltype(reducedTypeOf(reducer))>>;

    /// @brief Signal all threads to immediately terminate and exit. All other pending coroutines and IO tasks will not complete.
    ///        Call this function for a fast shutdown of the dispatcher.
    /// @note This function blocks.
    void terminate() final;
    
    /// @brief Returns the total number of queued tasks for the specified type and queue id.
    /// @param[in] type The type of queue.
    /// @param[in] queueId The queue number to query. Valid range is [0, numCoroutineThreads) for IQueue::QueueType::Coro,
    ///                    [0, numIoThreads) for IQueue::QueueType::IO and IQueue::QueueId::All for either.
    /// @return The total number of queued tasks including the currently executing one.
    /// @note IQueue::QueueId::Same is an invalid queue id. IQueue::QueueId::Any is only valid for IO queue type. When
    ///       type IQueue::QueueType::All is specified, the queueId is not used and must be left at default value.
    size_t size(IQueue::QueueType type = IQueue::QueueType::All,
                int queueId = (int)IQueue::QueueId::All) const;
    
    /// @brief Check if the specified type and queue id is empty (i.e. there are no running tasks)
    /// @param[in] type The type of queue.
    /// @param[in] queueId The queue number to query. Valid range is [0, numCoroutineThreads) for IQueue::QueueType::Coro,
    ///                    [0, numIoThreads) for IQueue::QueueType::IO and IQueue::QueueId::All for either.
    /// @return True if empty, false otherwise.
    /// @note IQueue::QueueId::Same is an invalid queue id. IQueue::QueueId::Any is only valid for IO queue type. When
    ///       type IQueue::QueueType::All is specified, the queueId is not used and must be left at default value.
    bool empty(IQueue::QueueType type = IQueue::QueueType::All,
               int queueId = (int)IQueue::QueueId::All) const;
    
    /// @brief Drains all queues on this dispatcher object.
    /// @param[in] timeout Maximum time for this function to wait. Set to 0 to wait indefinitely until all queues drain.
    /// @param[in] isFinal If set to true, the dispatcher will not allow any more processing after the drain completes.
    /// @note This function blocks until all coroutines and IO tasks have completed. During this time, posting
    ///       of new tasks is disabled unless they are posted from within an already executing coroutine.
    void drain(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero(),
               bool isFinal = false);
    
    /// @brief Returns the number of underlying coroutine threads as specified in the constructor. If -1 was passed
    ///        than this number essentially indicates the number of cores.
    /// @return The number of threads.
    /// @note Each thread services its own queueId, therefore this number can be used when assigning coroutines
    ///       to a specific queue.
    int getNumCoroutineThreads() const;
    
    /// @brief Returns the number of underlying IO threads as specified in the constructor.
    /// @return The number of threads.
    /// @note Each thread services its own queueId, therefore this number can be used when assigning IO tasks
    ///       to a specific queue.
    int getNumIoThreads() const;

    /// @brief Gets the range [minQueueId, maxQueueId] of coroutine queueIds covered by IQueue::QueueId::Any
    /// when using Dispatcher::post
    /// @return queueIdRange The range of queueIds that IQueue::QueueId::Any covers
    const std::pair<int, int>& getCoroQueueIdRangeForAny() const;
    
    /// @brief Returns a statistics object for the specified type and queue id.
    /// @param[in] type The type of queue.
    /// @param[in] queueId The queue number to query. Valid range is [0, numCoroutineThreads) for IQueue::QueueType::Coro,
    ///                    [0, numIoThreads) for IQueue::QueueType::IO and IQueue::QueueId::All for either.
    /// @return Aggregated or individual queue stats.
    /// @note IQueue::QueueId::Same is an invalid queue id. IQueue::QueueId::Any is only valid for IO queue type. When
    ///       type IQueue::QueueType::All is specified, the queueId is not used and must be left at default value.
    QueueStatistics stats(IQueue::QueueType type = IQueue::QueueType::All,
                          int queueId = (int)IQueue::QueueId::All);
    
    /// @brief Resets all coroutine and IO queue counters.
    void resetStats();
    
private:
    template <class RET, class FUNC, class ... ARGS>
    ThreadContextPtr<RET>
    postImpl(int queueId, bool isHighPriority, ITask::Type type, FUNC&& func, ARGS&&... args);
    
    template <class RET, class FUNC, class ... ARGS>
    ThreadFuturePtr<RET>
    postAsyncIoImpl(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    //Members
    DispatcherCore              _dispatcher;
    std::atomic_bool            _drain;
    std::atomic_bool            _terminated;
};

using TaskDispatcher = Dispatcher; //alias

}}

#include <quantum/impl/quantum_dispatcher_impl.h>

#endif //BLOOMBERG_QUANTUM_DISPATCHER_H
