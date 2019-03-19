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

namespace quantum = Bloomberg::quantum;

/// @brief Singleton class
class DispatcherSingleton
{
public:
    static quantum::Dispatcher& createInstance(bool loadBalance)
    {
        if (_dispatcher == nullptr)
        {
            quantum::Configuration config;
            config.setNumCoroutineThreads(numCoro);
            config.setNumIoThreads(numThreads);
            config.setLoadBalanceSharedIoQueues(loadBalance);
            config.setLoadBalancePollIntervalMs(std::chrono::milliseconds(10));
            _dispatcher = new quantum::Dispatcher(config);
        }
        return *_dispatcher;
    }
    
    static quantum::Dispatcher& instance()
    {
        if (_dispatcher == nullptr)
        {
            createInstance(false);
        }
        return *_dispatcher;
    }
    
    static void deleteInstance()
    {
        delete _dispatcher;
        _dispatcher = nullptr;
    }
    
    static constexpr int numCoro{4};
    
    static constexpr int numThreads{5};
private:
    static quantum::Dispatcher*  _dispatcher;
};

/// @brief Fixture used for certain tests
class DispatcherFixture : public ::testing::Test
{
public:
    DispatcherFixture()
    {
        quantum::StackTraits::defaultSize() = 1 << 14; //16k stack for testing
    }
    /// @brief Create a dispatcher object with equal number of coroutine and IO threads
    void SetUp()
    {
        _dispatcher = &DispatcherSingleton::instance();
    }
    
    void TearDown()
    {
        DispatcherSingleton::deleteInstance();
    }
protected:
    quantum::Dispatcher*  _dispatcher;
};


#endif //BLOOMBERG_QUANTUM_FIXTURE_H
