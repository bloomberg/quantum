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
#include <gtest/gtest.h>
#include <quantum/quantum_auxiliary.h>
#include <quantum/quantum_task_state_handler.h>
#include <stdexcept>
#include <functional>


//==============================================================================
//                             TEST CASES
//==============================================================================

TEST(AuxiliaryTest, IsIntersection)
{
    // Check with different integral types
    EXPECT_TRUE(Bloomberg::quantum::isIntersection(long(1), long(1)));
    EXPECT_FALSE(Bloomberg::quantum::isIntersection(short(1), short(0)));
    EXPECT_TRUE(Bloomberg::quantum::isIntersection(int(1), int(3)));

    for (auto state : {
            Bloomberg::quantum::TaskState::Started,
            Bloomberg::quantum::TaskState::Suspended,
            Bloomberg::quantum::TaskState::Resumed,
            Bloomberg::quantum::TaskState::Stopped,
            Bloomberg::quantum::TaskState::All })
    {
        EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::None, state));
        EXPECT_TRUE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::All, state));
        EXPECT_TRUE(Bloomberg::quantum::isIntersection(state, state));
    }

    EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::Initialized,
                                                    Bloomberg::quantum::TaskState::All));

    EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::Initialized,
                                                    Bloomberg::quantum::TaskState::Started));

    EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::Started,
                                                    Bloomberg::quantum::TaskState::Suspended));

    EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::Suspended,
                                                    Bloomberg::quantum::TaskState::Resumed));

    EXPECT_FALSE(Bloomberg::quantum::isIntersection(Bloomberg::quantum::TaskState::Resumed,
                                                    Bloomberg::quantum::TaskState::Stopped));

}

TEST(AuxiliaryTest, MakeExceptionSafe)
{
    std::function<void()> func1 = {};
    func1 = Bloomberg::quantum::makeExceptionSafe(func1);
    EXPECT_FALSE(func1);

    func1 = []()
    {
        throw std::runtime_error("Error");
    };

    func1 = Bloomberg::quantum::makeExceptionSafe(func1);
    EXPECT_TRUE(func1);
    EXPECT_NO_THROW(func1());


    std::function<void(int, char, std::unique_ptr<int>)> func2 =
    [](int, char, std::unique_ptr<int>)
    {
        throw std::runtime_error("Error");
    };
    func2 = Bloomberg::quantum::makeExceptionSafe(func2);
    EXPECT_TRUE(func2);
    EXPECT_NO_THROW(func2(1, 'a', std::make_unique<int>(-1)));
}
