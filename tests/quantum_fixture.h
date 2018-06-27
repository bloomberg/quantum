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
#ifndef QUANTUM_FIXTURE_H
#define QUANTUM_FIXTURE_H

#include <gtest/gtest.h>
#include <quantum/quantum_dispatcher.h>

namespace quantum = Bloomberg::quantum;

/// @brief Singleton class
class Dispatcher
{
public:
    static void CreateInstance(int numCoro, int numIo, bool pin)
    {
        if (_dispatcher == nullptr)
        {
            _dispatcher = new quantum::TaskDispatcher(numCoro, numIo, pin);
        }
    }
    
    static quantum::TaskDispatcher& Instance()
    {
        if (_dispatcher == nullptr)
        {
            CreateInstance(5, 5, false);
        }
        return *_dispatcher;
    }
    
    static void DeleteInstance()
    {
        delete _dispatcher;
        _dispatcher = nullptr;
    }
    
private:
    static quantum::TaskDispatcher*  _dispatcher;
};

/// @brief Fixture used for certain tests
class DispatcherFixture : public ::testing::Test
{
public:
    /// @brief Create a dispatcher object with equal number of coroutine and IO threads
    void SetUp()
    {
        Dispatcher::CreateInstance(5, 5, false);
        _dispatcher = &Dispatcher::Instance();
    }
    
    void TearDown()
    {
        Dispatcher::DeleteInstance();
    }
protected:
    quantum::TaskDispatcher*  _dispatcher;
};


#endif //QUANTUM_FIXTURE_H
