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
#ifndef BLOOMBERG_QUANTUM_CONFIGURATION_H
#define BLOOMBERG_QUANTUM_CONFIGURATION_H

#include <quantum/quantum_thread_traits.h>
#include <chrono>
#include <utility>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class Configuration
//==============================================================================================
/// @class class Configuration.
/// @brief Configuration parameters for the Quantum library.
class Configuration
{
public:
     enum class BackoffPolicy : int { Linear,        ///< Linear backoff
                                      Exponential }; ///< Exponential backoff (doubles every time)
                           
    /// @brief Get the JSON schema corresponding to this configuration object.
    /// @return The draft-04 compatible schema.
    static const std::string& getJsonSchema();
    
    /// @brief Get the schema URI used to resolve remote JSON references '$ref'.
    /// @return The URI.
    static const std::string& getJsonSchemaUri();
    
    /// @brief Set the number of threads running coroutines.
    /// @param[in] num The number of threads. Set to -1 to have one coroutine thread per core.
    ///            Default is -1.
    void setNumCoroutineThreads(int num);
    
    /// @brief Set the number of threads running IO tasks.
    /// @param[in] num The number of threads. Default is 5.
    void setNumIoThreads(int num);
    
    /// @brief Indicate if coroutine threads should be pinned to a core.
    /// @param[in] value True or False. Default is False.
    /// @note For best performance, the number of coroutine threads should
    ///       be <= the number of cores in the system.
    void setPinCoroutineThreadsToCores(bool value);
    
    /// @brief Load balancee the shared IO queues.
    /// @param[in] value If set to true, posting to the 'any' IO queue will result in
    ///              the load being spread among N queues. This mode can provide higher
    ///              throughput if dealing with high task loads. Default is false.
    /// @note To achieve higher performance, the threads run in polling mode which
    ///       increases CPU usage even when idle.
    void setLoadBalanceSharedIoQueues(bool value);
    
    /// @brief Set the interval between IO thread polls.
    /// @param[in] interval Interval in milliseconds. Default is 100ms.
    /// @note Setting this to a higher value means it may take longer to react to the first
    ///       IO task posted, and vice-versa if the interval is lower.
    void setLoadBalancePollIntervalMs(std::chrono::milliseconds interval);
    
    /// @brief Set a backoff policy for the shared queue polling interval.
    /// @param[in] policy The backoff policy to use. Default is 'Linear'.
    void setLoadBalancePollIntervalBackoffPolicy(BackoffPolicy policy);
    
    /// @brief Set the number of backoffs.
    /// @param[in] numBackoffs The number of backoff increments. Default is 0.
    ///                    When the number of backoffs is reached, the poll interval remains unchanged thereafter.
    void setLoadBalancePollIntervalNumBackoffs(size_t numBackoffs);

    /// @brief Sets the range of coroutine queueIds covered by IQueue::QueueId::Any when using Dispatcher::post
    /// @param[in] coroQueueIdRangeForAny The range [minQueueId, maxQueueId] of queueIds that IQueue::QueueId::Any
    /// will cover.
    /// @remark if the provided range is empty or invalid, then the default range of
    /// std::pair<int, int>(0, getNumCoroutineThreads()-1) will be used
    void setCoroQueueIdRangeForAny(const std::pair<int, int>& coroQueueIdRangeForAny);

    /// @brief Enables or disables the shared-coro-queue-for-any settings
    /// @param[in] isSharedCoroQueueForAny sets the shared-coro-queue-for any setting
    /// @warning When the coroutine sharing feature is enabled, then after each yield 
    /// (explicit or implicit) a coroutine sent to the Any queue may be executed by
    /// a different thread. As a result, coroutines using thread-local-storage (e.g., via thread_local),
    /// will _not_ work as expected.
    void setCoroutineSharingForAny(bool sharing);
    
    /// @brief Get the number of coroutine threads.
    /// @return The number of threads.
    int getNumCoroutineThreads() const;
    
    /// @brief Get the number of IO threads.
    /// @return The number of threads.
    int getNumIoThreads() const;
    
    /// @brief Check to see if coroutine threads are pinned to cores or not.
    /// @return True or False.
    bool getPinCoroutineThreadsToCores() const;
    
    /// @brief Check if IO shared queues are load balanced or not.
    /// @return True or False.
    bool getLoadBalanceSharedIoQueues() const;
    
    /// @brief Get load balance shared queue poll interval.
    /// @return The number of milliseconds.
    std::chrono::milliseconds getLoadBalancePollIntervalMs() const;
    
    /// @brief Get the backoff policy in load balance mode.
    /// @return The backoff policy used.
    BackoffPolicy getLoadBalancePollIntervalBackoffPolicy() const;
    
    /// @brief Get the number of backoffs used.
    /// @return The number of backoffs.
    size_t getLoadBalancePollIntervalNumBackoffs() const;

    /// @brief Gets the range [minQueueId, maxQueueId] of coroutine queueIds covered by IQueue::QueueId::Any
    /// by the Dispatcher.
    /// @return queueIdRange The range of queueIds that IQueue::QueueId::Any covers
    const std::pair<int, int>& getCoroQueueIdRangeForAny() const;
    
    /// @brief Gets the enablement flag of the shared-coro-queue-for-any
    /// @return the enablement flag for the feature
    bool getCoroutineSharingForAny() const;
    
private:
    int                         _numCoroutineThreads{-1};
    int                         _numIoThreads{5};
    bool                        _pinCoroutineThreadsToCores{false};
    bool                        _loadBalanceSharedIoQueues{false};
    std::chrono::milliseconds   _loadBalancePollIntervalMs{100};
    BackoffPolicy               _loadBalancePollIntervalBackoffPolicy{BackoffPolicy::Linear};
    size_t                      _loadBalancePollIntervalNumBackoffs{0};
    std::pair<int, int>         _coroQueueIdRangeForAny{-1, -1};
    bool                        _coroutineSharingForAny{false};
};

}}

#include <quantum/impl/quantum_configuration_impl.h>

#endif //BLOOMBERG_QUANTUM_CONFIGURATION_H
