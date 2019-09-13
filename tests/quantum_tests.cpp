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
#include <quantum_fixture.h>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <list>
#include <memory>
#include <functional>

using namespace quantum;
using ms = std::chrono::milliseconds;
using us = std::chrono::microseconds;
constexpr int DispatcherSingleton::numCoro;
constexpr int DispatcherSingleton::numThreads;
DispatcherSingleton::DispatcherMap DispatcherSingleton::_dispatchers;

//==============================================================================
// TEST FIXTURES
//==============================================================================

struct CoreTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(CoreTest_Default,
                        CoreTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));

struct ParamtersTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(ParamtersTest_Default,
                        ParamtersTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct ExecutionTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(ExecutionTest_Default,
                        ExecutionTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct PromiseTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(PromiseTest_Default,
                        PromiseTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct MutexTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(MutexTest_Default,
                        MutexTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct StressTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(StressTest_Default,
                        StressTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct StressTestBalanced: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(StressTestBalanced_Default,
                        StressTestBalanced,
                        ::testing::Values(TestConfiguration(true, false),
                                          TestConfiguration(true, true)));


struct ForEachTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(ForEachTest_Default,
                        ForEachTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct MapReduce: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(MapReduce_Default,
                        MapReduce,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));


struct FutureJoinerTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(FutureJoinerTest_Default,
                        FutureJoinerTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));

struct CoroLocalStorageTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(CoroLocalStorageTest_Default,
                        CoroLocalStorageTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));

struct CleanupTest: public DispatcherFixture
{};

INSTANTIATE_TEST_CASE_P(CleanupTest_Default,
                        CleanupTest,
                        ::testing::Values(TestConfiguration(false, false),
                                          TestConfiguration(false, true)));

//==============================================================================
//                           TEST HELPERS
//==============================================================================
int DummyCoro(CoroContext<int>::Ptr ctx)
{
    UNUSED(ctx);
    return 0;
}

std::string DummyCoro2(VoidContextPtr)
{
    return "test";
}

int DummyIoTask(ThreadPromise<int>::Ptr promise)
{
    UNUSED(promise);
    std::this_thread::sleep_for(ms(10));
    return 0;
}

struct Dummy
{
    int MemberCoro(CoroContext<std::string>::Ptr ctx)
    {
        return ctx->set("test");
    }
};

#ifdef BOOST_USE_VALGRIND
    int fibInput = 10;
#else
    int fibInput = 23;
#endif

std::map<int, int> fibValues{{10, 55}, {20, 6765}, {21, 10946}, {22, 17711},
                             {23, 28657}, {24, 46368}, {25, 75025}, {30, 832040}};

int sequential_fib(CoroContext<size_t>::Ptr ctx, size_t fib)
{
    size_t a = 0, b = 1, c, i;
    for (i = 2; i <= fib; i++)
    {
        c = a + b;
        a = b;
        b = c;
    }
    return ctx->set(c);
}

int recursive_fib(CoroContext<size_t>::Ptr ctx, size_t fib)
{
    ctx->sleep(us(100));

    if (fib <= 2)
    {
        return ctx->set(1);
    }
    else
    {
        //Post both branches of the Fibonacci series before blocking on get().
        auto ctx1 = ctx->post(recursive_fib, fib - 2);
        auto ctx2 = ctx->post(recursive_fib, fib - 1);
        return ctx->set(ctx1->get(ctx) + ctx2->get(ctx));
    }
}

void run_coro(Traits::Coroutine& coro, std::mutex& m, int& end, int start)
{
    int var = start;
    while(end<20) {
        m.lock();
        if (coro)
            coro(var);
        m.unlock();
        sleep(1);
    }
}

void enqueue_sleep_tasks(quantum::Dispatcher& dispatcher,
                         const std::vector<std::pair<size_t, ms>> & sleepTimes)
{
    for(auto item : sleepTimes)
    {
        for(size_t i = 0; i < item.first; ++i)
        {
            dispatcher.post((int)IQueue::QueueId::Any,
                            false,
                            [item](CoroContext<int>::Ptr)->int {
                                std::this_thread::sleep_for(item.second); 
                                return 0;
                            });
        }
    }
}

//==============================================================================
//                             TEST CASES
//==============================================================================
/*
TEST_P(CoreTest, ResumeFromTwoThreads)
{
    int end{0};
    std::mutex m;
    Traits::Coroutine coro([&end](Traits::Yield& yield){
        while (1) {
            std::cout << "Running on thread: " << std::this_thread::get_id() << " value: " << yield.get()++ << " iteration: " << end++ << std::endl;
            yield();
        }
    });
    std::thread t1(run_coro, std::ref(coro), std::ref(m), std::ref(end), 0);
    std::thread t2(run_coro, std::ref(coro), std::ref(m), std::ref(end), 100);
    t1.join();
    t2.join();
    std::cout << "Done" << std::endl;
}
*/

TEST_P(CoreTest, Constructor)
{
    //Check if we have 0 coroutines and IO tasks running
    EXPECT_EQ(0, (int)getDispatcher().size(IQueue::QueueType::Coro));
    EXPECT_EQ(0, (int)getDispatcher().size(IQueue::QueueType::IO));
    EXPECT_EQ(0, (int)getDispatcher().size());
}

TEST_P(CoreTest, CheckReturnValue)
{
    IThreadContext<std::string>::Ptr tctx = getDispatcher().post(DummyCoro2);
    std::string s = tctx->get();
    EXPECT_STREQ("test", s.c_str());
}

TEST_P(CoreTest, CheckNumThreads)
{
    IThreadContext<int>::Ptr tctx = getDispatcher().post([](CoroContext<int>::Ptr ctx)->int{
        EXPECT_EQ(DispatcherSingleton::numCoro, ctx->getNumCoroutineThreads());
        EXPECT_EQ(DispatcherSingleton::numThreads, ctx->getNumIoThreads());
        return 0;
    });
    EXPECT_EQ(DispatcherSingleton::numCoro, tctx->getNumCoroutineThreads());
    EXPECT_EQ(DispatcherSingleton::numThreads, tctx->getNumIoThreads());
}

TEST_P(CoreTest, CheckCoroutineQueuing)
{
    //Post various IO tasks and coroutines and make sure they executed on the proper queues
    for (int i = 0; i < 3; ++i)
    {
        getDispatcher().post(0, false, DummyCoro);
    }
    getDispatcher().post(1, true, DummyCoro);
    getDispatcher().post(2, false, DummyCoro);
    getDispatcher().drain();
    
    //Posted
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 1).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 2).postedCount());
    EXPECT_EQ((size_t)5, getDispatcher().stats(IQueue::QueueType::Coro).postedCount()); //total
    
    //Completed
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro, 0).completedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 1).completedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 2).completedCount());
    EXPECT_EQ((size_t)5, getDispatcher().stats(IQueue::QueueType::Coro).completedCount()); //total
    
    //Errors
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 0).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 1).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 2).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro).errorCount()); //total
    
    //High Priority
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro).highPriorityCount()); //total
    
    //Check if all tasks have stopped
    EXPECT_EQ((size_t)0, getDispatcher().size(IQueue::QueueType::Coro));
}

TEST_P(CoreTest, CheckIoQueuing)
{
    //IO (10 tasks)
    for (int i = 0; i < 10; ++i)
    {
        getDispatcher().postAsyncIo(DummyIoTask); //shared queue
    }
    getDispatcher().postAsyncIo(1, true, DummyIoTask);
    getDispatcher().postAsyncIo(2, false, DummyIoTask);
    
    getDispatcher().drain();
    
    //Posted
    EXPECT_EQ((size_t)10, getDispatcher().stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 1).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 2).postedCount());
    EXPECT_EQ((size_t)12, getDispatcher().stats(IQueue::QueueType::IO).postedCount()); //total
    
    //Completed
    EXPECT_LE((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).completedCount());
    EXPECT_EQ((size_t)10, getDispatcher().stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).completedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO, 0).sharedQueueCompletedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO, 1).sharedQueueCompletedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO, 2).sharedQueueCompletedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO, 3).sharedQueueCompletedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO, 4).sharedQueueCompletedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 1).completedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 2).completedCount());
    EXPECT_EQ((size_t)12, getDispatcher().stats(IQueue::QueueType::IO).completedCount() +
                          getDispatcher().stats(IQueue::QueueType::IO).sharedQueueCompletedCount()); //total
    
    //Errors
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, 1).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, 2).errorCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO).errorCount()); //total
    
    //High Priority
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO).highPriorityCount()); //total
    
    //Check if all tasks have stopped
    EXPECT_EQ((size_t)0, getDispatcher().size(IQueue::QueueType::IO));
}

TEST_P(CoreTest, CheckQueuingFromSameCoroutine)
{
    getDispatcher().post(0, false, [](CoroContext<int>::Ptr ctx)->int {
        //Test with VoidContext
        Util::makeVoidContext<int>(ctx)->postFirst(1, true, DummyCoro)->then(DummyCoro)->finally(DummyCoro)->end();
        return 0;
    });
    getDispatcher().drain();
    
    //Posted
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro, 1).postedCount());
    EXPECT_EQ((size_t)4, getDispatcher().stats(IQueue::QueueType::Coro).postedCount()); //total
    
    //High priority
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro, 1).highPriorityCount());
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro).highPriorityCount()); //total
}

TEST_P(CoreTest, CheckIoQueuingFromACoroutine)
{
    getDispatcher().post(0, false, [](CoroContext<int>::Ptr ctx)->int {
        ctx->postAsyncIo(1, true, DummyIoTask);
        ctx->postAsyncIo(2, false, DummyIoTask);
        ctx->postAsyncIo(3, true, DummyIoTask);
        return 0;
    });
    getDispatcher().drain();
    
    //Posted
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 1).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 2).postedCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 3).postedCount());
    
    //High priority
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, getDispatcher().stats(IQueue::QueueType::IO, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::IO, 3).highPriorityCount());
    
    //Completed count
    EXPECT_EQ((size_t)1, getDispatcher().stats(IQueue::QueueType::Coro).completedCount());
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::IO).completedCount());
    
    //Remaining
    EXPECT_EQ((size_t)0, getDispatcher().size());
}

TEST_P(CoreTest, CheckCoroutineErrors)
{
    std::string s("original"); //string must remain unchanged
    
    getDispatcher().post([](CoroContext<int>::Ptr ctx, std::string& str)->int {
        ctx->yield();
        return 1; //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    getDispatcher().post([](CoroContext<int>::Ptr ctx, std::string& str)->int {
        ctx->yield(); //test yield via the VoidContext
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    getDispatcher().post([](VoidContextPtr ctx, std::string& str)->std::string {
        ctx->yield(); //test yield via the VoidContext
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return str;
    }, s);
    
    getDispatcher().postAsyncIo([](ThreadPromise<int>::Ptr, std::string& str)->int {
        std::this_thread::sleep_for(ms(10));
        return 1; //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    getDispatcher().postAsyncIo([](ThreadPromise<int>::Ptr, std::string& str)->int {
        std::this_thread::sleep_for(ms(10));
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    getDispatcher().postAsyncIo([](std::string& str)->std::string {
        std::this_thread::sleep_for(ms(10));
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return str;
    }, s);
    
    getDispatcher().drain();
    
    //Error count
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::Coro).errorCount());
    EXPECT_EQ((size_t)3, getDispatcher().stats(IQueue::QueueType::IO).errorCount() +
                         getDispatcher().stats(IQueue::QueueType::IO).sharedQueueErrorCount());
    EXPECT_STREQ("original", s.c_str());
    
    //Remaining
    EXPECT_EQ((size_t)0, getDispatcher().size());
}

struct NonCopyable {
    NonCopyable() = delete;
    NonCopyable(const std::string& str) : _str(str){}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = default;
    std::string _str;
};

TEST_P(ParamtersTest, CheckParameterPassingInCoroutines)
{
    //Test pass by value, reference and address.
    int a = 5;
    std::string str = "original";
    std::string str2 = "original2";
    NonCopyable nc("move");
    double dbl = 4.321;
    
    auto func = [&](CoroContext<int>::Ptr ctx, int byVal, std::string& byRef, std::string&& byRvalue, NonCopyable&& byRvalueNoCopy, double* byAddress)->int {
        //modify all passed-in values
        EXPECT_EQ(5, byVal);
        byVal = 6;
        EXPECT_NE(a, byVal);
        byRef = "changed";
        EXPECT_EQ(byRef.data(), str.data());
        *byAddress = 6.543;
        EXPECT_NE(str2.c_str(), byRvalue.c_str());
        std::string tempStr(std::move(byRvalue));
        EXPECT_STREQ("original2", tempStr.c_str());
        NonCopyable tempStr2 = std::move(byRvalueNoCopy);
        EXPECT_STREQ("move", tempStr2._str.c_str());
        return ctx->set(0);
    };
    
    getDispatcher().post(func, int(a), str, std::move(str2), std::move(nc), &dbl)->get();
    
    //Validate values
    EXPECT_EQ(5, a);
    EXPECT_STREQ("changed", str.c_str());
    EXPECT_TRUE(str2.empty());
    EXPECT_EQ(0, (int)nc._str.size());
    EXPECT_DOUBLE_EQ(6.543, dbl);
}

TEST_P(ExecutionTest, DrainAllTasks)
{
    //Turn the drain on and make sure we cannot queue any tasks
    Dispatcher& dispatcher = getDispatcher();
    
    //Post a bunch of coroutines to run and wait for completion.
    for (int i = 0; i < 100; ++i)
    {
        dispatcher.post(DummyCoro);
    }
    
    dispatcher.drain();
    EXPECT_EQ((size_t)0, dispatcher.size(IQueue::QueueType::Coro));
    EXPECT_EQ((size_t)0, dispatcher.size());
}

TEST_P(ExecutionTest, YieldingBetweenTwoCoroutines)
{
    //Basic test which verifies cooperation between two coroutines.
    //This also outlines lock-free coding.
    
    auto func = [](CoroContext<int>::Ptr ctx, std::set<int>& s)->int {
        s.insert(1);
        ctx->yield();
        s.insert(3);
        ctx->yield();
        s.insert(5);
        return 0;
    };
    
    auto func2 = [](CoroContext<int>::Ptr ctx, std::set<int>& s)->int {
        s.insert(2);
        ctx->yield();
        s.insert(4);
        ctx->yield();
        s.insert(6);
        return 0;
    };
    
    std::set<int> testSet; //this will contain [1,6]
    
    Dispatcher& dispatcher = getDispatcher();
    
    dispatcher.post(3, false, func, testSet);
    dispatcher.post(3, false, func2, testSet);
    dispatcher.drain();
    
    std::set<int> validation{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(validation, testSet);
}

TEST_P(ExecutionTest, ChainCoroutinesFromDispatcher)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1;
    std::vector<int> v;
    std::vector<int> validation{1,2,3,4};
    
    auto func = [](CoroContext<int>::Ptr, std::vector<int>& v, int& i)->int
    {
        v.push_back(i++);
        return 0;
    };
    dispatcher.postFirst(func, v, i)->then(func, v, i)->then(func, v, i)->then(func, v, i)->end();
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST_P(ExecutionTest, ChainCoroutinesFromCoroutineContext)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1, err = 10, final = 20;
    std::vector<int> v;
    std::vector<int> validation{1,2,3,4,20};
    
    auto func2 = [](CoroContext<int>::Ptr, std::vector<int>& v, int& i)->int
    {
        v.push_back(i++);
        return 0;
    };
     
    auto func = [&](CoroContext<int>::Ptr ctx, std::vector<int>& v, int& i)->int
    {
        ctx->postFirst(func2, v, i)->
             then(func2, v, i)->
             then(func2, v, i)->
             then(func2, v, i)->
             onError(func2, v, err)->
             finally(func2, v, final)->
             end(); //OnError *should not* run
        return 0;
    };
    
    dispatcher.post(func, v, i);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST_P(ExecutionTest, ChainCoroutinesFromCoroutineContext2)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1, err = 10, final = 20;
    std::vector<int> v;
    std::vector<int> validation{1,2,3,4,20};
    
    auto func2 = [](VoidContextPtr, std::vector<int>& v, int& i)->std::vector<int>
    {
        v.push_back(i++);
        return v;
    };
    
    auto func = [&](VoidContextPtr ctx, std::vector<int>& v, int& i)->std::vector<int>
    {
        return ctx->postFirst(func2, v, i)->
                    then(func2, v, i)->
                    then(func2, v, i)->
                    then(func2, v, i)->
                    onError(func2, v, err)->
                    finally(func2, v, final)->
                    end()->get(ctx); //OnError *should not* run
    };
    
    dispatcher.post(func, v, i);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST_P(ExecutionTest, OnErrorTaskRuns)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1, err = 10, final = 20;
    std::vector<int> v;
    std::vector<int> validation{1,2,10,20}; //includes error
    
    auto func2 = [](CoroContext<int>::Ptr, std::vector<int>& v, int& i)->int
    {
        if (i == 3) return -1; //cause an error
        v.push_back(i++);
        return 0;
    };
    
    auto func = [&](CoroContext<int>::Ptr ctx, std::vector<int>& v, int& i)->int
    {
        ctx->postFirst(func2, v, i)->then(func2, v, i)->then(func2, v, i)->
             then(func2, v, i)->onError(func2, v, err)->finally(func2, v, final)->end(); //OnError *should* run
        return 0;
    };
    
    dispatcher.post(func, v, i);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST_P(ExecutionTest, FinallyAlwaysRuns)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1, final = 20;
    std::vector<int> v;
    std::vector<int> validation{1,2,20}; //includes error
    
    auto func2 = [](CoroContext<int>::Ptr, std::vector<int>& v, int& i)->int
    {
        if (i == 3) return -1; //cause an error
        v.push_back(i++);
        return 0;
    };
    
    auto func = [&](CoroContext<int>::Ptr ctx, std::vector<int>& v, int& i)->int
    {
        ctx->postFirst(func2, v, i)->then(func2, v, i)->then(func2, v, i)->
             then(func2, v, i)->finally(func2, v, final)->end(); //OnError *should* run
        return 0;
    };
    
    dispatcher.post(func, v, i);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST_P(ExecutionTest, CoroutineSleep)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(100));
        return 0;
    });
    
    auto start = std::chrono::steady_clock::now();
    ctx->wait(); //block until value is available
    auto end = std::chrono::steady_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)100);
}

TEST_P(PromiseTest, GetFutureFromCoroutine)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        return ctx->set(55); //Set the promise
    });
    EXPECT_EQ(55, ctx->get()); //block until value is available
    EXPECT_THROW(ctx->get(), FutureAlreadyRetrievedException);
}

TEST_P(PromiseTest, GetFutureFromIoTask)
{
    Dispatcher& dispatcher = getDispatcher();
    ThreadContext<int>::Ptr ctx = dispatcher.post([](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<double>::Ptr fut = ctx->postAsyncIo([](ThreadPromise<double>::Ptr promise)->int{
            return promise->set(33.22);
        });
        return ctx->set((int)fut->get(ctx)); //forward the promise
    });
    EXPECT_EQ(33, ctx->get()); //block until value is available
}

TEST_P(PromiseTest, GetFutureFromIoTask2)
{
    Dispatcher& dispatcher = getDispatcher();
    ThreadContext<int>::Ptr ctx = dispatcher.post([](VoidContextPtr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<double>::Ptr fut = ctx->postAsyncIo([]()->double{
            return 33.22;
        });
        return (int)fut->get(ctx); //forward the promise
    });
    EXPECT_EQ(33, ctx->get()); //block until value is available
}

TEST_P(PromiseTest, GetGenericFutureFromIoTask)
{
    Dispatcher& dispatcher = getDispatcher();
    GenericFuture<int> genFuture = dispatcher.post([](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        GenericFuture<double> genFuture(ctx->postAsyncIo([](ThreadPromise<double>::Ptr promise)->int{
            return promise->set(33.22);
        }), ctx);
        return ctx->set((int)genFuture.get()); //forward the promise
    });
    EXPECT_EQ(33, genFuture.get()); //block until value is available
}

TEST_P(PromiseTest, GetFutureFromExternalSource)
{
    Dispatcher& dispatcher = getDispatcher();
    Promise<int> promise;
    ThreadContext<int>::Ptr ctx = dispatcher.post([&promise](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<int>::Ptr future = promise.getICoroFuture();
        return ctx->set(future->get(ctx)); //forward the promise
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    promise.set(33); //set the external promise
    EXPECT_EQ(33, ctx->get()); //block until value is available
}

TEST_P(PromiseTest, BufferedFuture)
{
    Dispatcher& dispatcher = getDispatcher();
    ThreadContext<Buffer<int>>::Ptr ctx = dispatcher.post([](CoroContext<Buffer<int>>::Ptr ctx)->int{
        for (int d = 0; d < 100; d++)
        {
            ctx->push(d);
        }
        return ctx->closeBuffer();
    });
    
    std::vector<int> v;
    while (1)
    {
        bool isBufferClosed = false;
        int value = ctx->pull(isBufferClosed);
        if (isBufferClosed) break;
        v.push_back(value);
    }
    
    //Validate
    size_t num = 0;
    for (auto&& val : v) {
        EXPECT_EQ(num++, (size_t)val);
    }
}

TEST_P(PromiseTest, BufferedFutureException)
{
    Dispatcher& dispatcher = getDispatcher();
    ThreadContext<Buffer<double>>::Ptr ctx = dispatcher.post([](CoroContext<Buffer<double>>::Ptr ctx)->int{
        for (int d = 0; d < 100; d++)
        {
            ctx->push(d);
        }
        // Don't close the buffer but throw instead
        try {
            throw 5;
        }
        catch (...)
        {
            return ctx->setException(std::current_exception());
        }
    });
    
    std::vector<double> v;
    bool wasCaught = false;
    while (1)
    {
        try {
            bool isBufferClosed = false;
            double value = ctx->pull(isBufferClosed);
            if (isBufferClosed) break;
            v.push_back(value);
        }
        catch (...) {
            wasCaught = true;
            break;
        }
    }
    
    //Validate
    EXPECT_TRUE(wasCaught);
    EXPECT_GE(100, (int)v.size());
}

TEST_P(PromiseTest, GetFutureReference)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        return ctx->set(55); //Set the promise
    });
    EXPECT_EQ(55, ctx->getRef()); //block until value is available
    EXPECT_NO_THROW(ctx->getRef());
    EXPECT_NO_THROW(ctx->get());
    EXPECT_THROW(ctx->get(), FutureAlreadyRetrievedException);
}

TEST_P(PromiseTest, GetIntermediateFutures)
{
    Dispatcher& dispatcher = getDispatcher();
    auto ctx = dispatcher.postFirst([](CoroContext<int>::Ptr ctx)->int {
        return ctx->set(55); //Set the promise
    })->then([](CoroContext<double>::Ptr ctx)->int {
        return ctx->set(22.33); //Set the promise
    })->then([](CoroContext<std::string>::Ptr ctx)->int {
        return ctx->set("future"); //Set the promise
    })->then([](CoroContext<std::list<int>>::Ptr ctx)->int {
        return ctx->set(std::list<int>{1,2,3}); //Set the promise
    })->end();
    
    std::list<int> validate{1,2,3};
    
    EXPECT_EQ(55, ctx->getAt<int>(0));
    EXPECT_DOUBLE_EQ(22.33, ctx->getAt<double>(1));
    EXPECT_THROW(ctx->getAt<double>(1), FutureAlreadyRetrievedException); //already retrieved
    EXPECT_STREQ("future", ctx->getAt<std::string>(2).c_str());
    EXPECT_EQ(validate, ctx->getRefAt<std::list<int>>(-1));
    EXPECT_EQ(validate, ctx->get()); //ok - can read value again
}

TEST_P(PromiseTest, GetIntermediateFutures2)
{
    Dispatcher& dispatcher = getDispatcher();
    auto ctx = dispatcher.postFirst([](VoidContextPtr)->int {
        return 55; //Set the promise
    })->then([](CoroContext<double>::Ptr ctx)->int { //mix with V1 API
        return ctx->set(22.33); //Set the promise
    })->then([](VoidContextPtr)->std::string {
        return "future"; //Set the promise
    })->then([](VoidContextPtr)->std::list<int> {
        return {1,2,3}; //Set the promise
    })->end();
    
    std::list<int> validate{1,2,3};
    
    EXPECT_EQ(55, ctx->getAt<int>(0));
    EXPECT_DOUBLE_EQ(22.33, ctx->getAt<double>(1));
    EXPECT_THROW(ctx->getAt<double>(1), FutureAlreadyRetrievedException); //already retrieved
    EXPECT_STREQ("future", ctx->getAt<std::string>(2).c_str());
    EXPECT_EQ(validate, ctx->getRefAt<std::list<int>>(-1));
    EXPECT_EQ(validate, ctx->get()); //ok - can read value again
}

TEST_P(PromiseTest, GetPreviousFutures)
{
    Dispatcher& dispatcher = getDispatcher();
    auto ctx = dispatcher.postFirst([](CoroContext<int>::Ptr ctx)->int {
        return ctx->set(55); //Set the promise
    })->then([](CoroContext<double>::Ptr ctx)->int {
        EXPECT_EQ(55, ctx->getPrev<int>());
        return ctx->set(22.33); //Set the promise
    })->then([](CoroContext<std::string>::Ptr ctx)->int {
        EXPECT_DOUBLE_EQ(22.33, ctx->getPrev<double>());
        return ctx->set("future"); //Set the promise
    })->then([](CoroContext<std::list<int>>::Ptr ctx)->int {
        EXPECT_STREQ("future", ctx->getPrevRef<std::string>().c_str());
        return ctx->set(std::list<int>{1,2,3}); //Set the promise
    })->end();
    
    std::list<int> validate{1,2,3};
    EXPECT_EQ(validate, ctx->get()); //ok - can read value again
    EXPECT_STREQ("future", ctx->getAt<std::string>(2).c_str());
}

TEST_P(PromiseTest, BrokenPromiseInAsyncIo)
{
    Dispatcher& dispatcher = getDispatcher();
    ThreadContext<int>::Ptr ctx = dispatcher.post([](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<double>::Ptr fut = ctx->postAsyncIo([](ThreadPromise<double>::Ptr)->int{
            //Do not set the promise so that we break it
            return 0;
        });
        EXPECT_THROW(fut->get(ctx), BrokenPromiseException); //broken promise
        return 0;
    });
}

TEST_P(PromiseTest, BreakPromiseByThrowingError)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr)->int{
        throw std::runtime_error("don't set the promise");
    });
    EXPECT_THROW(ctx->getRef(), std::runtime_error);
    EXPECT_THROW(ctx->get(), std::runtime_error);
}

TEST_P(PromiseTest, PromiseBrokenWhenOnError)
{
    Dispatcher& dispatcher = getDispatcher();
    int i = 1;
    
    auto func2 = [](CoroContext<int>::Ptr ctx, int& i)->int
    {
        UNUSED(ctx);
        if (i == 2) return -1; //cause an error
        return ctx->set(i++);
    };
    
    auto onErrorFunc = [](CoroContext<int>::Ptr ctx)->int
    {
        EXPECT_THROW(ctx->getPrev<int>(), BrokenPromiseException);
        return ctx->set(77);
    };
    
    auto func = [&](CoroContext<int>::Ptr ctx, int& i)->int
    {
        CoroContext<int>::Ptr chain = ctx->postFirst(func2, i)->then(func2, i)->then(func2, i)->
             then(func2, i)->onError(onErrorFunc)->end(); //OnError *should* run
        EXPECT_THROW(chain->getAt<int>(1, ctx), BrokenPromiseException);
        EXPECT_THROW(chain->getAt<int>(2, ctx), BrokenPromiseException);
        EXPECT_THROW(chain->getAt<int>(3, ctx), BrokenPromiseException);
        EXPECT_NO_THROW(chain->getRef(ctx));
        EXPECT_EQ(77, chain->get(ctx)); //OnError task
        return 0;
    };
    
    dispatcher.post(func, i);
    dispatcher.drain();
}

TEST_P(PromiseTest, SetExceptionInPromise)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        try {
            throw 5;
        }
        catch (...)
        {
            return ctx->setException(std::current_exception());
        }
    });
    EXPECT_THROW(ctx->get(), int);
}

TEST_P(PromiseTest, FutureTimeout)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(300));
        return 0;
    });
    
    auto start = std::chrono::steady_clock::now();
    std::future_status status = ctx->waitFor(ms(100)); //block until value is available or 100ms have expired
    auto end = std::chrono::steady_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_LT(elapsed, (size_t)300);
    EXPECT_EQ(status, std::future_status::timeout);
}

TEST_P(PromiseTest, FutureWithoutTimeout)
{
    Dispatcher& dispatcher = getDispatcher();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(100));
        return 0;
    });
    
    auto start = std::chrono::steady_clock::now();
    std::future_status status = ctx->waitFor(ms(300)); //block until value is available or 300ms have expired
    auto end = std::chrono::steady_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)100);
    EXPECT_LT(elapsed, (size_t)300);
    EXPECT_EQ(status, std::future_status::ready);
}

TEST_P(PromiseTest, WaitForAllFutures)
{
    Dispatcher& dispatcher = getDispatcher();
    auto func = [](CoroContext<int>::Ptr ctx)->int {
        ctx->sleep(ms(50));
        return 0;
    };
    
    auto ctx = dispatcher.postFirst(func)->then(func)->then(func)->then(func)->end();
    auto start = std::chrono::steady_clock::now();
    ctx->waitAll(); //block until value is available or 4x50ms has expired
    auto end = std::chrono::steady_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)200);
}

TEST_P(MutexTest, LockingAndUnlocking)
{
    Dispatcher& dispatcher = getDispatcher();
    std::vector<int> v;
    
    Mutex m;
    
    //lock the vector before posting the coroutines
    m.lock();
    v.push_back(5);
    
    //start a couple of coroutines waiting to access the vector
    dispatcher.post([](ICoroContext<int>::Ptr ctx, Mutex& mu, std::vector<int>& vec)->int{
        mu.lock(ctx);
        vec.push_back(6);
        mu.unlock();
        return 0;
    }, m, v);
    dispatcher.post([](ICoroContext<int>::Ptr ctx, Mutex& mu, std::vector<int>& vec)->int{
        mu.lock(ctx);
        vec.push_back(7);
        mu.unlock();
        return 0;
    }, m, v);
    
    std::this_thread::sleep_for(ms(200));
    m.unlock();
    
    dispatcher.drain(); //wait for completion
    
    ASSERT_EQ((size_t)3, v.size());
    EXPECT_EQ(5, v[0]);
    EXPECT_TRUE((6 == v[1] || 7 == v[1]) && (6 == v[2] || 7 == v[2]));
}

TEST_P(MutexTest, SignalWithConditionVariable)
{
    Dispatcher& dispatcher = getDispatcher();
    std::vector<int> v;
    
    Mutex m;
    ConditionVariable cv;
    
    //access the vector
    m.lock();
    
    //start a couple of coroutines waiting to access the vector
    dispatcher.post(0, false, [](ICoroContext<int>::Ptr ctx, Mutex& mu, std::vector<int>& vec, ConditionVariable& cv)->int{
        mu.lock(ctx);
        cv.wait(ctx, mu, [&vec]()->bool{ return !vec.empty(); });
        vec.push_back(6);
        mu.unlock();
        return 0;
    }, m, v, cv);
    dispatcher.post(0, false, [](ICoroContext<int>::Ptr ctx, Mutex& mu, std::vector<int>& vec, ConditionVariable& cv)->int{
        mu.lock(ctx);
        cv.wait(ctx, mu, [&vec]()->bool{ return !vec.empty(); });
        vec.push_back(7);
        mu.unlock();
        return 0;
    }, m, v, cv);
    
    std::this_thread::sleep_for(ms(200));
    v.push_back(5);
    m.unlock();
    
    cv.notifyAll();
    dispatcher.drain();
    
    ASSERT_EQ((size_t)3, v.size());
    EXPECT_EQ(5, v[0]);
    EXPECT_TRUE((6 == v[1] || 7 == v[1]) && (6 == v[2] || 7 == v[2]));
}

TEST_P(StressTest, ParallelFibonacciSerie)
{
    Dispatcher& dispatcher = getDispatcher();
    
    for (int i = 0; i < 1; ++i)
    {
        ThreadContext<size_t>::Ptr tctx = dispatcher.post(sequential_fib, fibInput);
        if (i == 0)
        {
            //Check once
            size_t num = tctx->get();
            EXPECT_EQ((size_t)fibValues[fibInput], num);
        }
    }
    dispatcher.drain();
    EXPECT_TRUE(dispatcher.empty());
    EXPECT_EQ((size_t)0, dispatcher.size());
}

TEST_P(StressTest, RecursiveFibonacciSerie)
{
    ThreadContext<size_t>::Ptr tctx = getDispatcher().post(recursive_fib, fibInput);
    EXPECT_EQ((size_t)fibValues[fibInput], tctx->get());
}

#ifdef BOOST_USE_VALGRIND
    int ioLoops = 10;
    int batchNum = 10;
#else
    int ioLoops = 10000;
    int batchNum = 1000;
#endif

TEST_P(StressTest, AsyncIo)
{
    std::mutex m;
    std::set<std::pair<int, int>> s; //queueId,iteration
    std::vector<std::pair<int, int>> v;
    v.reserve(ioLoops);
    for (int i = 0; i < ioLoops; ++i) {
        int queueId = i % getDispatcher().getNumIoThreads();
        getDispatcher().postAsyncIo(queueId, false, [&m,&v,&s,queueId,i](ThreadPromise<int>::Ptr promise){
            {
            std::lock_guard<std::mutex> lock(m);
            s.insert(std::make_pair(queueId, i));
            v.push_back(std::make_pair(queueId, i));
            }
            return promise->set(0);
        });
    }
    getDispatcher().drain();
    EXPECT_EQ(ioLoops, (int)v.size());
    EXPECT_EQ(ioLoops, (int)s.size()); //all elements unique
}

TEST_P(StressTest, AsyncIoAnyQueue)
{
    std::mutex m;
    std::set<std::pair<int, int>> s; //queueId,iteration
    std::vector<std::pair<int, int>> v;
    v.reserve(ioLoops);
    for (int i = 0; i < ioLoops; ++i) {
        int queueId = i % getDispatcher().getNumIoThreads();
        getDispatcher().postAsyncIo([&m,&v,&s,queueId,i](ThreadPromise<int>::Ptr promise){
            {
            std::lock_guard<std::mutex> lock(m);
            s.insert(std::make_pair(queueId, i));
            v.push_back(std::make_pair(queueId, i));
            }
            return promise->set(0);
        });
    }
    getDispatcher().drain();
    EXPECT_EQ(ioLoops, (int)v.size());
    EXPECT_EQ(ioLoops, (int)s.size()); //all elements unique
}

TEST_P(StressTestBalanced, AsyncIoAnyQueueLoadBalance)
{
    std::mutex m;
    std::set<std::pair<int, int>> s; //queueId,iteration
    std::vector<std::pair<int, int>> v;
    v.reserve(ioLoops);
    for (int i = 0; i < ioLoops; ++i) {
        int queueId = i % getDispatcher().getNumIoThreads();
        getDispatcher().postAsyncIo([&m,&v,&s,queueId,i](ThreadPromise<int>::Ptr promise){
            {
            std::lock_guard<std::mutex> lock(m);
            s.insert(std::make_pair(queueId, i));
            v.push_back(std::make_pair(queueId, i));
            }
            return promise->set(0);
        });
    }
    getDispatcher().drain();
    EXPECT_EQ(ioLoops, (int)v.size());
    EXPECT_EQ(ioLoops, (int)s.size()); //all elements unique
}

TEST_P(ForEachTest, Simple)
{
    std::vector<int> start{0,1,2,3,4,5,6,7,8,9};
    std::vector<char> end{'a','b','c','d','e','f','g','h','i','j'};
    std::vector<char> results = getDispatcher().forEach(start.cbegin(), start.size(),
        [](VoidContextPtr, const int& val)->char {
        return 'a'+val;
    })->get();
    EXPECT_EQ(end, results);
}

TEST_P(ForEachTest, SimpleNonConst)
{
    std::vector<int> start{0,1,2,3,4,5,6,7,8,9};
    std::vector<char> end{'b','c','d','e','f','g','h','i','j','k'};
    std::vector<char> results = getDispatcher().forEach(start.begin(), start.size(),
        [](VoidContextPtr ctx, int& val)->char {
        val = ctx->postAsyncIo([&](ThreadPromisePtr<int> p){
            return p->set(++val);
        })->get(ctx);
        return 'a'+ val;
    })->get();
    EXPECT_EQ(end, results);
    EXPECT_EQ(1, start[0]);
    EXPECT_EQ(10, start[9]);
}

TEST_P(ForEachTest, SmallBatch)
{
    std::vector<int> start{0,1,2};
    std::vector<char> end{'a','b','c'};
    
    std::vector<std::vector<char>> results = getDispatcher().forEachBatch(start.cbegin(), start.size(),
        [](VoidContextPtr, const int& val)->char
    {
        return 'a'+val;
    })->get();

    ASSERT_EQ(start.size(), results.size());
    EXPECT_EQ(results[0].front(), end[0]);
    EXPECT_EQ(results[1].front(), end[1]);
    EXPECT_EQ(results[2].front(), end[2]);
}

TEST_P(ForEachTest, LargeBatch)
{
    std::vector<int> start(batchNum);
    
    //Build a large input vector
    for (int i = 0; i < batchNum; ++i) {
        start[i]=i;
    }
    
    std::vector<std::vector<int>> results = getDispatcher().forEachBatch(start.begin(), start.size(),
        [](VoidContextPtr, int val)->int {
        return val*2; //double the value
    })->get();
    
    ASSERT_EQ((int)results.size(), getDispatcher().getNumCoroutineThreads());
    
    //Merge batches
    std::vector<int> merged;
    for (auto&& v : results) {
        merged.insert(merged.end(), v.begin(), v.end());
    }
    
    ASSERT_EQ(batchNum, (int)merged.size());
    for (size_t i = 0; i < merged.size(); ++i) {
        EXPECT_EQ(merged[i], start[i]*2);
    }
}

TEST_P(ForEachTest, LargeBatchFromCoroutine)
{
    getDispatcher().post([this](CoroContext<int>::Ptr ctx)->int {
        std::vector<int> start(batchNum);
    
        std::vector<std::vector<int>> results = ctx->forEachBatch(start.begin(), start.size(),
            [](VoidContextPtr, int val)->int {
            return val*2; //double the value
        })->get(ctx);
        
        EXPECT_EQ(getDispatcher().getNumCoroutineThreads(), (int)results.size());
        
        //Merge batches
        std::vector<int> merged;
        for (auto&& v : results) {
            merged.insert(merged.end(), v.begin(), v.end());
        }
        
        size_t size = merged.size();
        EXPECT_EQ(batchNum, size);
        for (size_t i = 0; i < size; ++i) {
            EXPECT_EQ(merged[i], start[i]*2);
        }
        return ctx->set(0);
    })->get();
}

TEST_P(MapReduce, OccuranceCount)
{
    //count the number of times a word of a specific length occurs
    std::vector<std::vector<std::string>> input = {
        {"a", "b", "aa", "aaa", "cccc" },
        {"bb", "bbb", "bbbb", "a", "bb"},
        {"aaa", "bb", "eee", "cccc", "d", "ddddd"},
        {"eee", "d", "a" }
    };
    
    std::map<std::string, size_t> result = getDispatcher().mapReduce(input.begin(), input.size(),
        //mapper
        [](VoidContextPtr, const std::vector<std::string>& input)->std::vector<std::pair<std::string, size_t>>
        {
            std::vector<std::pair<std::string, size_t>> out;
            for (auto&& i : input) {
                out.push_back({i, 1});
            }
            return out;
        },
        //reducer
        [](VoidContextPtr, std::pair<std::string, std::vector<size_t>>&& input)->std::pair<std::string, size_t>
        {
            size_t sum = 0;
            for (auto&& i : input.second) {
                sum += i;
            }
            return {std::move(input.first), sum};
        })->get();
    
    ASSERT_EQ(result.size(), 11UL);
    EXPECT_EQ(result["a"], 3UL);
    EXPECT_EQ(result["aa"], 1UL);
    EXPECT_EQ(result["aaa"], 2UL);
    EXPECT_EQ(result["b"], 1UL);
    EXPECT_EQ(result["bb"], 3UL);
    EXPECT_EQ(result["bbb"], 1UL);
    EXPECT_EQ(result["bbbb"], 1UL);
    EXPECT_EQ(result["cccc"], 2UL);
    EXPECT_EQ(result["d"], 2UL);
    EXPECT_EQ(result["ddddd"], 1UL);
    EXPECT_EQ(result["eee"], 2UL);
}

TEST_P(MapReduce, WordLength)
{
    //count the number of times each string occurs
    std::vector<std::vector<std::string>> input = {
        {"a", "b", "aa", "aaa", "cccc" },
        {"bb", "bbb", "bbbb", "a", "bb"},
        {"aaa", "bb", "eee", "cccc", "d", "ddddd"},
        {"eee", "d", "a" }
    };
    
    std::map<size_t, size_t> result = getDispatcher().mapReduceBatch(input.begin(), input.size(),
        //mapper
        [](VoidContextPtr, const std::vector<std::string>& input)->std::vector<std::pair<size_t, std::string>>
        {
            std::vector<std::pair<size_t, std::string>> out;
            for (auto&& i : input) {
                out.push_back({i.size(), i});
            }
            return out;
        },
        //reducer
        [](VoidContextPtr, std::pair<size_t, std::vector<std::string>>&& input)->std::pair<size_t, size_t>
        {
            return {input.first, input.second.size()};
        })->get();
    
    ASSERT_EQ(result.size(), 5UL); //longest word 'ddddd'
    EXPECT_EQ(result[1], 6UL);
    EXPECT_EQ(result[2], 4UL);
    EXPECT_EQ(result[3], 5UL);
    EXPECT_EQ(result[4], 3UL);
    EXPECT_EQ(result[5], 1UL);
}

TEST_P(MapReduce, WordLengthFromCoroutine)
{
    //count the number of times each string occurs
    std::vector<std::vector<std::string>> input = {
        {"a", "b", "aa", "aaa", "cccc" },
        {"bb", "bbb", "bbbb", "a", "bb"},
        {"aaa", "bb", "eee", "cccc", "d", "ddddd"},
        {"eee", "d", "a" }
    };
    
    getDispatcher().post([input](CoroContext<int>::Ptr ctx)->int
    {
        std::map<size_t, size_t> result = ctx->mapReduceBatch(input.begin(), input.size(),
        //mapper
        [](VoidContextPtr, const std::vector<std::string>& input)->std::vector<std::pair<size_t, std::string>>
        {
            std::vector<std::pair<size_t, std::string>> out;
            for (auto&& i : input) {
                out.push_back({i.size(), i});
            }
            return out;
        },
        //reducer
        [](VoidContextPtr, std::pair<size_t, std::vector<std::string>>&& input)->std::pair<size_t, size_t>
        {
            return {input.first, input.second.size()};
        })->get(ctx);
        
        EXPECT_EQ(result.size(), 5UL); //longest word 'ddddd'
        EXPECT_EQ(result[1], 6UL);
        EXPECT_EQ(result[2], 4UL);
        EXPECT_EQ(result[3], 5UL);
        EXPECT_EQ(result[4], 3UL);
        EXPECT_EQ(result[5], 1UL);
        
        return ctx->set(0);
    
    })->get();
}

TEST_P(FutureJoinerTest, JoinThreadFutures)
{
    std::vector<ThreadContext<int>::Ptr> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(getDispatcher().post([i](CoroContext<int>::Ptr ctx)->int {
            ctx->sleep(std::chrono::milliseconds(10));
            return ctx->set(i);
        }));
    }
    
    std::vector<int> output = FutureJoiner<int>()(getDispatcher(), std::move(futures))->get();
    EXPECT_EQ(output, std::vector<int>({0,1,2,3,4,5,6,7,8,9}));
}

TEST_P(FutureJoinerTest, JoinCoroFutures)
{
    std::vector<int> output;
    
    getDispatcher().post([&output](CoroContext<double>::Ptr ctx)->int {
        std::vector<CoroContext<int>::Ptr> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(ctx->post([i](CoroContext<int>::Ptr ctx2)->int {
                ctx2->sleep(std::chrono::milliseconds(10));
                return ctx2->set(i);
            }));
        }
        output = FutureJoiner<int>()(*ctx, std::move(futures))->get(ctx);
        return ctx->set(0);
    })->get();
    
    EXPECT_EQ(output, std::vector<int>({0,1,2,3,4,5,6,7,8,9}));
}

TEST(SharedQueueTest, PerformanceTest1)
{
    // The code below enqueues 30 short tasks, then 1 large task, and then 30 short tasks.
    // The intuition is that in the shared-coro mode, while one thread is busy with the large task,
    // the other threads will help it with the short tasks, and as a result
    // the shared-coro dispatcher will finish faster.
    size_t elapsedWithoutCoroSharing, elapsedWithCoroSharing;
    const std::vector<std::pair<size_t, ms>> sleepTimes =
        {{30, ms(10)},
         {1, ms(100)},
         {30, ms(10)}};
    {
        const TestConfiguration noCoroSharingConfig(false, false);
        quantum::Dispatcher& dispatcher = DispatcherSingleton::instance(noCoroSharingConfig);
        dispatcher.drain();
        auto start = std::chrono::steady_clock::now();
        enqueue_sleep_tasks(dispatcher, sleepTimes);
        dispatcher.drain();
        auto end = std::chrono::steady_clock::now();
        elapsedWithoutCoroSharing = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    }
    
    {
        const TestConfiguration coroSharingConfig(false, true);
        quantum::Dispatcher& dispatcher = DispatcherSingleton::instance(coroSharingConfig);
        dispatcher.drain();
        auto start = std::chrono::steady_clock::now();
        enqueue_sleep_tasks(dispatcher, sleepTimes);
        dispatcher.drain();
        auto end = std::chrono::steady_clock::now();
        elapsedWithCoroSharing = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    }
    EXPECT_LT(elapsedWithCoroSharing, elapsedWithoutCoroSharing);
}

TEST_P(CoroLocalStorageTest, AccessTest)
{
    for(int globalCounter = 0; globalCounter < 100; ++globalCounter)
    {
        getDispatcher().post([globalCounter](CoroContext<int>::Ptr ctx)->int
        {
            static const std::string globalCounterName = "globalCounter";            
            static const std::string localCounterName = "localCounter";
            // make sure nothing is inherited from the previous tasks
            EXPECT_EQ(nullptr, local::variable<int>(globalCounterName));
            EXPECT_EQ(nullptr, local::variable<int>(localCounterName));

            // set the local variable that remains constant
            int globalCounterCopy = globalCounter;
            local::variable<int>(globalCounterName) = &globalCounterCopy;

            int i = 0;
            // set the local variable that is changed in every iteration
            local::variable<int>(localCounterName) = &i;

            for(i = 0; i < 10; ++i)
            {
                ctx->sleep(ms(10));

                int* localCounterValue = local::variable<int>(localCounterName);
                EXPECT_EQ(&i, localCounterValue);

                int* globalCounterValue = local::variable<int>(globalCounterName);
                EXPECT_EQ(&globalCounterCopy, globalCounterValue);
            }

            // exit abnormally from time to time
            if (globalCounter % 5 == 0)
            {
                throw std::runtime_error("test");
            }

            return ctx->set(0);
        });
    }
    getDispatcher().drain();
}

TEST_P(CoroLocalStorageTest, GuardTest)
{
    getDispatcher().post([](CoroContext<int>::Ptr ctx)->int
    {
        const std::string name = "v";
        int v = 1;
        local::VariableGuard<int> guard1(name, &v);
        for(int i = 0; i < 10; ++i)
        {
            EXPECT_EQ(&v, local::variable<int>(name));
            local::VariableGuard<int> guard2(name, &i);

            for(int j = 0; j < 10; ++j)
            {
                EXPECT_EQ(&i, local::variable<int>(name));
                local::VariableGuard<int> guard3(name, &j);
                EXPECT_EQ(&j, local::variable<int>(name));
            }
            EXPECT_EQ(&i, local::variable<int>(name));
        }
        EXPECT_EQ(&v, local::variable<int>(name));
        return ctx->set(0);
    })->get();
}

TEST_P(CoroLocalStorageTest, GetContext)
{
    EXPECT_EQ(nullptr, local::context());
    
    getDispatcher().post([](CoroContext<int>::Ptr ctx)->int
    {
        EXPECT_NE(nullptr, local::context());
        EXPECT_EQ((CoroContext<Void>*)ctx.get(), local::context().get());
        return ctx->set(0);
    })->get();
}

//This test **must** come last to make Valgrind happy.
TEST_P(CleanupTest, DeleteDispatcherInstance)
{
    DispatcherSingleton::deleteInstances();
}






