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
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################

namespace Bloomberg {
namespace quantum {

inline
QueueStatistics::QueueStatistics()
{
    reset();
}

inline
QueueStatistics::QueueStatistics(const QueueStatistics& other) :
    _numElements(other.numElements()),
    _errorCount(other._errorCount),
    _sharedQueueErrorCount(other._sharedQueueErrorCount),
    _completedCount(other._completedCount),
    _sharedQueueCompletedCount(other._sharedQueueCompletedCount),
    _postedCount(other._postedCount),
    _highPriorityCount(other._highPriorityCount)
{
}

inline
void QueueStatistics::reset()
{
    _numElements = 0;
    _errorCount = 0;
    _sharedQueueErrorCount = 0;
    _completedCount = 0;
    _sharedQueueCompletedCount = 0;
    _postedCount = 0;
    _highPriorityCount = 0;
}

inline
size_t QueueStatistics::numElements() const
{
    return _numElements;
}

inline
void QueueStatistics::incNumElements()
{
    ++_numElements;
}

inline
void QueueStatistics::decNumElements()
{
    size_t oldValue = 1;
    size_t newValue = 0;
    while(!_numElements.compare_exchange_weak(oldValue, newValue, std::memory_order_acq_rel))
    {
        if (oldValue == 0)
        {
            break;
        }
        else
        {
            newValue = oldValue - 1;
        }
    }
}

inline
size_t QueueStatistics::errorCount() const
{
    return _errorCount;
}

inline
void QueueStatistics::incErrorCount()
{
    ++_errorCount;
}

inline
size_t QueueStatistics::sharedQueueErrorCount() const
{
    return _sharedQueueErrorCount;
}

inline
void QueueStatistics::incSharedQueueErrorCount()
{
    ++_sharedQueueErrorCount;
}

inline
size_t QueueStatistics::completedCount() const
{
    return _completedCount;
}

inline
void QueueStatistics::incCompletedCount()
{
    ++_completedCount;
}

inline
size_t QueueStatistics::sharedQueueCompletedCount() const
{
    return _sharedQueueCompletedCount;
}

inline
void QueueStatistics::incSharedQueueCompletedCount()
{
    ++_sharedQueueCompletedCount;
}

inline
size_t QueueStatistics::postedCount() const
{
    return _postedCount;
}

inline
void QueueStatistics::incPostedCount()
{
    ++_postedCount;
}

inline
size_t QueueStatistics::highPriorityCount() const
{
    return _highPriorityCount;
}

inline
void QueueStatistics::incHighPriorityCount()
{
    ++_highPriorityCount;
}

inline
void QueueStatistics::print(std::ostream& out) const
{
    out << "Num elemetns: " << _numElements << std::endl;
    out << "Num queued: " << _errorCount << std::endl;
    out << "Num completed: " << _completedCount << std::endl;
    out << "Num shared completed: " << _sharedQueueCompletedCount << std::endl;
    out << "Num errors: " << _errorCount << std::endl;
    out << "Num shared errors: " << _sharedQueueErrorCount << std::endl;
    out << "Num high priority count: " << _highPriorityCount << std::endl;
}

inline
QueueStatistics& QueueStatistics::operator+=(const IQueueStatistics& rhs)
{
    _numElements += rhs.numElements();
    _errorCount += rhs.errorCount();
    _sharedQueueErrorCount += rhs.sharedQueueErrorCount();
    _completedCount += rhs.completedCount();
    _sharedQueueCompletedCount += rhs.sharedQueueCompletedCount();
    _postedCount += rhs.postedCount();
    _highPriorityCount += rhs.highPriorityCount();
    return *this;
}

inline
QueueStatistics operator+(QueueStatistics lhs,
                          const IQueueStatistics& rhs)
{
    lhs += rhs;
    return lhs;
}

inline
std::ostream& operator<<(std::ostream& out, const IQueueStatistics& stats)
{
    stats.print(out);
    return out;
}

}}
