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

#include <quantum_fixture.h>
#include <sstream>

using namespace Bloomberg;

class TestTaskStateHandler::TestTaskStateHandlerImpl
{
public:
    void operator()(size_t taskId, int queueId, Bloomberg::quantum::TaskType type, Bloomberg::quantum::TaskState state)
    {
        switch (state)
        {
            case Bloomberg::quantum::TaskState::Started:
            {
                size_t*& startedId = Bloomberg::quantum::local::variable<size_t>("StartedId");
                EXPECT_FALSE(startedId);
                startedId = new size_t(Bloomberg::quantum::local::taskId().id());
                break;
            }
            case Bloomberg::quantum::TaskState::Resumed:
            {
                // Check that suspended id exists, equals to the task id and removes it
                size_t*& suspendedId = Bloomberg::quantum::local::variable<size_t>("SuspendedId");
                EXPECT_TRUE(suspendedId);
                EXPECT_EQ(*suspendedId, Bloomberg::quantum::local::taskId().id());
                delete suspendedId;
                suspendedId = nullptr;

                // Create a resumed id
                size_t*& resumedId = Bloomberg::quantum::local::variable<size_t>("ResumedId");
                EXPECT_FALSE(resumedId);
                resumedId = new size_t(Bloomberg::quantum::local::taskId().id());
                break;
            }
            case Bloomberg::quantum::TaskState::Suspended:
            {
                // Call this handler with 'Stopped' task state to handle stored variables
                (*this)(taskId, queueId, type, Bloomberg::quantum::TaskState::Stopped);
                // Create a suspended id
                size_t*& suspendedId = Bloomberg::quantum::local::variable<size_t>("SuspendedId");
                EXPECT_FALSE(suspendedId);
                suspendedId = new size_t(Bloomberg::quantum::local::taskId().id());
                break;
            }
            case Bloomberg::quantum::TaskState::Stopped:
            {
                size_t*& startedId = Bloomberg::quantum::local::variable<size_t>("StartedId");
                size_t*& resumedId = Bloomberg::quantum::local::variable<size_t>("ResumedId");

                // Only one id must exist
                EXPECT_FALSE(startedId && resumedId);
                EXPECT_TRUE(startedId || resumedId);

                // If started id exists check that it equals to the task id and removes it
                if (startedId)
                {
                    EXPECT_EQ(*startedId, Bloomberg::quantum::local::taskId().id());
                    delete startedId;
                    startedId = nullptr;
                }

                // If resumed id exists check that it equals to the task id and removes it
                if (resumedId)
                {
                    EXPECT_EQ(*resumedId, Bloomberg::quantum::local::taskId().id());
                    delete resumedId;
                    resumedId = nullptr;
                }
                break;
            }

            default:
                break;
        }
    }
};

TestTaskStateHandler::TestTaskStateHandler():
    _impl(std::make_shared<TestTaskStateHandlerImpl>())
{}

void TestTaskStateHandler::operator()(size_t taskId, int queueId, Bloomberg::quantum::TaskType type, Bloomberg::quantum::TaskState state)
{
    _impl->operator()(taskId, queueId, type, state);
}

Bloomberg::quantum::TaskStateConfiguration getDefaultTaskStateConfiguration()
{
    Bloomberg::quantum::TaskStateConfiguration config;
    config.setTaskStateHandler(TestTaskStateHandler());
    config.setHandledTaskStates(Bloomberg::quantum::TaskState::All);
    config.setHandledTaskTypes(Bloomberg::quantum::TaskType::Coroutine);
    return config;
}

TestConfiguration::TestConfiguration(bool loadBalance,
                                     bool coroutineSharingForAny,
                                     const Bloomberg::quantum::TaskStateConfiguration& TaskStateConfiguration):
    _loadBalance(loadBalance),
    _coroutineSharingForAny(coroutineSharingForAny),
    _TaskStateConfiguration(TaskStateConfiguration)
{}



std::shared_ptr<Bloomberg::quantum::Dispatcher>
DispatcherSingleton::createInstance(const TestConfiguration& taskConfig)
{
    Bloomberg::quantum::Configuration config;
    config.setNumCoroutineThreads(numCoro);
    config.setNumIoThreads(numThreads);
    config.setLoadBalanceSharedIoQueues(taskConfig._loadBalance);
    config.setLoadBalancePollIntervalMs(std::chrono::milliseconds(10));
    config.setCoroQueueIdRangeForAny(std::make_pair(1,numCoro-1));
    config.setCoroutineSharingForAny(taskConfig._coroutineSharingForAny);
    config.setTaskStateConfiguration(taskConfig._TaskStateConfiguration);
    return std::make_shared<Bloomberg::quantum::Dispatcher>(config);
}

Bloomberg::quantum::Dispatcher&
DispatcherSingleton::instance(const TestConfiguration& config)
{
    auto it = _dispatchers.find(config);
    if (it == _dispatchers.end())
    {
        it = _dispatchers.emplace(config, createInstance(config)).first;
    }
    return *it->second;
}

void DispatcherSingleton::deleteInstances()
{
    _dispatchers.clear();
}


DispatcherFixture::DispatcherFixture():
    _dispatcher()
{
    Bloomberg::quantum::StackTraits::defaultSize() = 1 << 14; //16k stack for testing
}

void DispatcherFixture::SetUp()
{
    _dispatcher = &DispatcherSingleton::instance(GetParam());
    //Don't drain in the TearDown() because of the final CleanupTest::DeleteDispatcherInstance()
    _dispatcher->drain();
    _dispatcher->resetStats();
}

void DispatcherFixture::TearDown()
{
    _dispatcher = nullptr;
}

Bloomberg::quantum::Dispatcher& DispatcherFixture::getDispatcher()
{
    return *_dispatcher;
}
