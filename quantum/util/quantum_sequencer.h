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
#ifndef BLOOMBERG_QUANTUM_SEQUENCER_H
#define BLOOMBERG_QUANTUM_SEQUENCER_H

#include <quantum/quantum_dispatcher.h>
#include <quantum/interface/quantum_ithread_context_base.h>
#include <quantum/util/quantum_sequencer_configuration.h>
#include <quantum/util/quantum_sequence_key_statistics.h>
#include <vector>
#include <unordered_map>
 
namespace Bloomberg {
namespace quantum {
 
//==============================================================================================
//                                      class Sequencer
//==============================================================================================
/// @class Sequencer.
/// @brief Implementation of a key-based task sequencing with quantum.
/// @tparam SequenceKey Type of the key based that sequenced tasks are associated with
/// @tparam Hash Hash-function used for storing instances of SequenceKey in hash maps
/// @tparam KeyEqual The equal-function used for storing instances of SequenceKey in hash maps
/// @tparam Allocator The allocator used for storing instances of SequenceKey in hash maps
    
template <class SequenceKey,
          class Hash = std::hash<SequenceKey>,
          class KeyEqual = std::equal_to<SequenceKey>,
          class Allocator = std::allocator<std::pair<const SequenceKey, SequenceKeyData>>>
class Sequencer
{
public:
    /// @brief Configuration class for Sequencer
    using Configuration = SequencerConfiguration<SequenceKey, Hash, KeyEqual, Allocator>;

    /// @brief Constructor.
    /// @param[in] dispatcher Dispatcher for all task dispatching
    /// @param[in] configuration the configuration object
    Sequencer(Dispatcher& dispatcher, const Configuration& configuration = Configuration());

    /// @brief Enqueue a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available and will run when the previous coroutine
    ///          associated with the same 'sequenceKey' completes. If there are none, it will run immediately.
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] sequenceKey SequenceKey object that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id (@see SequencerConfiguration::setControlQueueId).
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueue(const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args);

    /// @brief Enqueue a coroutine to run asynchronously on a specific queue (thread).
    /// @details This method will post the coroutine on any thread available and will run when the previous coroutine
    ///          associated with the same 'sequenceKey' completes. If there are none, it will run immediately.
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can
    ///                    specify IQueue::QueueId::Any as a value, which is equivalent to running
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception handler (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] sequenceKey SequenceKey object that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, queueId is suggested to be
    ///       different from the control queue id (@see SequencerConfiguration::setControlQueueId). Hence, if
    ///       IQueue::QueueId::Any is intended to be used as queueId here, then it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueue(void* opaque, int queueId, bool isHighPriority, const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args);

    /// @brief Enqueue a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available and will run when the previous coroutine(s)
    ///          associated with all the 'sequenceKeys' complete. If there are none, then it will run immediately.
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] sequenceKeys A collection of sequenceKey objects that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id (@see SequencerConfiguration::setControlQueueId).
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueue(const std::vector<SequenceKey>& sequenceKeys, FUNC&& func, ARGS&&... args);

    /// @brief Enqueue a coroutine to run asynchronously on a specific queue (thread).
    /// @details This method will post the coroutine on any thread available and will run when the previous coroutine(s)
    ///          associated with all the 'sequenceKeys' complete. If there are none, then it will run immediately.
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user 
    ///                    can specify IQueue::QueueId::Any as a value, which is equivalent to running 
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or 
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception handler (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] sequenceKeys A collection of sequenceKey objects that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, queueId is suggested to be
    ///       different from the control queue id (@see SequencerConfiguration::setControlQueueId). Hence, if
    ///       IQueue::QueueId::Any is intended to be used as queueId here, then it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueue(void* opaque,
            int queueId,
            bool isHighPriority,
            const std::vector<SequenceKey>& sequenceKeys,
            FUNC&& func,
            ARGS&&... args);

    /// @brief Enqueue a coroutine to run asynchronously after all keys have run.
    /// @details This method will post the coroutine on any thread available. The posted task is assumed to be associated
    ///          with the entire universe of sequenceKeys already running or pending, which means that it will wait
    ///          until all tasks complete. This task can be considered as having a 'universal' key.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id (@see SequencerConfiguration::setControlQueueId).
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueueAll(FUNC&& func, ARGS&&... args);

    /// @brief Enqueue a coroutine to run asynchronously on a specific queue (thread), after all keys have run.
    /// @details This method will post the coroutine on any thread available. The posted task is assumed to be associated
    ///          with the entire universe of sequenceKeys already running or pending, which means that it will wait
    ///          until all tasks complete. This task can be considered as having a 'universal' key.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine with signature 'int(VoidContextPtr, Args...)'
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user 
    ///                    can specify IQueue::QueueId::Any as a value, which is equivalent to running 
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or 
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception handler (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    /// @note For lowering the latencies of processing tasks posted here, queueId is suggested to be
    ///       different from the control queue id (@see SequencerConfiguration::setControlQueueId). Hence, if
    ///       IQueue::QueueId::Any is intended to be used as queueId here, then it is suggested that the configured
    ///       Any-coroutine-queue-range (@see Configuration::setCoroQueueIdRangeForAny) does not contain
    ///       the control queue id.
    /// @warning The VoidContextPtr can be used to yield() or to post additional coroutines or IO tasks.
    ///          However it should *not* be set and this will result in undefined behavior.
    template <class FUNC, class ... ARGS>
    void
    enqueueAll(void* opaque, int queueId, bool isHighPriority, FUNC&& func, ARGS&&... args);

    /// @brief Trims the sequence keys not used by the sequencer anymore.
    /// @details It's recommended to call this function periodically to clean up state sequence keys.
    /// @remark This call clears all the statistics for trimmed keys. 
    /// @return The number of sequenceKeys after the trimming.
    /// @note This function blocks until the trimming job posted to the dispatcher is finished
    size_t trimSequenceKeys();

    /// @brief Gets the number of tracked sequence keys
    /// @return sequence key count
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    size_t getSequenceKeyCount();

    /// @brief Gets the sequencer statistics for a specific sequence key
    /// @param sequenceKey the key 
    /// @return the statistics objects for the specified key
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    SequenceKeyStatistics getStatistics(const SequenceKey& sequenceKey);

    /// @brief Gets the sequencer statistics for the 'universal key', a.k.a. posted via postAll() method.
    /// @return the statistics objects
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    SequenceKeyStatistics getStatistics();
    
    /// @brief Gets the sequencer statistics for all jobs.
    /// @return the statistics objects
    /// @note The difference with the previous two statistics methods is that it aggregates stats on a per-task basis,
    ///       not on per-key basis.
    SequenceKeyStatistics getTaskStatistics();
    
    /// @brief Drains all sequenced tasks.
    /// @param[in] timeout Maximum time for this function to wait. Set to 0 to wait indefinitely until all sequences drain.
    /// @param[in] isFinal If set to true, the sequencer will not allow any more processing after the drain completes.
    /// @note This function blocks until all sequences have completed. During this time, posting
    ///       of new tasks is disabled unless they are posted from within an already executing coroutine.
    ///       Since this function posts a task which will wait on all others, getStatistics().getPostedTaskCount()
    ///       will contain one extra count.
    void drain(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero(),
               bool isFinal = false);

private:
    using ContextMap = std::unordered_map<SequenceKey, SequenceKeyData, Hash, KeyEqual, Allocator>;
    using ExceptionCallback = typename Configuration::ExceptionCallback;

    template <class FUNC, class ... ARGS>
    static int waitForTwoDependents(VoidContextPtr ctx,
                                    void* opaque,
                                    Sequencer& sequencer,
                                    SequenceKeyData&& dependent,
                                    SequenceKeyData&& universalDependent,
                                    FUNC&& func,
                                    ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int waitForDependents(VoidContextPtr ctx,
                                 void* opaque,
                                 Sequencer& sequencer,
                                 std::vector<SequenceKeyData>&& dependents,
                                 SequenceKeyData&& universalDependent,
                                 FUNC&& func,
                                 ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int waitForUniversalDependent(VoidContextPtr ctx,
                                         void* opaque,
                                         Sequencer& sequencer,
                                         std::vector<SequenceKeyData>&& dependents,
                                         SequenceKeyData&& universalDependent,
                                         FUNC&& func,
                                         ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int singleSequenceKeyTaskScheduler(
                                    VoidContextPtr ctx,
                                    void* opaque,
                                    int queueId,
                                    bool isHighPriority,
                                    Sequencer& sequencer,
                                    SequenceKey&& sequenceKey,
                                    FUNC&& func,
                                    ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int multiSequenceKeyTaskScheduler(
                                    VoidContextPtr ctx,
                                    void* opaque,
                                    int queueId,
                                    bool isHighPriority,
                                    Sequencer& sequencer,
                                    std::vector<SequenceKey>&& sequenceKeys,
                                    FUNC&& func,
                                    ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int universalTaskScheduler(
                                    VoidContextPtr ctx,
                                    void* opaque,
                                    int queueId,
                                    bool isHighPriority,
                                    Sequencer& sequencer,
                                    FUNC&& func,
                                    ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int callPosted(VoidContextPtr ctx,
                           void* opaque,
                           const Sequencer& sequencer,
                           FUNC&& func,
                           ARGS&&... args);

    static bool canTrimContext(const ICoroContextBasePtr& ctx,
                               const ICoroContextBasePtr& ctxToValidate);
    static bool isPendingContext(const ICoroContextBasePtr& ctx,
                                 const ICoroContextBasePtr& ctxToValidate);

    Dispatcher&              _dispatcher;
    std::atomic_bool         _drain;
    int                      _controllerQueueId;
    SequenceKeyData          _universalContext;
    ContextMap               _contexts;
    ExceptionCallback        _exceptionCallback;
    std::shared_ptr<SequenceKeyStatisticsWriter> _taskStats;
};

}}

#include <quantum/util/impl/quantum_sequencer_impl.h>

#endif //BLOOMBERG_QUANTUM_SEQUENCER_H
