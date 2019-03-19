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
#ifndef BLOOMBERG_QUANTUM_IQUEUE_STATISTICS_H
#define BLOOMBERG_QUANTUM_IQUEUE_STATISTICS_H

#include <ostream>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  interface IQueueStatistics
//==============================================================================================
/// @interface IQueueStatistics.
/// @brief Interface to access and manipulate a QueueStatistics object.
struct IQueueStatistics
{
    /// @brief Virtual destructor.
    virtual ~IQueueStatistics(){};
    
    /// @brief Reset all the counters to 0.
    virtual void reset() = 0;
    
    /// @brief Gets the current size of the queue
    virtual size_t numElements() const = 0;
    
    /// @brief Increment this counter.
    virtual void incNumElements() = 0;
    
    /// @brief Decrement this counter.
    virtual void decNumElements() = 0;
    
    /// @brief Count of all coroutine and IO task execution errors on this queue.
    /// @return Counter value.
    virtual size_t errorCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incErrorCount() = 0;
    
    /// @brief Count of all IO tasks which were dequeued from the shared queue and failed.
    /// @return Counter value.
    virtual size_t sharedQueueErrorCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incSharedQueueErrorCount() = 0;
    
    /// @brief Count of all coroutine and IO tasks which completed successfully.
    /// @return Counter value.
    virtual size_t completedCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incCompletedCount() = 0;
    
    /// @brief Count of all IO tasks which were dequeued from the shared queue and completed successfully.
    /// @return Counter value.
    virtual size_t sharedQueueCompletedCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incSharedQueueCompletedCount() = 0;
    
    /// @brief Count of all coroutine and IO tasks which were posted on this queue.
    /// @return Counter value.
    virtual size_t postedCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incPostedCount() = 0;
    
    /// @brief Count of all coroutine and IO tasks which were posted on this queue at higher priority.
    /// @return Counter value.
    virtual size_t highPriorityCount() const = 0;
    
    /// @brief Increment this counter.
    virtual void incHighPriorityCount() = 0;
    
    /// @brief Print to std::cout the value of all internal counters.
    /// @param[in,out] out Output stream.
    virtual void print(std::ostream& out) const = 0;
};

/// @brief Overloads stream operator for IQueueStatistics object
/// @param[in] out Output stream.
/// @param[in] stats Statistics object to stream.
/// @return Reference to the same input stream.
std::ostream& operator<<(std::ostream& out, const IQueueStatistics& stats);

}}

#endif //BLOOMBERG_QUANTUM_IQUEUE_STATISTICS_H
