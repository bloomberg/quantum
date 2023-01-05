/*
** Copyright 2018 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_QUANTUM_FIXTURE_H
#define BLOOMBERG_QUANTUM_FIXTURE_H

#include <gtest/gtest.h>
#include <quantum/quantum.h>
#include <unordered_map>
#include <memory>
#include <functional>

namespace quantum = Bloomberg::quantum;

using ms = std::chrono::milliseconds;

/// @brief TestTaskStateHandler class
class TestTaskStateHandler
{
public:
    TestTaskStateHandler();

    void operator()(size_t taskId, int queueId, quantum::TaskType type, quantum::TaskState state);

private:
    class TestTaskStateHandlerImpl;
    std::shared_ptr<TestTaskStateHandlerImpl> _impl;
};

quantum::TaskStateConfiguration getDefaultTaskStateConfiguration();

/// @brief TestConfiguration struct
struct TestConfiguration
{
    TestConfiguration(bool loadBalance,
                      bool coroutineSharingForAny,
                      const quantum::TaskStateConfiguration& TaskStateConfiguration = getDefaultTaskStateConfiguration());

    bool operator == (const TestConfiguration& that) const
    {
        return _loadBalance == that._loadBalance &&
            _coroutineSharingForAny == that._coroutineSharingForAny;
    }

    bool _loadBalance;
    bool _coroutineSharingForAny;
    quantum::TaskStateConfiguration _TaskStateConfiguration;
};

namespace std {
    template<> struct hash<TestConfiguration>
    {
        size_t operator()(const TestConfiguration & x) const
        {
            return std::hash<bool>()(x._loadBalance) +
                std::hash<bool>()(x._coroutineSharingForAny);
        }
    };
}

/// @brief Singleton class
class DispatcherSingleton
{
public:
    static std::shared_ptr<quantum::Dispatcher> createInstance(const TestConfiguration& taskConfig);

    static quantum::Dispatcher& instance(const TestConfiguration& config);
    static void deleteInstances();

    static constexpr int numCoro{4};

    static constexpr int numThreads{5};
private:
    using DispatcherMap = std::unordered_map<TestConfiguration, std::shared_ptr<quantum::Dispatcher>>;
    static DispatcherMap _dispatchers;
};

/// @brief Fixture used for certain tests
class DispatcherFixture : public ::testing::TestWithParam<TestConfiguration>
{
public:
    DispatcherFixture();

    /// @brief Create a dispatcher object with equal number of coroutine and IO threads
    void SetUp();
    void TearDown();

    quantum::Dispatcher& getDispatcher();

protected:
    quantum::Dispatcher*  _dispatcher;
};


#endif //BLOOMBERG_QUANTUM_FIXTURE_H
