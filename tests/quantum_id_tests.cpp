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
#include <vector>
#include <unordered_set>

using namespace Bloomberg;

struct MockTaskId : public quantum::TaskId
{
    struct ThisThreadTag{};
    MockTaskId() : quantum::TaskId() {}
    explicit MockTaskId(ThisThreadTag) : quantum::TaskId(quantum::TaskId::ThisThreadTag{}) {}
    explicit MockTaskId(quantum::CoroContextTag t) : quantum::TaskId(t){}
    explicit MockTaskId(quantum::ThreadContextTag t) : quantum::TaskId(t){}
    void assign() { assignCurrentThread(); }
};

//==============================================================================
//                             TEST CASES
//==============================================================================
TEST(TaskId, DefaultInitialization)
{
    MockTaskId idMain; //empty
    MockTaskId idMain2(MockTaskId::ThisThreadTag{}); //has main's thread id
    MockTaskId idCoro(quantum::CoroContextTag{});
    MockTaskId idCoroCopy = idCoro;
    idCoro.assign(); //set current thread id
    MockTaskId idCoro2(quantum::CoroContextTag{});
    MockTaskId idIo(quantum::ThreadContextTag{});
    idIo.assign(); //set current thread id
    MockTaskId idIo2(quantum::ThreadContextTag{});

    EXPECT_NE(idMain, idMain2);
    EXPECT_EQ(idMain2, quantum::local::taskId());
    EXPECT_NE(idCoro, idCoro2);
    EXPECT_EQ(idCoro, idCoroCopy); //do not compare thread ids, only coroutine ids
    EXPECT_NE(idIo, idIo2);
    EXPECT_NE(idCoro, idIo);
    EXPECT_NE(idCoro, idMain);
    EXPECT_NE(idIo, idMain);
    
    //check if it's a coroutine
    EXPECT_TRUE(idCoro.isCoroutine());
    EXPECT_FALSE(idMain.isCoroutine());
    EXPECT_FALSE(idIo.isCoroutine());
    
    //check the thread ids
    EXPECT_NE(idMain.threadId(), idCoro.threadId());
    EXPECT_EQ(idMain2.threadId(), idCoro.threadId());
    EXPECT_EQ(idCoro.threadId(), idIo.threadId());
    EXPECT_EQ(std::this_thread::get_id(), idCoro.threadId());
    EXPECT_EQ(std::thread::id(), idCoro2.threadId());
    EXPECT_EQ(0, idMain.id());
    EXPECT_NE(idCoro.id(), idCoro2.id());
    EXPECT_NE(idIo.id(), idIo2.id());
}

TEST(TaskId, Uniqueness)
{
    std::vector<MockTaskId> coro, io;
    MockTaskId firstCoro(quantum::CoroContextTag{});
    for (int i = 0; i < 10; ++i) {
        coro.emplace_back(MockTaskId(quantum::CoroContextTag{}));
    }
    //Check if all id's are different and are in decreasing order
    for (const auto& id : coro) {
        EXPECT_EQ(firstCoro.id()-1, id.id());
        firstCoro = id;
    }
    
    //IO ids are in increasing order
    MockTaskId firstIo(quantum::ThreadContextTag{});
    for (int i = 0; i < 10; ++i) {
        io.emplace_back(MockTaskId(quantum::ThreadContextTag{}));
    }
    //Check if all id's are different and are in decreasing order
    for (const auto& id : io) {
        EXPECT_EQ(firstIo.id()+1, id.id());
        firstIo = id;
    }
}

TEST(TaskId, LocalContext)
{
    quantum::Configuration config;
    config.setNumCoroutineThreads(2);
    config.setNumIoThreads(2);
    quantum::Dispatcher dispatcher(config);
    std::vector<quantum::TaskId> coroIds, ioIds;
    
    auto coroFunc = [](quantum::VoidCoroContextPtr ctx) mutable->quantum::TaskId  {
        return ctx->taskId();
    };
    auto ioFunc = []() mutable->quantum::TaskId {
        return quantum::local::taskId();
    };
    //Collect task ids
    coroIds.emplace_back(dispatcher.post(0,false,coroFunc)->get());
    coroIds.emplace_back(dispatcher.post(0,false,coroFunc)->get());
    coroIds.emplace_back(dispatcher.post(1,false,coroFunc)->get());
    coroIds.emplace_back(dispatcher.post(1,false,coroFunc)->get());
    ioIds.emplace_back(dispatcher.postAsyncIo(0,false,ioFunc)->get());
    ioIds.emplace_back(dispatcher.postAsyncIo(0,false,ioFunc)->get());
    ioIds.emplace_back(dispatcher.postAsyncIo(1,false,ioFunc)->get());
    ioIds.emplace_back(dispatcher.postAsyncIo(1,false,ioFunc)->get());
    
    //Compare
    EXPECT_TRUE(coroIds[0].isCoroutine());
    for (size_t i = 1; i < coroIds.size(); ++i) {
        EXPECT_TRUE(coroIds[i].isCoroutine());
        EXPECT_EQ(coroIds[i-1].id()-1, coroIds[i].id());
    }
    //Make sure the executing thread id is properly set
    EXPECT_EQ(coroIds[0].threadId(), coroIds[1].threadId());
    EXPECT_EQ(coroIds[2].threadId(), coroIds[3].threadId());
    EXPECT_NE(coroIds[0].threadId(), coroIds[2].threadId());
    EXPECT_NE(quantum::TaskId().threadId(), coroIds[0].threadId());
    
    EXPECT_FALSE(ioIds[0].isCoroutine());
    for (size_t i = 1; i < ioIds.size(); ++i) {
        EXPECT_FALSE(ioIds[i].isCoroutine());
        EXPECT_EQ(ioIds[i-1].id()+1, ioIds[i].id());
    }
    //Make sure the executing thread id is properly set
    EXPECT_EQ(ioIds[0].threadId(), ioIds[1].threadId());
    EXPECT_EQ(ioIds[2].threadId(), ioIds[3].threadId());
    EXPECT_NE(ioIds[0].threadId(), ioIds[2].threadId());
    EXPECT_NE(quantum::TaskId().threadId(), ioIds[0].threadId());
    
    //Check hashing
    std::unordered_set<quantum::TaskId> idSet;
    idSet.insert(coroIds.begin(), coroIds.end());
    idSet.insert(ioIds.begin(), ioIds.end());
    EXPECT_EQ(coroIds.size() + ioIds.size(), idSet.size());
}

