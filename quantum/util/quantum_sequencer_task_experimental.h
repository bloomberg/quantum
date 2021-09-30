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

#ifndef BLOOMBERG_QUANTUM_SEQUENCER_TASK_EXPERIMENTAL_H
#define BLOOMBERG_QUANTUM_SEQUENCER_TASK_EXPERIMENTAL_H

#include <atomic>
#include <memory>
#include <queue>
#include <vector>
#include <list>

namespace Bloomberg {
namespace quantum {
namespace experimental {

template <class SequenceKey>
struct SequencerKeyData;

template <class SequenceKey>
struct SequencerTask
{
    template<typename FuncType>
    SequencerTask(
        FuncType&& func,
        bool universal,
        void* opaque,
        int queueId,
        bool isHighPriority);

    Function<int(VoidContextPtr)> _func; // the function to run
    std::vector<SequencerKeyData<SequenceKey>*> _keyData; // pointers to the key data of my keys
    unsigned int _pendingKeyCount; // number of key queues where I am not at the head
    bool _universal; // true of universal tasks
    void* _opaque; // opaque pointer passed by user
    int _queueId; // the queue to enqueue the task
    bool _isHighPriority; // high priority task
};

template <class SequenceKey>
struct SequencerKeyData
{
    SequencerKeyData();

    std::list<std::shared_ptr<SequencerTask<SequenceKey>>> _tasks; // task queue
    std::shared_ptr<SequenceKeyStatisticsWriter> _stats; // stats for all tasks sharing this key
};

}}}

#include <quantum/util/impl/quantum_sequencer_task_experimental_impl.h>

#endif // BLOOMBERG_QUANTUM_SEQUENCER_TASK_EXPERIMENTAL_H

