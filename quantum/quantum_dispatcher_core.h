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
#ifndef BLOOMBERG_QUANTUM_DISPATCHER_CORE_H
#define BLOOMBERG_QUANTUM_DISPATCHER_CORE_H

#include <vector>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <algorithm>
#ifdef _WIN32
#include <winbase.h>
#else
#include <pthread.h>
#endif
#include <quantum/quantum_configuration.h>
#include <quantum/quantum_task_queue.h>
#include <quantum/quantum_io_queue.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class DispatcherCore
//==============================================================================================
/// @class DispatcherCore
/// @brief Class implementing the dispatching logic unto worker threads. Used for both coroutines
///        and IO tasks.
/// @note For internal use only.
class DispatcherCore : public ITerminate
{
public:
    friend class Dispatcher;
    
    ~DispatcherCore();
    
    void terminate() final;
    
    size_t size(IQueue::QueueType type, int queueId) const;
    
    bool empty(IQueue::QueueType type, int queueId) const;
    
    QueueStatistics stats(IQueue::QueueType type, int queueId);
    
    void resetStats();
    
    void post(Task::Ptr task);
    
    void postAsyncIo(IoTask::Ptr task);
    
    int getNumCoroutineThreads() const;
    
    int getNumIoThreads() const;

    const std::pair<int, int>& getCoroQueueIdRangeForAny() const;
    
private:
    DispatcherCore(const Configuration& config);
    
    size_t coroSize(int queueId) const;
    
    size_t ioSize(int queueId) const;
    
    bool coroEmpty(int queueId) const;
    
    bool ioEmpty(int queueId) const;
    
    QueueStatistics coroStats(int queueId);
    
    QueueStatistics ioStats(int queueId);
    
    //Members
    std::shared_ptr<TaskQueue>  _sharedCoroAnyQueue; // shared coro queue for Any
    std::vector<TaskQueue>      _coroQueues;     //coroutine queues
    std::vector<IoQueue>        _sharedIoQueues; //shared IO task queues (hold tasks posted to 'Any' IO queue)
    std::vector<IoQueue>        _ioQueues;       //dedicated IO task queues
    bool                        _loadBalanceSharedIoQueues; //tasks posted to 'Any' IO queue are load balanced
    std::atomic_bool            _terminated;
    std::pair<int, int>         _coroQueueIdRangeForAny; // range of coroutine queueIds covered by 'Any' 
};

}}

#include <quantum/impl/quantum_dispatcher_core_impl.h>

#endif //BLOOMBERG_QUANTUM_DISPATCHER_CORE_H
