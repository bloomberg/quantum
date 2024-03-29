/*
** Copyright 2021 Bloomberg Finance L.P.
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
namespace experimental {

template <class SequenceKey>
template<typename FuncType>
SequencerTask<SequenceKey>::SequencerTask(
    FuncType&& func,
    bool universal,
    void* opaque,
    int queueId,
    bool isHighPriority) :
_func(std::move(func)),
_pendingKeyCount(0),
_universal(universal),
_opaque(opaque),
_queueId(queueId),
_isHighPriority(isHighPriority)
{
}

template <class SequenceKey>
SequencerKeyData<SequenceKey>::SequencerKeyData() :
_stats(std::make_shared<SequenceKeyStatisticsWriter>())
{
}

}}}

