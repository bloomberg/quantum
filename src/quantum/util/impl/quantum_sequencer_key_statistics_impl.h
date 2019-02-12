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
SequencerKeyStatistics::SequencerKeyStatistics(const SequencerKeyStatistics& that) :
    _postedTaskCount(that._postedTaskCount),
    _pendingTaskCount(that._pendingTaskCount.load())
{
}

inline
SequencerKeyStatistics::SequencerKeyStatistics(SequencerKeyStatistics&& that) :
    _postedTaskCount(std::move(that._postedTaskCount)),
    _pendingTaskCount(that._pendingTaskCount.load())
{
}

inline
SequencerKeyStatistics& SequencerKeyStatistics::operator = (SequencerKeyStatistics&& that)
{
    _postedTaskCount = std::move(that._postedTaskCount);
    _pendingTaskCount = that._pendingTaskCount.load();
    return *this;
}

inline
SequencerKeyStatistics& SequencerKeyStatistics::operator = (const SequencerKeyStatistics& that)
{
    _postedTaskCount = that._postedTaskCount;
    _pendingTaskCount = _pendingTaskCount.load();
    return *this;
}
 
inline
size_t
SequencerKeyStatistics::getPostedTaskCount() const
{
    return _postedTaskCount;
}

inline
size_t
SequencerKeyStatistics::getPendingTaskCount() const
{
    return _pendingTaskCount;
}
 
inline
void
SequencerKeyStatisticsWriter::incrementPostedTaskCount()
{
    ++_postedTaskCount;
}
 
inline
void
SequencerKeyStatisticsWriter::incrementPendingTaskCount()
{
    ++_pendingTaskCount;
}

inline
void
SequencerKeyStatisticsWriter::decrementPendingTaskCount()
{
    --_pendingTaskCount;
}
 
}}
