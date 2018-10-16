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
#ifndef QUANTUM_DISPATCHER_H
#define QUANTUM_DISPATCHER_H

#include <quantum/quantum_context.h>
#include <quantum/quantum_configuration.h>
#include <quantum/quantum_macros.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class TaskDispatcher
//==============================================================================================
/// @class TaskDispatcher.
/// @brief Parallel execution engine used to run coroutines or IO tasks asynchronously.
///        This class is the main entry point into the library.
class TaskDispatcher : public ITerminate
{
public:
    /// @brief Constructor.
    /// @details This will build two thread pools, one used for running parallel coroutines and another
    ///          used for running blocking IO tasks.
    /// @param[in] numCoroutineThreads Number of parallel threads running coroutines. -1 indicates one per core.
    /// @param[in] numIoThreads Number of parallel threads running blocking IO calls.
    /// @param[in] pinCoroutineThreadsToCores If set to true, it will pin all coroutine threads unto physical cores.
    ///                                       provided numCoroutineThreads <= cores.
    /// @warning This constructor is deprecated and will be removed in v1.0. Use the configuration-based
    ///          constructor instead.
    DEPRECATED TaskDispatcher(int numCoroutineThreads = -1,
                              int numIoThreads = 5,
                              bool pinCoroutineThreadsToCores = false);
    
    /// @brief Constructor.
    /// @param config The configuration for the Quantum dispatcher.
    explicit TaskDispatcher(const Configuration& config);
    
    /// @brief Destructor.
    /// @details Destroys the task dispatcher object. This will wait until all coroutines complete, signal
    ///          all worker threads (coroutine and IO) to exit and join them.
    ~TaskDispatcher();
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadContext<RET>::Ptr
    post(FUNC&& func, ARGS&&... args);
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadContext<RET>::Ptr
    post(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadContext<RET>::Ptr
    postFirst(FUNC&& func, ARGS&&... args);
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadContext<RET>::Ptr
    postFirst(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadFuture<RET>::Ptr
    postAsyncIo(FUNC&& func, ARGS&&... args);
    
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
    template <class RET = int, class FUNC, class ... ARGS>
    typename ThreadFuture<RET>::Ptr
    postAsyncIo(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    /// @brief Applies the given unary function to all the elements in the range [first,last).
    ///        This function runs in parallel.
    /// @tparam RET The return value of the unary function.
    /// @tparam UNARY_FUNC A unary function of type <RET(*INPUT_IT)>.
    /// @tparam InputIt The type of iterator.
    /// @param first The first element in the range.
    /// @param last The last element in the range (exclusive).
    /// @param func The unary function.
    /// @return A vector of future values corresponding to the output of 'func' on every element in the range.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator
    /// @note Each func invocation will run inside its own coroutine instance.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<typename ThreadContext<RET>::Ptr>
    forEach(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func);
    
    /// @brief The synchronous version of the forEach() above.
    /// @return A vector of values corresponding to the output of 'func' on every element in the range.
    /// @note This method blocks until all values are returned.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<RET>
    forEachSync(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func);
    
    /// @brief Applies the given unary function to all the elements in the range [first,first+num).
    ///        This function runs in parallel.
    /// @tparam RET The return value of the unary function.
    /// @tparam UNARY_FUNC A unary function of type <RET(*INPUT_IT)>.
    /// @tparam InputIt The type of iterator.
    /// @param first The first element in the range.
    /// @param num The number of elements to iterate over.
    /// @param func The unary function.
    /// @return A vector of future values corresponding to the output of 'func' on every element in the range.
    /// @note Use this function if InputIt *does not* meet the requirement of a RandomAccessIterator.
    /// @note Each func invocation will run inside its own coroutine instance.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<typename ThreadContext<RET>::Ptr>
    forEach(INPUT_IT first, size_t num, UNARY_FUNC&& func);
    
    /// @brief The synchronous version of the forEach() above.
    /// @return A vector of values corresponding to the output of 'func' on every element in the range.
    /// @note This method blocks until all values are returned.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<RET>
    forEachSync(INPUT_IT first, size_t num, UNARY_FUNC&& func);
    
    /// @brief Applies the given unary function to all the elements in the range [first,last).
    ///        This function runs serially with respect to other functions in the same batch.
    /// @tparam RET The return value of the unary function.
    /// @tparam UNARY_FUNC A unary function of type <RET(*INPUT_IT)>.
    /// @tparam InputIt The type of iterator.
    /// @param first The first element in the range.
    /// @param last The last element in the range (exclusive).
    /// @param func The unary function.
    /// @return A vector of future value vectors corresponding to each execution batch.
    /// @note Use this function if InputIt meets the requirement of a RandomAccessIterator.
    /// @note The input range is split equally among coroutines and executed in batches. This function
    ///       achieves higher throughput rates than the non-batched mode, if func() is CPU-bound.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<typename ThreadContext<std::vector<RET>>::Ptr>
    forEachBatch(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func);
    
    /// @brief The synchronous version of the forEachBatch() above.
    /// @return A vector of values corresponding to the output of 'func' on every element in the range.
    /// @note This method blocks until all values are returned.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<RET>
    forEachBatchSync(INPUT_IT first, INPUT_IT last, UNARY_FUNC&& func);
    
    /// @brief Applies the given unary function to all the elements in the range [first,last).
    ///        This function runs serially with respect to other functions in the same batch.
    /// @tparam RET The return value of the unary function.
    /// @tparam UNARY_FUNC A unary function of type <RET(*INPUT_IT)>.
    /// @tparam InputIt The type of iterator.
    /// @param first The first element in the range.
    /// @param last The last element in the range (exclusive).
    /// @param func The unary function.
    /// @return A vector of future value vectors corresponding to each execution batch.
    /// @note Use this function if InputIt *does not* meet the requirement of a RandomAccessIterator.
    /// @note The input range is split equally among coroutines and executed in batches. This function
    ///       achieves higher throughput rates than the non-batched mode, if func() is CPU-bound.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<typename ThreadContext<std::vector<RET>>::Ptr>
    forEachBatch(INPUT_IT first, size_t num, UNARY_FUNC&& func);
    
    /// @brief The synchronous version of the forEachBatch() above.
    /// @return A vector of values corresponding to the output of 'func' on every element in the range.
    /// @note This method blocks until all values are returned.
    template <class RET = int, class UNARY_FUNC, class INPUT_IT>
    std::vector<RET>
    forEachBatchSync(INPUT_IT first, size_t num, UNARY_FUNC&& func);
    
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
    /// @note This function blocks until all coroutines and IO tasks have completed. During this time, posting
    ///       of new tasks is disabled unless they are posted from within an already executing coroutine.
    void drain();
    
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
    typename ThreadContext<RET>::Ptr
    postImpl(int queueId, bool isHighPriority, ITask::Type type, FUNC&& func, ARGS&&... args);
    
    template <class RET, class FUNC, class ... ARGS>
    typename ThreadFuture<RET>::Ptr
    postAsyncIoImpl(int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);
    
    //Members
    DispatcherCore              _dispatcher;
    bool                        _drain;
    std::atomic_flag            _terminated;
};

}}

#include <quantum/impl/quantum_dispatcher_impl.h>

#endif //QUANTUM_DISPATCHER_H

