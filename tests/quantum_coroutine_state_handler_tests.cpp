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

#include <gtest/gtest.h>
#include <quantum/quantum.h>
#include <quantum_fixture.h>
#include <quantum_perf_utils.h>

using namespace Bloomberg;
using namespace quantum;
using ms = std::chrono::milliseconds;

namespace {

struct TaskParams
{
    size_t yieldIterations{0}; // number of times the task is supposed to yield
    ms sleepTime{30}; // sleep time between yield calls. If yieldIterations is 0, a task will sleep sleepTime ms
    bool randomSleepTime{false}; // if true the task will sleep random time in range (0, sleepTime)
    bool throwException{false}; // indicates if a task should throw exception
    size_t exceptionIteration{0}; // iteration on which exception is thrown
    ITask::RetCode returnCode{ITask::RetCode::Success}; // return code of the task
};

std::function<int(CoroContext<int>::Ptr)> makeTask(const TaskParams& taskParms)
{
    auto yield = [](CoroContext<int>::Ptr ctx) -> bool {
        ctx->yield();
        return true;
    };
    return [taskParms, yield](CoroContext<int>::Ptr ctx)-> int {
        size_t iteration = 0;
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
        } while ((iteration++ < taskParms.yieldIterations) && yield(ctx));

        return static_cast<int>(taskParms.returnCode);
    };
}

void throwException(CoroutineState)
{
    throw std::runtime_error("Coroutine state handler exception");
}

void createVariablesInLocalStorage(CoroutineState state)
{
    switch (state)
    {
        case quantum::CoroutineState::Constructed:
        {
            local::variable<size_t>("WholeLifetime") = new size_t(local::taskId().id());
            break;
        }
        case quantum::CoroutineState::Resumed:
        {
            local::variable<size_t>("ExecutionLifetime") = new size_t(local::taskId().id());
            break;
        }
        case quantum::CoroutineState::Suspended:
        {
            size_t*& var = local::variable<size_t>("ExecutionLifetime");
            if (var)
            {
                EXPECT_EQ(*var, local::taskId().id());
                delete var;
                var = nullptr;
            }
            break;
        }
        case quantum::CoroutineState::Destructed:
        {
            size_t*& var = local::variable<size_t>("WholeLifetime");
            EXPECT_TRUE(var);
            EXPECT_EQ(*var, local::taskId().id());
            delete var;
            var = nullptr;
            break;
        }
    }
}

class TestCoroutineStateHandler : public ::testing::Test
{
protected:
    TestCoroutineStateHandler()
    {
        srand((unsigned) time(NULL));
    }

    void testCoroutineStateHandler(
        const CoroutineStateHandler& coroutineStateHandler,
        size_t tasksCount,
        const TaskParams& taskParams)
    {
        CoroutineStateHandler handler{};
        if (coroutineStateHandler)
        {
            handler = [this, &coroutineStateHandler](CoroutineState state)
            {
                countCoroutineStates(state);
                if (coroutineStateHandler)
                {
                    coroutineStateHandler(state);
                }
            };
        }
        const TestConfiguration config(false, false, handler);
        auto dispatcher = DispatcherSingleton::createInstance(config);
        {
            Timer timer;
            for(size_t taskId = 0; taskId < tasksCount; ++taskId)
            {
                dispatcher->post((int)IQueue::QueueId::Any, false, makeTask(taskParams));
            }
            dispatcher->drain();
        }
        EXPECT_EQ(_constructed, _destructed);
        EXPECT_EQ(_constructed + _resumed, _suspended);

        std::cout
                << "Execution time: " <<  Timer::elapsed<std::chrono::milliseconds>() << "ms\n"
                << "constructed counter: " << _constructed << '\n'
                << "resumed counter: " << _resumed << '\n'
                << "suspended counter: " << _suspended << '\n'
                << "destructed counter: " << _destructed << std::endl;
    }

private:
    void countCoroutineStates(quantum::CoroutineState state)
    {
        switch (state)
        {
            case quantum::CoroutineState::Constructed:
                ++_constructed;
                break;
            case quantum::CoroutineState::Resumed:
                ++_resumed;
                break;
            case quantum::CoroutineState::Suspended:
                ++_suspended;
                break;
            case quantum::CoroutineState::Destructed:
                ++_destructed;
                break;
        }
    }

private:
    std::atomic_int _constructed = 0;
    std::atomic_int _resumed = 0;
    std::atomic_int _suspended = 0;
    std::atomic_int _destructed = 0;
};

} // namespace

//==============================================================================
//                             TEST CASES
//==============================================================================

TEST_F(TestCoroutineStateHandler, WithMultipleYieldsAnwWithEmptyHandler)
{
    testCoroutineStateHandler({}, 100, {2, ms(30), true});
}

TEST_F(TestCoroutineStateHandler, WithoutYield)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {0, ms(30), true});
}

TEST_F(TestCoroutineStateHandler, WithMultipleYields)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {2, ms(30), true});
}

TEST_F(TestCoroutineStateHandler, WithoutYieldAndWithTaskException)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {0, ms(30), true, true});
}

TEST_F(TestCoroutineStateHandler, WithYieldAndWithTaskException)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {1, ms(30), true, true, 1});
}

TEST_F(TestCoroutineStateHandler, WithoutYieldAndWithTaskExceptionCode)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {0, ms(30), true, false, 0, ITask::RetCode::Exception});
}

TEST_F(TestCoroutineStateHandler, WithYieldAndWithTaskExceptionCode)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 100, {1, ms(30), true, false, 0, ITask::RetCode::Exception});
}

TEST_F(TestCoroutineStateHandler, WithoutYieldAndWithHandlerException)
{
    testCoroutineStateHandler(throwException, 100, {0, ms(30)});
}

TEST_F(TestCoroutineStateHandler, WithYieldAndWithHandlerException)
{
    testCoroutineStateHandler(throwException, 100, {1, ms(30)});
}

TEST_F(TestCoroutineStateHandler, WithYieldAndWithHandlerExceptionAndWithTaskException)
{
    testCoroutineStateHandler(throwException, 100, {1, ms(30), true, true, 1});
}

TEST_F(TestCoroutineStateHandler, WithYieldAndWithHandlerExceptionAndWithTaskExceptionCode)
{
    testCoroutineStateHandler(throwException, 100, {1, ms(30), true, false, 0, ITask::RetCode::Exception});
}

TEST_F(TestCoroutineStateHandler, StressTest)
{
    testCoroutineStateHandler(createVariablesInLocalStorage, 1000, {2, ms(50), true});
}
