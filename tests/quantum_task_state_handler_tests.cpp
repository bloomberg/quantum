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
    size_t yieldIterations{0}; // A number of times the task is supposed to yield. Coroutines only support.
    bool randomYieldIterations{false}; // If true the task will yield a random number of times in range [0, yieldIterations]. Coroutines only support.
    ms sleepTime{30}; // Sleep time between yield calls. If yieldIterations is 0, a task will sleep sleepTime ms. For coroutine real work will take sleepTime / 2
    bool randomSleepTime{false}; // If true the task will sleep random time in range [0, sleepTime]
    bool throwException{false}; // Indicates if a task should throw exception
    size_t exceptionIteration{0}; // Iteration on which exception is thrown. Coroutines only support.
    Bloomberg::quantum::ITask::RetCode returnCode{Bloomberg::quantum::ITask::RetCode::Success}; // Return code of the task
};

std::function<int(Bloomberg::quantum::CoroContext<int>::Ptr)> makeCoroutineTask(const TaskParams& taskParms)
{
    return [taskParms](Bloomberg::quantum::CoroContext<int>::Ptr ctx)-> int {
        size_t yieldIterations = taskParms.yieldIterations;
        if (taskParms.randomYieldIterations)
        {
            yieldIterations = rand() % (taskParms.yieldIterations + 1);
        }

        size_t iteration = 0;
        while (iteration++ < yieldIterations)
        {
            ms sleepTime = taskParms.sleepTime;
            if (taskParms.randomSleepTime && (sleepTime.count() > 0))
            {
                sleepTime = static_cast<ms>(rand() % sleepTime.count() + 1);
            }
            ctx->sleep(sleepTime);
            std::this_thread::sleep_for(static_cast<ms>(sleepTime.count() / 2));
            if (taskParms.throwException && (taskParms.exceptionIteration == iteration))
            {
                throw std::runtime_error("Unexpected error");
            }
        }

        return static_cast<int>(taskParms.returnCode);
    };
}

std::function<int(Bloomberg::quantum::ThreadPromise<int>::Ptr)> makeIoTask(const TaskParams& taskParms)
{
    return [taskParms](Bloomberg::quantum::ThreadPromise<int>::Ptr)-> int {
        ms sleepTime = taskParms.sleepTime;
        if (taskParms.randomSleepTime)
        {
            sleepTime = static_cast<ms>(rand() % taskParms.sleepTime.count() + 1);
        }
        std::this_thread::sleep_for(sleepTime);
        if (taskParms.throwException)
        {
            throw std::runtime_error("Unexpected error");
        }
        return static_cast<int>(taskParms.returnCode);
    };
}

template <typename BitField>
BitField unify(BitField lhs, BitField rhs)
{
    return static_cast<BitField>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

struct TaskStatesCounter {
    explicit TaskStatesCounter(int initialized = 0,
                               int started = 0,
                               int resumed = 0,
                               int suspended = 0,
                               int stopped = 0)
        : _initialized(initialized)
        , _started(started)
        , _resumed(resumed)
        , _suspended(suspended)
        , _stopped(stopped)
    {}
    void operator()(Bloomberg::quantum::TaskState state)
    {
        switch (state)
        {
            case Bloomberg::quantum::TaskState::Initialized:
                ++_initialized;
                break;
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

    void clear() {
        _initialized = _started = _resumed = _suspended = _stopped = 0;
    }

    friend std::ostream& operator<<(std::ostream& out, const TaskStatesCounter& taskStatesCounter)
    {
        out << "initialized: " << taskStatesCounter._initialized << '\n'
            << "started: " << taskStatesCounter._started << '\n'
            << "resumed: " << taskStatesCounter._resumed << '\n'
            << "suspended: " << taskStatesCounter._suspended << '\n'
            << "stopped: " << taskStatesCounter._stopped;
        return out;
    }

    std::atomic_int _initialized;
    std::atomic_int _started;
    std::atomic_int _resumed;
    std::atomic_int _suspended;
    std::atomic_int _stopped;
};


class TaskStateHandlerTest :
    public ::testing::TestWithParam<std::tuple<Bloomberg::quantum::TaskType,
                                               Bloomberg::quantum::TaskStateHandler>>
{
protected:
    TaskStateHandlerTest()
    {
        srand((unsigned) time(NULL));
    }

    void SetUp() override {
        _handledTaskType = std::get<0>(GetParam());
        _taskStateHandler = std::get<1>(GetParam());
    }

    void testTaskStateHandler(
        size_t tasksCount,
        const TaskParams& taskParams,
        bool coroutineSharingForAny = false,
        bool loadBalanceSharedIoQueues = false,
        Bloomberg::quantum::TaskState handledTaskStates = Bloomberg::quantum::TaskState::All,
        Bloomberg::quantum::TaskStateHandler taskStateHandler = {})
    {
        TaskStatesCounter taskStatesCounter;
        if (taskStateHandler)
        {
            _taskStateHandler = taskStateHandler;
        }

        if (_taskStateHandler)
        {
            taskStateHandler = [this, &taskStatesCounter](size_t taskId, int queueId, Bloomberg::quantum::TaskState state)
            {
                taskStatesCounter(state);
                _taskStateHandler(taskId, queueId, state);
            };
        }

        Bloomberg::quantum::TaskStateConfig taskStateConfig {
            taskStateHandler,
            handledTaskStates,
            _handledTaskType
        };
        const TestConfiguration config(loadBalanceSharedIoQueues,
                                       coroutineSharingForAny,
                                       taskStateConfig);
        auto dispatcher = DispatcherSingleton::createInstance(config);
        for(size_t taskId = 0; taskId < tasksCount; ++taskId)
        {
            switch (_handledTaskType)
            {
                case Bloomberg::quantum::TaskType::None:
                    break;
                case Bloomberg::quantum::TaskType::Coroutine: {
                    dispatcher->post(makeCoroutineTask(taskParams));
                    break;
                }
                case Bloomberg::quantum::TaskType::IoTask: {
                    dispatcher->postAsyncIo(makeIoTask(taskParams));
                    break;
                }
                default:
                    break;
            }
        }
        dispatcher->drain();

        std::cout << taskStatesCounter << std::endl;
        EXPECT_EQ(taskStatesCounter._initialized, 0);
        EXPECT_EQ(taskStatesCounter._started, taskStatesCounter._stopped);
        EXPECT_EQ(taskStatesCounter._resumed, taskStatesCounter._suspended);
        if (Bloomberg::quantum::TaskType::IoTask == _handledTaskType)
        {
            EXPECT_EQ(taskStatesCounter._resumed, 0);
        }
    }

    Bloomberg::quantum::TaskType _handledTaskType;
    Bloomberg::quantum::TaskStateHandler _taskStateHandler;
};

// Handlers
Bloomberg::quantum::TaskStateHandler EmptyHandler = [](size_t, int, Bloomberg::quantum::TaskState){};
Bloomberg::quantum::TaskStateHandler MemoryManagementHandler = TestTaskStateHandler();

// Task handled states
const auto StartedAndStoppedHandledStates = unify(Bloomberg::quantum::TaskState::Started,
                                                  Bloomberg::quantum::TaskState::Stopped);
const auto ResumedAndSuspendedHandledStates = unify(Bloomberg::quantum::TaskState::Resumed,
                                                    Bloomberg::quantum::TaskState::Suspended);

void testTaskStatesCounter(const TaskStatesCounter& expected, const TaskStatesCounter& actual)
{
    EXPECT_EQ(expected._initialized, actual._initialized);
    EXPECT_EQ(expected._started, actual._started);
    EXPECT_EQ(expected._resumed, actual._resumed);
    EXPECT_EQ(expected._suspended, actual._suspended);
    EXPECT_EQ(expected._stopped, actual._stopped);
}

void testHandleTaskType(Bloomberg::quantum::TaskType taskType,
                        size_t tasksCount,
                        const TaskParams& taskParams,
                        const TaskStatesCounter& expectedTaskStatesCounter,
                        bool loadBalance = false,
                        bool coroutineSharingForAny = false,
                        bool isHandlerAvailable = true
                        )
{
    TaskStatesCounter actualTaskStatesCounter;
    Bloomberg::quantum::TaskStateConfig taskStateConfig {
        [&actualTaskStatesCounter](size_t, int, Bloomberg::quantum::TaskState state) { actualTaskStatesCounter(state); },
        Bloomberg::quantum::TaskState::All,
        taskType
    };

    if (not isHandlerAvailable)
    {
        taskStateConfig.handler = {};
    }

    auto dispatcher = DispatcherSingleton::createInstance(
        TestConfiguration(loadBalance, coroutineSharingForAny, taskStateConfig));
    for (size_t taskId = 0; taskId < tasksCount; ++taskId)
    {
        dispatcher->post(makeCoroutineTask(taskParams));
        dispatcher->postAsyncIo(makeIoTask(taskParams));
    }
    dispatcher->drain();

    std::cout << actualTaskStatesCounter << std::endl;
    testTaskStatesCounter(expectedTaskStatesCounter, actualTaskStatesCounter);
}

void testHandleTaskState(const std::vector<Bloomberg::quantum::TaskState>& states,
                         Bloomberg::quantum::TaskState handledStates,
                         const TaskStatesCounter& expectedTaskStatesCounter)
{
    TaskStatesCounter actualTaskStatesCounter;
    bool isHandlerCalled = false;
    auto handler = [&isHandlerCalled, &actualTaskStatesCounter](size_t, int, Bloomberg::quantum::TaskState state)
    {
        actualTaskStatesCounter(state);
        isHandlerCalled = true;
    };

    auto state = Bloomberg::quantum::TaskState::Initialized;
    for (auto nextState : states)
    {
        isHandlerCalled = false;
        handleTaskState(handler,
                    0,
                    0,
                    handledStates,
                    nextState,
                    state);
        EXPECT_TRUE(Bloomberg::quantum::isIntersection(handledStates, nextState) == isHandlerCalled);
        EXPECT_EQ(state, nextState);
    }

    testTaskStatesCounter(expectedTaskStatesCounter, actualTaskStatesCounter);
}


} // namespace

INSTANTIATE_TEST_CASE_P(TaskStateHandlerTest_Default,
                         TaskStateHandlerTest,
                         ::testing::Combine(
                            ::testing::Values(
                                Bloomberg::quantum::TaskType::Coroutine,
                                Bloomberg::quantum::TaskType::IoTask
                            ),
                            ::testing::Values(
                                TestTaskStateHandler())));

//==============================================================================
//                             TEST CASES
//==============================================================================

TEST(TestTaskStateHandler, UnableToHandleTaskState)
{
    TaskStatesCounter taskStatesCounter;
    bool isHandlerCalled = false;
    auto handler = [&isHandlerCalled, &taskStatesCounter](size_t, int, Bloomberg::quantum::TaskState state)
    {
        taskStatesCounter(state);
        isHandlerCalled = true;
    };

    // Wrong task state order
    auto state = Bloomberg::quantum::TaskState::Stopped;
    handleTaskState(handler,
                    0,
                    0,
                    Bloomberg::quantum::TaskState::None,
                    Bloomberg::quantum::TaskState::Started,
                    state);
    EXPECT_FALSE(isHandlerCalled);
    EXPECT_EQ(state, Bloomberg::quantum::TaskState::Stopped);

    // No states processed
    state = Bloomberg::quantum::TaskState::Initialized;
    handleTaskState(handler,
                    0,
                    0,
                    Bloomberg::quantum::TaskState::None,
                    Bloomberg::quantum::TaskState::Started,
                    state);
    EXPECT_FALSE(isHandlerCalled);
    EXPECT_EQ(state, Bloomberg::quantum::TaskState::Started);

    // Missing handled state
    handleTaskState(handler,
                    0,
                    0,
                    unify(Bloomberg::quantum::TaskState::Started,
                          Bloomberg::quantum::TaskState::Stopped),
                    Bloomberg::quantum::TaskState::Suspended,
                    state);
    EXPECT_FALSE(isHandlerCalled);
    EXPECT_EQ(state, Bloomberg::quantum::TaskState::Suspended);
}

TEST(TestTaskStateHandler, HandleTaskState)
{
    // [Initialized -> Started -> [Suspended -> Resumed] x 2 -> Stopped]
    const std::vector<Bloomberg::quantum::TaskState> fullStatesSequence = {
        Bloomberg::quantum::TaskState::Started,
        Bloomberg::quantum::TaskState::Suspended,
        Bloomberg::quantum::TaskState::Resumed,
        Bloomberg::quantum::TaskState::Suspended,
        Bloomberg::quantum::TaskState::Resumed,
        Bloomberg::quantum::TaskState::Stopped
    };

    // [Initialized -> Started -> Stopped]
    const std::vector<Bloomberg::quantum::TaskState> startedAndStoppedSequence = {
        Bloomberg::quantum::TaskState::Started,
        Bloomberg::quantum::TaskState::Stopped
    };
    testHandleTaskState(fullStatesSequence,
                        Bloomberg::quantum::TaskState::All,
                        TaskStatesCounter(0, 1, 2, 2, 1));

    testHandleTaskState(startedAndStoppedSequence,
                        Bloomberg::quantum::TaskState::All,
                        TaskStatesCounter(0, 1, 0, 0, 1));

    testHandleTaskState(fullStatesSequence,
                        StartedAndStoppedHandledStates,
                        TaskStatesCounter(0, 1, 0, 0, 1));

    testHandleTaskState(startedAndStoppedSequence,
                        StartedAndStoppedHandledStates,
                        TaskStatesCounter(0, 1, 0, 0, 1));

    testHandleTaskState(fullStatesSequence,
                        ResumedAndSuspendedHandledStates,
                        TaskStatesCounter(0, 0, 2, 2, 0));

    testHandleTaskState(startedAndStoppedSequence,
                        ResumedAndSuspendedHandledStates,
                        TaskStatesCounter(0, 0, 0, 0, 0));

}

TEST(TestTaskStateHandler, HandleDifferentTaskTypes)
{
    const TaskParams taskParams{1, false, ms(100), true};
    const size_t tasksCount = 100;

    // No task state handling
    testHandleTaskType(Bloomberg::quantum::TaskType::None,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, 0, 0, 0, 0));

    // Coroutine state handling

    // Without shared coroutine queue
    testHandleTaskType(Bloomberg::quantum::TaskType::Coroutine,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, tasksCount, tasksCount, tasksCount, tasksCount),
                       false,
                       false);

    // With shared coroutine queue
    testHandleTaskType(Bloomberg::quantum::TaskType::Coroutine,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, tasksCount, tasksCount, tasksCount, tasksCount),
                       false,
                       true);

    // Without handler
    testHandleTaskType(Bloomberg::quantum::TaskType::Coroutine,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, 0, 0, 0, 0),
                       false,
                       false,
                       false);

    // IoTask state handling

    // Without shared IO queue
    testHandleTaskType(Bloomberg::quantum::TaskType::IoTask,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, tasksCount, 0, 0, tasksCount),
                       false,
                       false);

    // With shared IO queue
    testHandleTaskType(Bloomberg::quantum::TaskType::IoTask,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, tasksCount, 0, 0, tasksCount),
                       true,
                       false);

    // Without handler
    testHandleTaskType(Bloomberg::quantum::TaskType::IoTask,
                       tasksCount,
                       taskParams,
                       TaskStatesCounter(0, 0, 0, 0, 0),
                       false,
                       false,
                       false);
}

TEST_P(TaskStateHandlerTest, HandleNoneTaskStates)
{
    testTaskStateHandler(100, {3, true, ms(100), true}, false, false, Bloomberg::quantum::TaskState::None);
}

TEST_P(TaskStateHandlerTest, HandleStartedAndStoppedTaskStates)
{
    testTaskStateHandler(100, {3, true, ms(100), true}, false, false, StartedAndStoppedHandledStates);
}

TEST_P(TaskStateHandlerTest, HandleResumedAndSuspendedTaskStates)
{
    // Use empty handler here to avoid issues with checks in the default handler
    testTaskStateHandler(100, {3, true, ms(100), true}, false, false, ResumedAndSuspendedHandledStates, EmptyHandler);
}

TEST_P(TaskStateHandlerTest, HandleAllTaskStates)
{
    testTaskStateHandler(100, {3, true, ms(100), true}, false, false, Bloomberg::quantum::TaskState::All);
}

TEST_P(TaskStateHandlerTest, NoYield)
{
    testTaskStateHandler(100, {0, false, ms(100), true});
}

TEST_P(TaskStateHandlerTest, MultipleYields)
{
    testTaskStateHandler(100, {3, true, ms(100), true});
}

TEST_P(TaskStateHandlerTest, NoYieldSharedQueue)
{
    testTaskStateHandler(100, {0, false, ms(100), true}, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsSharedQueue)
{
    testTaskStateHandler(100, {3, true, ms(100), true}, true);
}

TEST_P(TaskStateHandlerTest, NoYieldLoadBalanceSharedIoQueues)
{
    testTaskStateHandler(100, {0, false, ms(100), true}, false, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsLoadBalanceSharedIoQueues)
{
    testTaskStateHandler(100, {3, true, ms(100), true}, false, true);
}

TEST_P(TaskStateHandlerTest, NoYieldTaskException)
{
    testTaskStateHandler(100, {0, false, ms(100), true, true});
}

TEST_P(TaskStateHandlerTest, MultipleYieldsException)
{
    testTaskStateHandler(100, {2, false, ms(100), true, true, 1});
}

TEST_P(TaskStateHandlerTest, NoYieldTaskExceptionSharedQueue)
{
    testTaskStateHandler(100, {0, false, ms(100), true, true}, true);
}

TEST_P(TaskStateHandlerTest, MultipleYieldsTaskExceptionSharedQueue)
{
    testTaskStateHandler(100, {2, false, ms(100), true, true, 1}, true);
}

TEST_P(TaskStateHandlerTest, NoYieldTaskCodeException)
{
    testTaskStateHandler(100, {0, false, ms(100), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(TaskStateHandlerTest, MultipleYieldsTaskCodeException)
{
    testTaskStateHandler(100, {2, false, ms(100), true, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(TaskStateHandlerTest, LongRunningTask)
{
    testTaskStateHandler(20, {2, true, ms(1000), false, false, 0, Bloomberg::quantum::ITask::RetCode::Exception});
}

TEST_P(TaskStateHandlerTest, StressTest)
{
    testTaskStateHandler(5000, {2, true, ms(50), true});
}
