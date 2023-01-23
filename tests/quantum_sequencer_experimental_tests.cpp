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

#include <quantum_fixture.h>
#include <quantum_sequencer_test_common.h>
#include <gtest/gtest.h>
#include <sstream>

#ifdef BLOOMBERG_QUANTUM_SEQUENCER_SUPPORT

using namespace quantum;

//==============================================================================
// TEST FIXTURES
//==============================================================================

struct SequencerExperimentalTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(SequencerExperimentalTest_Default,
                        SequencerExperimentalTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));

// Utility with common functionality for the sequencer related tests
class SequencerExperimentalTestData
{
public: // types
    using SequenceKey = int;
    using TaskId = int;
    using TaskSequencer = experimental::Sequencer<SequenceKey>;
    using TaskSequencerConfiguration = experimental::SequencerConfiguration<SequenceKey>;
    struct TaskResult
    {
        /// Task start time
        std::chrono::steady_clock::time_point startTime;
        /// Task end time
        std::chrono::steady_clock::time_point endTime;
    };
    using TaskResultMap = std::unordered_map<TaskId, TaskResult>;
    using SequenceKeyMap = std::unordered_map<SequenceKey, std::vector<TaskId>>;

    /**
     * @Brief checks if a task was started before another task finished
     */
    void ensureOrder(TaskId beforeTaskId, TaskId afterTaskId)
    {
        TaskResultMap::const_iterator beforeTaskIt = _results.find(beforeTaskId);
        ASSERT_NE(beforeTaskIt, _results.end());
        TaskResultMap::const_iterator afterTaskIt = _results.find(afterTaskId);
        ASSERT_NE(afterTaskIt, _results.end());
        EXPECT_LE(beforeTaskIt->second.endTime, afterTaskIt->second.startTime);
    }

    std::function<int(VoidContextPtr)> makeTask(TaskId taskId)
    {
        return [this, taskId](VoidContextPtr ctx)->int
        {
            taskFunc(ctx, taskId, nullptr, "", 0, nullptr);
            return 0;
        };
    }

    std::function<int(VoidContextPtr)> makeTaskWithYields(TaskId taskId, unsigned int yieldCount, std::atomic<unsigned int>* totalYieldCount)
    {
        return [this, taskId, yieldCount, totalYieldCount](VoidContextPtr ctx)->int
        {
            taskFunc(ctx, taskId, nullptr, "", yieldCount, totalYieldCount);
            return 0;
        };
    }

    std::function<int(VoidContextPtr)> makeTaskWithBlock(TaskId taskId, std::atomic<bool>* blockFlag)
    {
        return [this, taskId, blockFlag](VoidContextPtr ctx)->int
        {
            taskFunc(ctx, taskId, blockFlag, "", 0, nullptr);
            return 0;
        };
    }

    std::function<int(VoidContextPtr)> makeTaskWithException(TaskId taskId, std::string error)
    {
        return [this, taskId, error](VoidContextPtr ctx)->int
        {
            taskFunc(ctx, taskId, nullptr, error, 0, nullptr);
            return 0;
        };
    }

    TaskResultMap& results()
    {
        return _results;
    }

    void sleep(int periodCount = 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(periodCount*1));
    }

    void taskFunc(
        VoidContextPtr ctx,
        TaskId id,
        std::atomic<bool>* blockFlag,
        std::string error,
        unsigned int yieldCount,
        std::atomic<unsigned int>* totalYieldCount)
    {
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        if (yieldCount)
        {
            do {
                ctx->sleep(std::chrono::milliseconds(1));
                ctx->yield();
                --yieldCount;
                if (totalYieldCount)
                    ++(*totalYieldCount);
            } while(yieldCount);
        }
        else
        {
            do {
                ctx->sleep(std::chrono::milliseconds(1));
                if (not error.empty()) {
                    throw std::runtime_error(error);
                }
            }
            while(blockFlag and *blockFlag);
        }
        std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

        // update the task map with the time stats
        quantum::Mutex::Guard lock(ctx, _resultMutex);

        _results[id].startTime = startTime;
        _results[id].endTime = endTime;
    }

private: // members

    /// task results
    TaskResultMap _results;
    // mutex for _results accessing
    quantum::Mutex _resultMutex;
};

TEST_P(SequencerExperimentalTest, BasicTaskOrder)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 2000;
    const int sequenceKeyCount = 3;
    SequencerExperimentalTestData testData;
    SequencerExperimentalTestData::SequenceKeyMap sequenceKeys;

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());

    // enqueue the tasks
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
        // save the task id for this sequenceKey
        sequenceKeys[sequenceKey].push_back(id);
        sequencer.enqueue(sequenceKey, testData.makeTask(id));
    }
    sequencer.drain();

    EXPECT_EQ(testData.results().size(), (size_t)taskCount);

    // the tasks must be ordered within the same sequenceKey
    for(auto sequenceKeyData : sequenceKeys)
    {
        for(size_t i = 1; i < sequenceKeyData.second.size(); ++i)
        {
            testData.ensureOrder(sequenceKeyData.second[i-1], sequenceKeyData.second[i]);
        }
    }
}

TEST_P(SequencerExperimentalTest, BasicTaskOrderWithYields)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 2000;
    const int sequenceKeyCount = 3;
    const int yieldCount = 2;
    SequencerExperimentalTestData testData;
    SequencerExperimentalTestData::SequenceKeyMap sequenceKeys;
    std::atomic<unsigned int> totalYieldCount(0);

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());

    // enqueue the tasks
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
        // save the task id for this sequenceKey
        sequenceKeys[sequenceKey].push_back(id);
        sequencer.enqueue(sequenceKey, testData.makeTaskWithYields(id, yieldCount, &totalYieldCount));
    }
    sequencer.drain();

    EXPECT_EQ(testData.results().size(), (size_t)taskCount);

    // the tasks must be ordered within the same sequenceKey
    for(auto sequenceKeyData : sequenceKeys)
    {
        for(size_t i = 1; i < sequenceKeyData.second.size(); ++i)
        {
            testData.ensureOrder(sequenceKeyData.second[i-1], sequenceKeyData.second[i]);
        }
    }
    EXPECT_EQ(yieldCount * taskCount, totalYieldCount);
}

TEST_P(SequencerExperimentalTest, BasicTaskOrderWithParams)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 2000;
    const int sequenceKeyCount = 3;
    SequencerExperimentalTestData testData;
    std::ostringstream buf;
    SequencerExperimentalTestData::SequenceKeyMap sequenceKeys;

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());
    std::atomic<int> mismatchCount(0);

    // enqueue the tasks
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
        buf.str("");
        buf << "Task " << id;
        std::string text1 = buf.str();
        std::string text2 = text1;
        std::string text3 = text1;

        sequenceKeys[sequenceKey].push_back(id);
        auto task = [&testData, id, &mismatchCount, text3](
            VoidContextPtr ctx, std::string&& text1, std::string text2)->int
        {
            std::ostringstream buf2;
            buf2 << "Task " << id;
            std::string strId = buf2.str();
            if (strId != text1) {
                ++mismatchCount;
            }
            if (strId != text2){
                ++mismatchCount;
            }
            if (strId != text3){
                ++mismatchCount;
            }

            testData.taskFunc(ctx, id, nullptr, "", 0, nullptr);
            return 0;
        };

        sequencer.enqueue(sequenceKey, std::move(task), std::move(text1), std::string(text2));
    }
    sequencer.drain();

    EXPECT_EQ(0u, mismatchCount);
    EXPECT_EQ(testData.results().size(), (size_t)taskCount);

    // the tasks must be ordered within the same sequenceKey
    for(auto sequenceKeyData : sequenceKeys)
    {
        for(size_t i = 1; i < sequenceKeyData.second.size(); ++i)
        {
            testData.ensureOrder(sequenceKeyData.second[i-1], sequenceKeyData.second[i]);
        }
    }
}

TEST_P(SequencerExperimentalTest, TrimKeys)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 100;
    const int sequenceKeyCount = 3;
    SequencerExperimentalTestData testData;

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());

    // enqueue the tasks
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
        sequencer.enqueue(sequenceKey, testData.makeTask(id));
    }
    sequencer.drain();

    EXPECT_EQ(sequencer.getSequenceKeyCount(), (size_t)sequenceKeyCount);
    EXPECT_EQ(sequencer.trimSequenceKeys(), 0u);
    EXPECT_EQ(sequencer.getSequenceKeyCount(), 0u);
}

TEST_P(SequencerExperimentalTest, ExceptionHandler)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 100;
    const int sequenceKeyCount = 3;
    const int exceptionFrequency = 14;
    SequencerExperimentalTestData testData;
    std::vector<SequencerExperimentalTestData::TaskId> sequenceKeys(taskCount);

    // generate the sequence of tasks for passing as opaque
    int generateTaskId = 0;
    std::generate(sequenceKeys.begin(),
                  sequenceKeys.end(),
                  [&generateTaskId]() { return generateTaskId++; });

    const std::string errorText = "Error";
    // the callback will check that exceptions are generated as expected
    std::atomic<unsigned int> exceptionCallbackCallCount(0);
    auto exceptionCallback = [&exceptionCallbackCallCount, &errorText](std::exception_ptr exception, void* opaque)
    {
        ++exceptionCallbackCallCount;
        ASSERT_NE(exception, nullptr);
        try
        {
            if (exception)
            {
                std::rethrow_exception(exception);
            }
            // the exception above must be thrown
            ASSERT_TRUE(false);
        }
        catch (const std::exception& e)
        {
            EXPECT_EQ(e.what(), errorText);
            ASSERT_NE(opaque, nullptr);
            SequencerExperimentalTestData::TaskId taskId = *static_cast<SequencerExperimentalTestData::TaskId*>(opaque);
            EXPECT_EQ(taskId % exceptionFrequency, 0);
        }
        catch (...)
        {
            ASSERT_TRUE(false);
        }
    };

    SequencerExperimentalTestData::TaskSequencerConfiguration config;
    config.setExceptionCallback(exceptionCallback);
    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher(), config);

    unsigned int generatedExceptionCount = 0;
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
        if (id % exceptionFrequency == 0)
        {
            // enqueue with generating exception
            sequencer.enqueue(&sequenceKeys[id], (int)IQueue::QueueId::Any, false, sequenceKey, testData.makeTaskWithException(id, errorText));
            ++generatedExceptionCount;
        }
        else
        {
            // enqueue with no exception generation
            sequencer.enqueue(&sequenceKeys[id], (int)IQueue::QueueId::Any, false, sequenceKey, testData.makeTask(id));
        }
    }
    sequencer.drain();

    EXPECT_EQ(generatedExceptionCount, exceptionCallbackCallCount);
}

TEST_P(SequencerExperimentalTest, SequenceKeyStats)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 50;
    const int sequenceKeyCount = 3;
    const int universalTaskFrequency = 11; // every 11th task is universal
    SequencerExperimentalTestData testData;
    std::atomic<bool> blockFlag(true);

    SequencerExperimentalTestData::TaskSequencerConfiguration config;
    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher(), config);

    // enqueue the first half
    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount / 2; ++id)
    {
        if ( id % universalTaskFrequency == 0 )
        {
            sequencer.enqueueAll(testData.makeTaskWithBlock(id, &blockFlag));
        }
        else
        {
            SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
            sequencer.enqueue(sequenceKey, testData.makeTaskWithBlock(id, &blockFlag));
        }
    }

    // make sure all the enqueued tasks are pending
    size_t postedCount = 0;
    size_t pendingCount = 0;
    for(int key = 0; key < sequenceKeyCount; ++key)
    {
        auto stats = sequencer.getStatistics(key);
        postedCount += stats.getPostedTaskCount();
        pendingCount += stats.getPendingTaskCount();
    }
    auto universalStats = sequencer.getStatistics();
    postedCount += universalStats.getPostedTaskCount();
    pendingCount += universalStats.getPendingTaskCount();

    EXPECT_EQ(sequenceKeyCount, (int)sequencer.getSequenceKeyCount());
    EXPECT_EQ((unsigned int)taskCount / 2, postedCount);
    // we expect one less because the first universal task starts running until it hits the block,
    // therefore all tasks are pending except one
    EXPECT_LE(((unsigned int)(taskCount / 2) - 1), pendingCount);
    EXPECT_GE((unsigned int)(taskCount / 2), pendingCount);
    // release the tasks
    blockFlag = false;

    // enqueue the second half
    for(SequencerExperimentalTestData::TaskId id = taskCount / 2; id < taskCount; ++id)
    {
        if ( id % universalTaskFrequency == 0 )
        {
            sequencer.enqueueAll(testData.makeTaskWithBlock(id, &blockFlag));
        }
        else
        {
            SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
            sequencer.enqueue(sequenceKey, testData.makeTaskWithBlock(id, &blockFlag));
        }
    }

    sequencer.drain();

    // check the final stats
    postedCount = 0;
    pendingCount = 0;
    for(int key = 0; key < sequenceKeyCount; ++key)
    {
        auto stats = sequencer.getStatistics(key);
        postedCount += stats.getPostedTaskCount();
        pendingCount += stats.getPendingTaskCount();
    }
    auto universalStatsAfter = sequencer.getStatistics();
    postedCount += universalStatsAfter.getPostedTaskCount();
    pendingCount += universalStatsAfter.getPendingTaskCount();

    EXPECT_EQ(sequenceKeyCount, (int)sequencer.getSequenceKeyCount());
    EXPECT_EQ((unsigned int)taskCount, postedCount-1); //-1 for drain()
    EXPECT_EQ(0u, pendingCount);
    EXPECT_EQ(taskCount, (int)sequencer.getTaskStatistics().getPostedTaskCount()-1); //-1 for drain()
    EXPECT_EQ(0u, sequencer.getTaskStatistics().getPendingTaskCount());
}

TEST_P(SequencerExperimentalTest, TaskOrderWithUniversal)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 50;
    const int sequenceKeyCount = 3;
    const int universalTaskFrequency = 11; // every 11th task is universal
    SequencerExperimentalTestData testData;
    SequencerExperimentalTestData::SequenceKeyMap sequenceKeys;
    std::vector<SequencerExperimentalTestData::TaskId> universal;

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());

    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        if ( id % universalTaskFrequency == 0 )
        {
            // save the task id as universal
            universal.push_back(id);
            sequencer.enqueueAll(testData.makeTask(id));
        }
        else
        {
            SequencerExperimentalTestData::SequenceKey sequenceKey = id % sequenceKeyCount;
            // save the task id for this sequenceKey
            sequenceKeys[sequenceKey].push_back(id);
            sequencer.enqueue(sequenceKey, testData.makeTask(id));
        }
    }
    sequencer.drain();

    EXPECT_EQ((int)testData.results().size(), taskCount);
    EXPECT_EQ((int)sequencer.getSequenceKeyCount(), sequenceKeyCount);

    // the tasks must be ordered within the same sequenceKey
    for(auto sequenceKeyData : sequenceKeys)
    {
        for(size_t i = 1; i < sequenceKeyData.second.size(); ++i)
        {
            testData.ensureOrder(sequenceKeyData.second[i-1], sequenceKeyData.second[i]);
        }
    }
    // all tasks enqueued before a universal task must be finished before it starts
    for (auto universalTaskId : universal)
    {
        for(SequencerExperimentalTestData::TaskId taskId = 0; taskId < universalTaskId; ++taskId)
        {
            testData.ensureOrder(taskId, universalTaskId);
        }
    }
    // all tasks enqueued after a universal task must be started after it finishes
    for (auto universalTaskId : universal)
    {
        for(SequencerExperimentalTestData::TaskId taskId = universalTaskId + 1; taskId < taskCount; ++taskId)
        {
            testData.ensureOrder(universalTaskId, taskId);
        }
    }
}

TEST_P(SequencerExperimentalTest, MultiSequenceKeyTasks)
{
    using namespace Bloomberg::quantum;

    const int sequenceKeyCount = 7;
    const int taskCount = (2 << (sequenceKeyCount - 1)) - 1;
    SequencerExperimentalTestData testData;

    // constructs a sequence key collection from the bitmap of an unsigned it,
    // e.g. for the taskId = 5, the returned collection will be {0, 2}
    auto getBitVector = [](unsigned int value)->std::vector<SequencerExperimentalTestData::SequenceKey>
    {
        std::vector<SequencerExperimentalTestData::SequenceKey> result;
        int bit = 0;
        for(; value; ++bit, value = value >> 1)
        {
            if ( value % 2 )
            {
                result.push_back(bit);
            }
        }
        return result;
    };

    SequencerExperimentalTestData::TaskSequencer sequencer(getDispatcher());
    for(SequencerExperimentalTestData::TaskId id = 1; id <= taskCount; ++id)
    {
        std::vector<SequencerExperimentalTestData::SequenceKey> sequenceKeys = getBitVector(id);
        // save the task id for this sequenceKey
        sequencer.enqueue(std::move(sequenceKeys), testData.makeTask(id));
    }
    sequencer.drain();

    EXPECT_EQ((int)testData.results().size(), taskCount);
    EXPECT_EQ((int)sequencer.getSequenceKeyCount(), sequenceKeyCount);

    // the tasks must be ordered within the sequenceKey set intersection
    for(SequencerExperimentalTestData::TaskId id = 1; id <= taskCount; ++id)
    {
        for(SequencerExperimentalTestData::TaskId refId = 1; refId <= taskCount; ++refId)
        {
            if (id != refId and id & refId)
            {
                if ( refId < id )
                {
                    testData.ensureOrder(refId, id);
                }
                else
                {
                    testData.ensureOrder(id, refId);
                }
            }
        }
    }
}

TEST_P(SequencerExperimentalTest, CustomHashFunction)
{
    using namespace Bloomberg::quantum;

    const int taskCount = 100;
    const int fullSequenceKeyCount = 20;
    const int restrictedSequenceKeyCount = 3;
    SequencerExperimentalTestData testData;
    SequencerExperimentalTestData::SequenceKeyMap sequenceKeys;

    // our custom hash value will be restricted to [0, restrictedSequenceKeyCount-1]
    using Hasher = std::function<size_t(SequencerExperimentalTestData::SequenceKey sequenceKeyId)>;
    Hasher customHasher = [](SequencerExperimentalTestData::SequenceKey sequenceKeyId)->size_t
    {
        return std::hash<SequencerExperimentalTestData::SequenceKey>()(sequenceKeyId % restrictedSequenceKeyCount);
    };

    // our custom equal operator will compare the hash values of sequenceKeys
    // this effectively means that the number of sequenceKeys is reduced to
    // restrictedSequenceKeyCount. So we get a lower memory consumption due to the
    // bound size of the hash table in experimental::Sequencer and hence need not to call
    // trimSequenceKeys from time to time. The price for this is a reduced parallelism:
    // instead of running at most fullSequenceKeyCount tasks in parallel, we can now run at most
    // restrictedSequenceKeyCount

    using KeyEqual = std::function<bool(SequencerExperimentalTestData::SequenceKey sequenceKeyId0, SequencerExperimentalTestData::SequenceKey sequenceKeyId1)>;
    const KeyEqual customEqual = [customHasher](SequencerExperimentalTestData::SequenceKey sequenceKeyId0, SequencerExperimentalTestData::SequenceKey sequenceKeyId1)->bool
    {
        return customHasher(sequenceKeyId0) == customHasher(sequenceKeyId1);
    };

    experimental::SequencerConfiguration<SequencerExperimentalTestData::SequenceKey, Hasher, KeyEqual> config;
    config.setBucketCount(0);
    config.setHash(customHasher);
    config.setKeyEqual(customEqual);

    experimental::Sequencer<SequencerExperimentalTestData::SequenceKey, Hasher, KeyEqual>
        sequencer(getDispatcher(), config);

    for(SequencerExperimentalTestData::TaskId id = 0; id < taskCount; ++id)
    {
        SequencerExperimentalTestData::SequenceKey sequenceKey = id % fullSequenceKeyCount;
        // save the task id for this sequenceKey
        sequenceKeys[sequenceKey].push_back(id);
        // enqueue the task with the real sequenceKey id
        sequencer.enqueue(std::move(sequenceKey), testData.makeTask(id));
    }
    sequencer.drain();

    EXPECT_EQ((int)testData.results().size(), taskCount);
    EXPECT_EQ((int)sequencer.getSequenceKeyCount(), restrictedSequenceKeyCount);

    // the tasks must be ordered within the same sequenceKey
    for(auto sequenceKeyData : sequenceKeys)
    {
        for(size_t i = 1; i < sequenceKeyData.second.size(); ++i)
        {
            testData.ensureOrder(sequenceKeyData.second[i-1], sequenceKeyData.second[i]);
        }
    }
}

TEST_P(SequencerExperimentalTest, PerformanceTest)
{
    using namespace Bloomberg::quantum;
    const int taskCount = 10000;
    const unsigned int sleepTime = 1000;

    testSequencerPerformance<SequencerExperimentalTestData::TaskSequencer>(
        "Highly dependent tasks",
        getDispatcher(),
        sleepTime,
        3,
        2,
        taskCount,
        10,
        1);

    testSequencerPerformance<SequencerExperimentalTestData::TaskSequencer>(
        "Independent tasks",
        getDispatcher(),
        sleepTime,
        taskCount,
        1,
        taskCount,
        0,
        0);
}

TEST_P(SequencerExperimentalTest, CoroSafety)
{
    // This test demonstrates that it is safe to call the experimental::Sequencer from within a
    // coroutine

    SequencerExperimentalTestData::TaskSequencer sequencer{ getDispatcher() };

    getDispatcher().post([&sequencer](VoidContextPtr) -> int {
        sequencer.enqueue(0, [](VoidContextPtr) -> int { return 0; });
        return 0;
    });

    getDispatcher().drain();
}

#endif // BLOOMBERG_QUANTUM_SEQUENCER_LITE_SUPPORT
