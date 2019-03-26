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
#ifndef BLOOMBERG_QUANTUM_SEQUENCE_KEY_STATISTICS_H
#define BLOOMBERG_QUANTUM_SEQUENCE_KEY_STATISTICS_H

#include <vector>
#include <tuple>
#include <atomic>
#include <memory>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                  class SequenceKeyStatistics
//==============================================================================================
/// @class SequenceKeyStatistics.
/// @brief Implementation of a statistics collection for a SequenceKey in Sequencer
class SequenceKeyStatistics
{
public:
    /// @brief Constructor.
    SequenceKeyStatistics() = default;

    /// @brief Constructor.
    SequenceKeyStatistics(const SequenceKeyStatistics& that);

    /// @brief Constructor.
    SequenceKeyStatistics(SequenceKeyStatistics&& that);

    /// @brief Assignment operator.
    SequenceKeyStatistics& operator=(const SequenceKeyStatistics& that);

    /// @brief Assignment operator.
    SequenceKeyStatistics& operator=(SequenceKeyStatistics&& that);

    /// @brief Gets the total number of tasks associated with the key that have been posted to the Sequencer
    ///        since the sequencer started tracking the key
    /// @return the number of tasks
    size_t getPostedTaskCount() const;

    /// @brief Gets the total number of pending tasks associated with the key
    /// @remark A task is pending if the dispatcher has not started it yet
    /// @return the number of tasks
    size_t getPendingTaskCount() const;

protected:
    /// @brief Number of posted tasks associated with the sequence key
    size_t _postedTaskCount{0};
    /// @brief Number of pending tasks associated with the sequence key
    std::atomic<size_t> _pendingTaskCount{0};
};

//==============================================================================================
//                                      class SequenceKeyStatisticsWriter
//==============================================================================================
/// @class SequenceKeyStatistics.
/// @brief Implementation of a writer for the SequenceKeyStatistics
class SequenceKeyStatisticsWriter: public SequenceKeyStatistics
{
public:
    /// @brief Increments the total number of tasks associated with the key that have been posted to the Sequencer
    ///        since the sequencer started tracking the key
    void incrementPostedTaskCount();

    /// @brief Increments the total number of pending tasks associated with the key
    void incrementPendingTaskCount();

    /// @brief Increments the total number of pending tasks associated with the key
    void decrementPendingTaskCount();
};

}}

#include <quantum/util/impl/quantum_sequence_key_statistics_impl.h>

#endif //BLOOMBERG_QUANTUM_SEQUENCE_KEY_STATISTICS_H
