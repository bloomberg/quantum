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
#ifndef BLOOMBERG_QUANTUM_SEQUENCER_TEST_COMMON_H
#define BLOOMBERG_QUANTUM_SEQUENCER_TEST_COMMON_H

#include <quantum_perf_utils.h>
#include <chrono>
#include <iostream>
#include <atomic>

namespace Bloomberg {
namespace quantum {

template<typename Sequencer>
void testSequencerPerformance(
    const std::string& name,
    quantum::Dispatcher& dispatcher,
    unsigned int sleepTimeMicroseconds,
    unsigned int distinctKeyCount,
    unsigned int keyCountPerTask,
    unsigned int taskCount,
    unsigned int universalTaskFreq,
    unsigned int yieldCount)
{
    Sequencer sequencer(dispatcher);

    if (keyCountPerTask == 0 || distinctKeyCount == 0 || taskCount == 0)
    {
        return;
    }

    ProcStats startStats = getProcStats();

    // enqueue the tasks
    {
        Timer timer;
        for(unsigned int id = 0; id < taskCount; ++id)
        {
            // save the task id for this sequenceKey
            auto task = [id, sleepTimeMicroseconds, yieldCount](VoidContextPtr ctx)->int
            {
                auto sleepTime = sleepTimeMicroseconds / (yieldCount + 1);
                for(unsigned int i = 0; i < yieldCount; ++i)
                {
                    ctx->sleep(std::chrono::microseconds(sleepTime));
                    ctx->yield();
                }
                ctx->sleep(std::chrono::microseconds(sleepTime));
                return 0;
            };

            if (id and universalTaskFreq and id % universalTaskFreq == 0)
            {
                sequencer.enqueueAll(std::move(task));
            }
            else
            {
                if (keyCountPerTask == 1)
                {
                    sequencer.enqueue(id % distinctKeyCount, std::move(task));
                }
                else
                {
                    std::vector<int> keys;
                    for(unsigned int i = 0; i < keyCountPerTask; ++i)
                    {
                        keys.push_back((id + i) % distinctKeyCount);
                    }

                    sequencer.enqueue(keys, std::move(task));
                }
            }
        }
        sequencer.drain();
    }
    ProcStats procStats = getProcStats() - startStats;

    std::cout << name << ": elapsed "
              << Timer::elapsed<std::chrono::seconds>() << " s, "
              << procStats._kernelModeTime + procStats._userModeTime << " CPU ticks"
              << std::endl;
}

}}

#endif // BLOOMBERG_QUANTUM_SEQUENCER_TEST_COMMON_H
