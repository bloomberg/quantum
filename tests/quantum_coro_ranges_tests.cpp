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
#include <quantum/quantum.h>

using namespace Bloomberg;

//==============================================================================
//                             TEST CASES
//==============================================================================
TEST(DispatcherCoroRangeTest, DefaultAnyCoroQueueIdRange1)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    quantum::Dispatcher dispatcher(configuration);

    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(9, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, ValidAnyCoroQueueIdRange1)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(2,3));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(2, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(3, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, ValidAnyCoroQueueIdRange2)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(1,8));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(1, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(8, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, ValidAnyCoroQueueIdRange3)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(1,1));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(1, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(1, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, EmptyAnyCoroQueueIdRange1)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(1,0));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(9, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, EmptyAnyCoroQueueIdRange2)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(10,9));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(9, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, EmptyAnyCoroQueueIdRange3)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(0);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(10,9));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, InvalidAnyCoroQueueIdRange1)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(10);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(20,30));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(9, dispatcher.getCoroQueueIdRangeForAny().second);
}

TEST(DispatcherCoroRangeTest, InvalidAnyCoroQueueIdRange2)
{
    quantum::Configuration configuration;
    configuration.setNumCoroutineThreads(0);
    configuration.setCoroQueueIdRangeForAny(std::make_pair(20,30));
    quantum::Dispatcher dispatcher(configuration);
    
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().first);
    EXPECT_EQ(0, dispatcher.getCoroQueueIdRangeForAny().second);
}
