/*
** Copyright 2022 Bloomberg Finance L.P.
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

#include <cstdlib>
#include <atomic>

#include <gtest/gtest.h>
#include <quantum/quantum.h>
#include <quantum_fixture.h>


namespace {

struct TaskParams
{
    size_t yieldIterations{0}; // number of times the task is supposed to yield
    bool randomYieldIterations{false}; // if true the task will yield a random number of times in range [0, yieldIterations]
    ms sleepTime{30}; // sleep time between yield calls. If yieldIterations is 0, a task will sleep sleepTime ms
    bool randomSleepTime{false}; // if true the task will sleep random time in range [0, sleepTime]
    bool throwException{false}; // indicates if a task should throw exception
    size_t exceptionIteration{0}; // iteration on which exception is thrown
    Bloomberg::quantum::ITask::RetCode returnCode{Bloomberg::quantum::ITask::RetCode::Success}; // return code of the task
};

std::function<int(Bloomberg::quantum::CoroContext<int>::Ptr)> makeTask(const TaskParams& taskParms)
{
    auto yield = [](Bloomberg::quantum::CoroContext<int>::Ptr ctx) -> bool {
        ctx->yield();
        return true;
    };
    return [taskParms, yield](Bloomberg::quantum::CoroContext<int>::Ptr ctx)-> int {
        size_t iteration = 0;
        size_t yieldIterations = taskParms.yieldIterations;
        if (taskParms.randomYieldIterations)
        {
            yieldIterations = rand() % (taskParms.yieldIterations + 1);
        }

        do {
            ms sleepTime = taskParms.sleepTime;
            if (taskParms.randomSleepTime)
            {
                sleepTime = static_cast<ms>(rand() % taskParms.sleepTime.count() + 1);
            }
            std::this_thread::sleep_for(sleepTime);
            if (taskParms.throwException && (taskParms.exceptionIteration == iteration))
            {
                throw std::runtime_error("Unexpected error");
            }
        } while ((iteration++ < yieldIterations) && yield(ctx));

        return static_cast<int>(taskParms.returnCode);
    };
}

class TaskStateHandlerTest : public ::testing::TestWithParam<Bloomberg::quantum::TaskStateHandler>
{
protected:
    TaskStateHandlerTest():
        _started(0),
        _resumed(0),
        _suspended(0),
        _stopped(0)
    {
        srand((unsigned) time(NULL));
    }

    void testTaskStateHandler(
        size_t tasksCount,
        const TaskParams& taskParams,
        bool coroutineSharingForAny = false,
        bool loadBalanceSharedIoQueues = false)
    {
        Bloomberg::quantum::TaskStateHandler finalStateHandler{};
        auto& taskStateHandler = GetParam();
        if (taskStateHandler)
        {
            finalStateHandler = [this, taskStateHandler](size_t taskId, int queueId, Bloomberg::quantum::TaskState state)
            {
                countCoroutineStates(state);
                taskStateHandler(taskId, queueId, state);
            };
        }

        Bloomberg::quantum::TaskStateConfig taskStateConfig {
            finalStateHandler,
            Bloomberg::quantum::TaskState::All,
            Bloomberg::quantum::TaskType::Coroutine
        };
        const TestConfiguration config(loadBalanceSharedIoQueues,
                                       coroutineSharingForAny,
                                       taskStateConfig);
        auto dispatcher = DispatcherSingleton::createInstance(config);
        dispatcher->drain();
        for(size_t taskId = 0; taskId < tasksCount; ++taskId)
        {
            dispatcher->post((int)Bloomberg::quantum::IQueue::QueueId::Any, false, makeTask(taskParams));
        }
        dispatcher->drain();

        std::cout << "Counters" << '\n'
                  << "constructed: " << _started << '\n'
                  << "resumed: " << _resumed << '\n'
                  << "suspended: " << _suspended << '\n'
                  << "destructed: " << _stopped << std::endl;

        EXPECT_EQ(_started, _stopped);
        EXPECT_EQ(_started + _resumed, _suspended);
    }

private:
    void countCoroutineStates(Bloomberg::quantum::TaskState state)
    {
        switch (state)
        {
            case Bloomberg::quantum::TaskState::Started:
                ++_started;
                break;
            case Bloomberg::quantum::TaskState::Resumed:
                ++_resumed;
                break;
            case Bloomberg::quantum::TaskState::Suspended:
                ++_suspended;
                break;
            case Bloomberg::quantum::TaskState::Stopped:
                ++_stopped;
                break;
            default:
                break;
        }
    }

private:
    std::atomic_int _started;
    std::atomic_int _resumed;
    std::atomic_int _suspended;
    std::atomic_int _stopped;
};


// Handlers
Bloomberg::quantum::TaskStateHandler emptyHandler{};
Bloomberg::quantum::TaskStateHandler exceptionThrowingHandler = [](size_t, int, Bloomberg::quantum::TaskState)
{
    throw std::runtime_error("Coroutine state handler exception");
};
Bloomberg::quantum::TaskStateHandler memoryManagementHandler = TestTaskStateHandler();

} // namespace

INSTANTIATE_TEST_CASE_P(TaskStateHandlerTest_Default,
                        TaskStateHandlerTest,
                            ::testing::Values(emptyHandler,
                                              exceptionThrowingHandler,
                                              memoryManagementHandler));

//==============================================================================
//                             TEST CASES
//==============================================================================

TEST_P(TaskStateHandlerTest, )
{
    // Task Config

    // 1. Task types
    // 1.1. Coroutine
    // 1.2. IoTask
    // 1.3. Both

    // 2. Handled states
    // 2.1. None
    // 2.2. Initialized
    // 2.3. Started
    // 2.4. Suspended
    // 2.5. Resumed
    // 2.6. Stopped
    // 2.2. Started/Stopped
    // 2.3. Resumed/Suspended
    // 2.4. All
}


TEST_P(TaskStateHandlerTest, NoYield)
{
    testTaskStateHandler(100, {0, false, ms(30), true});
}

TEST_P(TaskStateHandlerTest, MultipleYields)
{
    testTaskStateHandler(100, {3, true, ms(30), true});
}

TEST_P(TaskStateHandlerTest, NoYieldSharedQueue)
{
    testTaskStateHandler(100, {0, false, ms(30), true}, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsSharedQueue)
{
    testTaskStateHandler(100, {3, true, ms(30), true}, true);
}

TEST_P(TaskStateHandlerTest, NoYieldLoadBalanceSharedIoQueues)
{
    testTaskStateHandler(100, {0, false, ms(30), true}, false, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsLoadBalanceSharedIoQueues)
{
    testTaskStateHandler(100, {3, true, ms(30), true}, false, true);
}

TEST_P(TaskStateHandlerTest, NoYieldTaskException)
{
    testTaskStateHandler(100, {0, false, ms(30), true, true});
}

TEST_P(TaskStateHandlerTest, MultipleYieldsException)
{
    testTaskStateHandler(100, {2, false, ms(30), true, true, 1});
}

TEST_P(TaskStateHandlerTest, NoYieldTaskExceptionSharedQueue)
{
    testTaskStateHandler(100, {0, false, ms(30), true, true}, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsTaskExceptionSharedQueue)
{
    testTaskStateHandler(100, {2, false, ms(30), true, true, 1}, true);
}

TEST_P(TaskStateHandlerTest, NoYieldTaskCodeException)
{
    testTaskStateHandler(100, {0, false, ms(30), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(TaskStateHandlerTest, MultipleYieldsTaskCodeException)
{
    testTaskStateHandler(100, {2, false, ms(30), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(TaskStateHandlerTest, StressTest)
{
    testTaskStateHandler(1000, {3, true, ms(20), true});
}
