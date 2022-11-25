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

class CoroutineStateHandlerTest : public ::testing::TestWithParam<Bloomberg::quantum::CoroutineStateHandler>
{
protected:
    CoroutineStateHandlerTest():
        _constructed(0),
        _resumed(0),
        _suspended(0),
        _destructed(0)
    {
        srand((unsigned) time(NULL));
    }

    void testCoroutineStateHandler(
        size_t tasksCount,
        const TaskParams& taskParams,
        bool coroutineSharingForAny = false,
        bool loadBalanceSharedIoQueues = false)
    {
        Bloomberg::quantum::CoroutineStateHandler finalStateHandler{};
        auto& coroutineStateHandler = GetParam();
        if (coroutineStateHandler)
        {
            finalStateHandler = [this, coroutineStateHandler](Bloomberg::quantum::CoroutineState state)
            {
                countCoroutineStates(state);
                coroutineStateHandler(state);
            };
        }
        const TestConfiguration config(loadBalanceSharedIoQueues,
                                       coroutineSharingForAny,
                                       finalStateHandler);
        auto dispatcher = DispatcherSingleton::createInstance(config);
        dispatcher->drain();
        for(size_t taskId = 0; taskId < tasksCount; ++taskId)
        {
            dispatcher->post((int)Bloomberg::quantum::IQueue::QueueId::Any, false, makeTask(taskParams));
        }
        dispatcher->drain();

        std::cout << "Counters" << '\n'
                  << "constructed: " << _constructed << '\n'
                  << "resumed: " << _resumed << '\n'
                  << "suspended: " << _suspended << '\n'
                  << "destructed: " << _destructed << std::endl;

        EXPECT_EQ(_constructed, _destructed);
        EXPECT_EQ(_constructed + _resumed, _suspended);
    }

private:
    void countCoroutineStates(Bloomberg::quantum::CoroutineState state)
    {
        switch (state)
        {
            case Bloomberg::quantum::CoroutineState::Constructed:
                ++_constructed;
                break;
            case Bloomberg::quantum::CoroutineState::Resumed:
                ++_resumed;
                break;
            case Bloomberg::quantum::CoroutineState::Suspended:
                ++_suspended;
                break;
            case Bloomberg::quantum::CoroutineState::Destructed:
                ++_destructed;
                break;
        }
    }

private:
    std::atomic_int _constructed;
    std::atomic_int _resumed;
    std::atomic_int _suspended;
    std::atomic_int _destructed;
};


// Handlers
Bloomberg::quantum::CoroutineStateHandler emptyHandler{};
Bloomberg::quantum::CoroutineStateHandler exceptionThrowingHandler = [](Bloomberg::quantum::CoroutineState)
{
    throw std::runtime_error("Coroutine state handler exception");
};
Bloomberg::quantum::CoroutineStateHandler memoryManagementHandler = TestCoroutineStateHandler();

} // namespace

INSTANTIATE_TEST_CASE_P(CoroutineStateHandlerTest_Default,
                        CoroutineStateHandlerTest,
                            ::testing::Values(emptyHandler,
                                              exceptionThrowingHandler,
                                              memoryManagementHandler));

//==============================================================================
//                             TEST CASES
//==============================================================================

TEST_P(CoroutineStateHandlerTest, NoYield)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true});
}

TEST_P(CoroutineStateHandlerTest, MultipleYields)
{
    testCoroutineStateHandler(100, {3, true, ms(30), true});
}

TEST_P(CoroutineStateHandlerTest, NoYieldSharedQueue)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true}, true);
}

TEST_P(CoroutineStateHandlerTest, MultipleYieldsSharedQueue)
{
    testCoroutineStateHandler(100, {3, true, ms(30), true}, true);
}

TEST_P(CoroutineStateHandlerTest, NoYieldLoadBalanceSharedIoQueues)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true}, false, true);
}

TEST_P(CoroutineStateHandlerTest, MultipleYieldsLoadBalanceSharedIoQueues)
{
    testCoroutineStateHandler(100, {3, true, ms(30), true}, false, true);
}

TEST_P(CoroutineStateHandlerTest, NoYieldTaskException)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true, true});
}

TEST_P(CoroutineStateHandlerTest, MultipleYieldsException)
{
    testCoroutineStateHandler(100, {2, false, ms(30), true, true, 1});
}

TEST_P(CoroutineStateHandlerTest, NoYieldTaskExceptionSharedQueue)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true, true}, true);
}

TEST_P(CoroutineStateHandlerTest, MultipleYieldsTaskExceptionSharedQueue)
{
    testCoroutineStateHandler(100, {2, false, ms(30), true, true, 1}, true);
}

TEST_P(CoroutineStateHandlerTest, NoYieldTaskCodeException)
{
    testCoroutineStateHandler(100, {0, false, ms(30), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(CoroutineStateHandlerTest, MultipleYieldsTaskCodeException)
{
    testCoroutineStateHandler(100, {2, false, ms(30), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(CoroutineStateHandlerTest, StressTest)
{
    testCoroutineStateHandler(1000, {3, true, ms(20), true});
}
