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
SequenceKeyStatistics::SequenceKeyStatistics(const SequenceKeyStatistics& that) :
    _postedTaskCount(that._postedTaskCount),
    _pendingTaskCount(that._pendingTaskCount.load())
{
}

inline
SequenceKeyStatistics::SequenceKeyStatistics(SequenceKeyStatistics&& that) :
    _postedTaskCount(std::move(that._postedTaskCount)),
    _pendingTaskCount(that._pendingTaskCount.load())
{
}

inline
SequenceKeyStatistics& SequenceKeyStatistics::operator = (SequenceKeyStatistics&& that)
{
    _postedTaskCount = std::move(that._postedTaskCount);
    _pendingTaskCount = that._pendingTaskCount.load();
    return *this;
}

inline
SequenceKeyStatistics& SequenceKeyStatistics::operator = (const SequenceKeyStatistics& that)
{
    _postedTaskCount = that._postedTaskCount;
    _pendingTaskCount = _pendingTaskCount.load();
    return *this;
}
 
inline
size_t
SequenceKeyStatistics::getPostedTaskCount() const
{
    return _postedTaskCount;
}

inline
size_t
SequenceKeyStatistics::getPendingTaskCount() const
{
    return _pendingTaskCount;
}
 
inline
void
SequenceKeyStatisticsWriter::incrementPostedTaskCount()
{
    ++_postedTaskCount;
}
 
inline
void
SequenceKeyStatisticsWriter::incrementPendingTaskCount()
{
    ++_pendingTaskCount;
}

inline
void
SequenceKeyStatisticsWriter::decrementPendingTaskCount()
{
    --_pendingTaskCount;
}
 
}}
