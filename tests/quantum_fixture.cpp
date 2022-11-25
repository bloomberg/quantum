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

class TestCoroutineStateHandler::TestCoroutineStateHandlerImpl
{
public:
    void operator()(Bloomberg::quantum::CoroutineState state)
    {
        switch (state)
        {
            case Bloomberg::quantum::CoroutineState::Constructed:
            {
                size_t*& constructedId = Bloomberg::quantum::local::variable<size_t>("ConstructedId");
                EXPECT_FALSE(constructedId);
                constructedId = new size_t(Bloomberg::quantum::local::taskId().id());
                break;
            }
            case Bloomberg::quantum::CoroutineState::Resumed:
            {
                size_t*& resumedId = Bloomberg::quantum::local::variable<size_t>("ResumedId");
                EXPECT_FALSE(resumedId);
                resumedId = new size_t(Bloomberg::quantum::local::taskId().id());
                break;
            }
            case Bloomberg::quantum::CoroutineState::Suspended:
            {
                // Check constructed id exists and equals to the task id
                size_t*& constructedId = Bloomberg::quantum::local::variable<size_t>("ConstructedId");
                EXPECT_TRUE(constructedId);
                EXPECT_EQ(*constructedId, Bloomberg::quantum::local::taskId().id());

                // If resumed id exists check that it equals to the task id and removes it
                size_t*& resumedId = Bloomberg::quantum::local::variable<size_t>("ResumedId");
                if (resumedId)
                {
                    EXPECT_EQ(*resumedId, Bloomberg::quantum::local::taskId().id());
                    delete resumedId;
                    resumedId = nullptr;
                }
                break;
            }
            case Bloomberg::quantum::CoroutineState::Destructed:
            {
                // Check that constructed id exists, equals to the task id and rmeove it
                size_t*& constructedId = Bloomberg::quantum::local::variable<size_t>("ConstructedId");
                EXPECT_TRUE(constructedId);
                EXPECT_EQ(*constructedId, Bloomberg::quantum::local::taskId().id());
                delete constructedId;
                constructedId = nullptr;
                break;
            }
        }
    }
};

TestCoroutineStateHandler::TestCoroutineStateHandler():
    _impl(std::make_shared<TestCoroutineStateHandlerImpl>())
{}

void TestCoroutineStateHandler::operator()(Bloomberg::quantum::CoroutineState state)
{
    _impl->operator()(state);
}

TestConfiguration::TestConfiguration(bool loadBalance,
                                     bool coroutineSharingForAny,
                                     const Bloomberg::quantum::CoroutineStateHandler& coroutineStateHandler):
    _loadBalance(loadBalance),
    _coroutineSharingForAny(coroutineSharingForAny),
    _coroutineStateHandler(coroutineStateHandler)
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
    config.setCoroutineStateHandler(taskConfig._coroutineStateHandler);
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
