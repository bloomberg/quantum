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
#ifndef BLOOMBERG_QUANTUM_QUEUE_STATISTICS_H
#define BLOOMBERG_QUANTUM_QUEUE_STATISTICS_H

#include <quantum/interface/quantum_iqueue_statistics.h>
#include <atomic>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                       class QueueStatistics
//==============================================================================================
/// @class QueueStatistics.
/// @brief Provides various counters related to queues and task execution.
/// @note See IQueueStatistics for detailed description.
class QueueStatistics : public IQueueStatistics
{
    friend class TaskQueue;
    friend class IOQueue;
    
public:
    QueueStatistics();
    QueueStatistics(const QueueStatistics&);
    
    //===================================
    //         IQUEUESTATISTICS
    //===================================
    void reset() final;
    
    size_t numElements() const final;
    
    void incNumElements() final;
    
    void decNumElements() final;
    
    size_t errorCount() const final;
    
    void incErrorCount() final;
    
    size_t sharedQueueErrorCount() const final;
    
    void incSharedQueueErrorCount() final;
    
    size_t completedCount() const final;
    
    void incCompletedCount() final;
    
    size_t sharedQueueCompletedCount() const final;
    
    void incSharedQueueCompletedCount() final;
    
    size_t postedCount() const final;
    
    void incPostedCount() final;
    
    size_t highPriorityCount() const final;
    
    void incHighPriorityCount() final;
    
    void print(std::ostream& out) const final;
    
    QueueStatistics& operator+=(const IQueueStatistics& rhs);
    
    friend QueueStatistics operator+(QueueStatistics lhs,
                                     const IQueueStatistics& rhs);

private:
    std::atomic_size_t  _numElements;
    size_t      _errorCount;
    size_t      _sharedQueueErrorCount;
    size_t      _completedCount;
    size_t      _sharedQueueCompletedCount;
    size_t      _postedCount;
    size_t      _highPriorityCount;
};

}}

#include <quantum/impl/quantum_queue_statistics_impl.h>

#endif //BLOOMBERG_QUANTUM_QUEUE_STATISTICS_H
