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
#ifndef QUANTUM_SEQUENCER_H
#define QUANTUM_SEQUENCER_H

#include <quantum/quantum_dispatcher.h>
#include <quantum/interface/quantum_ithread_context_base.h>
#include <quantum/util/quantum_sequencer_configuration.h>
#include <quantum/util/quantum_sequencer_key_statistics.h>
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
/// @tparam Hash Hash-function used for storing instances of SequncerKey in hash maps
/// @tparam KeyEqual The equal-function used for storing instances of SequncerKey in hash maps
/// @tparam Allocator The allocator used for storing instances of SequncerKey in hash maps
    
template <class SequenceKey,
          class Hash = std::hash<SequenceKey>,
          class KeyEqual = std::equal_to<SequenceKey>,
          class Allocator = std::allocator<std::pair<const SequenceKey, SequencerKeyData>>>
class Sequencer
{
public:
    /// @brief Configuration class for Sequencer
    using Configuration = SequencerConfiguration<SequenceKey, Hash, KeyEqual, Allocator>;

    /// @brief Constructor.
    /// @param[in] dispatcher Dispatcher for all task dispatching
    /// @param[in] configuration the configuration object
    Sequencer(Dispatcher& dispatcher, const Configuration& configuration = Configuration());

    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine
    ///              (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] sequenceKey SequenceKey object that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    post(const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args);

    /// @brief Post a coroutine to run asynchronously on a specific queue (thread).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine
    ////             (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user can
    ///                    specify IQueue::QueueId::Any as a value, which is equivalent to running
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception hander (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] sequenceKey SequenceKey object that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    postEx(int queueId, bool isHighPriority, void* opaque, const SequenceKey& sequenceKey, FUNC&& func, ARGS&&... args);

    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available
    ///          (@see Dispatcher::post for more details).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine
    ///              (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] sequenceKeys A collection of sequenceKey objects that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    post(const std::vector<SequenceKey>& sequenceKeys, FUNC&& func, ARGS&&... args);

    /// @brief Post a coroutine to run asynchronously on a specific queue (thread).
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine
    ///              (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user 
    ///                    can specify IQueue::QueueId::Any as a value, which is equivalent to running 
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or 
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception hander (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] sequenceKeys A collection of sequenceKey objects that the posted task is associated with
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    postEx(int queueId, 
           bool isHighPriority,
           void* opaque,
           const std::vector<SequenceKey>& sequenceKeys, 
           FUNC&& func, 
           ARGS&&... args);

    /// @brief Post a coroutine to run asynchronously.
    /// @details This method will post the coroutine on any thread available 
    ///          (@see Dispatcher::post for more details). The posted task is assumed to be associated 
    ///           with the entire universe of sequenceKeys of the SequenceKey type.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine 
    ///              (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @remark This function also trims the sequence keys not used by the sequencer anymore
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    postAll(FUNC&& func, ARGS&&... args);

    /// @brief Post a coroutine to run asynchronously on a specific queue (thread).
    /// @details This method will post the coroutine on any thread available 
    ///          (@see Dispatcher::post for more details). The posted task is assumed to be associated with 
    ///           the entire universe of sequenceKeys of the SequenceKey type.
    /// @tparam FUNC Callable object type which will be wrapped in a coroutine 
    ///              (@see Dispatcher::post for more details).
    /// @tparam ARGS Argument types passed to FUNC (@see Dispatcher::post for more details).
    /// @param[in] queueId Id of the queue where this coroutine should run. Note that the user 
    ///                    can specify IQueue::QueueId::Any as a value, which is equivalent to running 
    ///                    the simpler version of post() above. Valid range is [0, numCoroutineThreads) or 
    ///                    IQueue::QueueId::Any.
    /// @param[in] isHighPriority If set to true, the sequencer coroutine will be scheduled right 
    ///                           after the currently executing coroutine on 'queueId'.
    /// @param[in] opaque pointer to opaque data that is passed to the exception hander (if provided)
    ///            if an unhandled exception is thrown in func
    /// @param[in] func Callable object.
    /// @param[in] args Variable list of arguments passed to the callable object.
    /// @remark This function also trims the sequence keys not used by the sequencer anymore
    /// @note This function is non-blocking and returns immediately.
    template <class FUNC, class ... ARGS>
    void
    postAllEx(int queueId, bool isHighPriority, void* opaque, FUNC&& func, ARGS&&... args);

    /// @brief Trims the sequence keys not used by the sequencer anymore.
    /// @details It's recommented to call this function periodically to clean up state sequence keys. 
    /// @remark This call clears all the statistics for trimmed keys. 
    /// @return The number of sequenceKeys after the trimming.
    /// @note This function blocks until the trimming job posted to the dispatcher is finished
    size_t trimSequenceKeys();

    /// @brief Gets the number of tracked sequence keys
    /// @return sequence key count
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    size_t getSequenceKeyCount();

    /// @brief Gets the sequencer statistics for a sequence key
    /// @param sequenceKey the key 
    /// @return the statistics objects for the specified key
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    SequencerKeyStatistics getStatistics(const SequenceKey& sequenceKey);

    /// @brief Gets the sequencer statistics for jobs posted via postAll calls
    /// @return the statistics objects
    /// @note This function blocks until the statistics computation job posted to the dispatcher is finished.
    SequencerKeyStatistics getStatistics();

private:
    using ContextMap = std::unordered_map<SequenceKey, SequencerKeyData, Hash, KeyEqual, Allocator>;
    using ExceptionCallback = typename Configuration::ExceptionCallback;

    template <class FUNC, class ... ARGS>
    static int waitForTwoDependents(CoroContextPtr<int> ctx,
                                    ICoroContextBasePtr dependent,
                                    ICoroContextBasePtr universalContext,
                                    SequencerKeyStatisticsWriter& stats,
                                    void* opaque, 
                                    const ExceptionCallback& exceptionCallback,
                                    FUNC&& func,
                                    ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int waitForDependents(CoroContextPtr<int> ctx,
                                 std::vector<std::pair<ICoroContextBasePtr, SequencerKeyStatisticsWriter*>>&& dependents,
                                 void* opaque,
                                 const ExceptionCallback& exceptionCallback,
                                 FUNC&& func,
                                 ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int singleSequenceKeyTaskScheduler(CoroContextPtr<int> ctx,
                                              int queueId,
                                              bool isHighPriority, 
                                              void* opaque, 
                                              const ExceptionCallback& exceptionCallback,
                                              SequenceKey&& sequenceKey,
                                              ContextMap& contexts,
                                              SequencerKeyData& universalContext,
                                              FUNC&& func,
                                              ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int multiSequenceKeyTaskScheduler(CoroContextPtr<int> ctx,
                                             int queueId,
                                             bool isHighPriority, 
                                             void* opaque, 
                                             const ExceptionCallback& exceptionCallback,
                                             std::vector<SequenceKey>&& sequenceKeys,
                                             ContextMap& contexts,
                                             SequencerKeyData& universalContext,
                                             FUNC&& func,
                                             ARGS&&... args);
    template <class FUNC, class ... ARGS>
    static int universalTaskScheduler(CoroContextPtr<int> ctx,
                                      int queueId,
                                      bool isHighPriority,
                                      void* opaque, 
                                      const ExceptionCallback& exceptionCallback,
                                      ContextMap& contexts,
                                      SequencerKeyData& universalContext,
                                      FUNC&& func,
                                      ARGS&&... args);
    template <class FINAL_ACTION, class FUNC, class ... ARGS>
    static void callPosted(CoroContextPtr<int> ctx, 
                           const FINAL_ACTION& finalAction,
                           void* opaque, 
                           const ExceptionCallback& exceptionCallback,
                           FUNC&& func, 
                           ARGS&&... args);

    static bool canTrimContext(const ICoroContextBasePtr& ctx,
                               const ICoroContextBasePtr& ctxToValidate);

    Dispatcher&              _dispatcher;
    int                      _controllerQueueId;
    SequencerKeyData         _universalContext;
    ContextMap               _contexts;
    ExceptionCallback        _exceptionCallback;
};

}}

#include <quantum/util/impl/quantum_sequencer_impl.h>

#endif //QUANTUM_SEQUENCER_H
