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
#include <gmock/gmock.h>
#include <quantum_fixture.h>
#include <vector>
#include <set>
#include <map>

using namespace quantum;
using ms = std::chrono::milliseconds;

constexpr int Dispatcher::numCoro;
constexpr int Dispatcher::numThreads;
TaskDispatcher* Dispatcher::_dispatcher = nullptr;

//==============================================================================
//                           TEST HELPERS
//==============================================================================
int DummyCoro(CoroContext<int>::Ptr ctx)
{
    UNUSED(ctx);
    return 0;
}

int DummyIoTask(ThreadPromise<int>::Ptr promise)
{
    UNUSED(promise);
    std::this_thread::sleep_for(ms(10));
    return 0;
}

int fibInput = 20;
std::map<int, int> fibValues{{10, 55}, {20, 6765}, {25, 75025}, {30, 832040}};

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
    if (fib <= 2)
    {
        return ctx->set(1);
    }
    else
    {
        //Post both branches of the Fibonacci series before blocking on get().
        auto ctx1 = ctx->post<size_t>(recursive_fib, fib - 2);
        auto ctx2 = ctx->post<size_t>(recursive_fib, fib - 1);
        return ctx->set(ctx1->get(ctx) + ctx2->get(ctx));
    }
}

//==============================================================================
//                             TEST CASES
//==============================================================================
TEST_F(DispatcherFixture, Constructor)
{
    //Check if we have 0 coroutines and IO tasks running
    EXPECT_EQ(0, (int)_dispatcher->size(IQueue::QueueType::Coro));
    EXPECT_EQ(0, (int)_dispatcher->size(IQueue::QueueType::IO));
    EXPECT_EQ(0, (int)_dispatcher->size());
}

TEST_F(DispatcherFixture, CheckNumThreads)
{
    IThreadContext<int>::Ptr tctx = _dispatcher->post([](CoroContext<int>::Ptr ctx)->int{
        EXPECT_EQ(Dispatcher::numCoro, ctx->getNumCoroutineThreads());
        EXPECT_EQ(Dispatcher::numThreads, ctx->getNumIoThreads());
        return 0;
    });
    EXPECT_EQ(Dispatcher::numCoro, tctx->getNumCoroutineThreads());
    EXPECT_EQ(Dispatcher::numThreads, tctx->getNumIoThreads());
}

TEST_F(DispatcherFixture, CheckCoroutineQueuing)
{
    //Post various IO tasks and coroutines and make sure they executed on the proper queues
    for (int i = 0; i < 3; ++i)
    {
        _dispatcher->post(0, false, DummyCoro);
    }
    _dispatcher->post(1, true, DummyCoro);
    _dispatcher->post(2, false, DummyCoro);
    _dispatcher->drain();
    
    //Posted
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 1).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 2).postedCount());
    EXPECT_EQ((size_t)5, _dispatcher->stats(IQueue::QueueType::Coro).postedCount()); //total
    
    //Completed
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::Coro, 0).completedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 1).completedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 2).completedCount());
    EXPECT_EQ((size_t)5, _dispatcher->stats(IQueue::QueueType::Coro).completedCount()); //total
    
    //Errors
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 0).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 1).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 2).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro).errorCount()); //total
    
    //High Priority
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro).highPriorityCount()); //total
    
    //Check if all tasks have stopped
    EXPECT_EQ((size_t)0, _dispatcher->size(IQueue::QueueType::Coro));
}

TEST_F(DispatcherFixture, CheckIoQueuing)
{
    //IO (10 tasks)
    for (int i = 0; i < 10; ++i)
    {
        _dispatcher->postAsyncIo(DummyIoTask); //shared queue
    }
    _dispatcher->postAsyncIo(1, true, DummyIoTask);
    _dispatcher->postAsyncIo(2, false, DummyIoTask);
    
    _dispatcher->drain();
    
    //Posted
    EXPECT_EQ((size_t)10, _dispatcher->stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 1).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 2).postedCount());
    EXPECT_EQ((size_t)12, _dispatcher->stats(IQueue::QueueType::IO).postedCount()); //total
    
    //Completed
    EXPECT_LE((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).completedCount());
    EXPECT_EQ((size_t)10, _dispatcher->stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).completedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO, 0).sharedQueueCompletedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO, 1).sharedQueueCompletedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO, 2).sharedQueueCompletedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO, 3).sharedQueueCompletedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO, 4).sharedQueueCompletedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 1).completedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 2).completedCount());
    EXPECT_EQ((size_t)12, _dispatcher->stats(IQueue::QueueType::IO).completedCount() +
                          _dispatcher->stats(IQueue::QueueType::IO).sharedQueueCompletedCount()); //total
    
    //Errors
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, 1).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, 2).errorCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO).errorCount()); //total
    
    //High Priority
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, (int)IQueue::QueueId::Any).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO).highPriorityCount()); //total
    
    //Check if all tasks have stopped
    EXPECT_EQ((size_t)0, _dispatcher->size(IQueue::QueueType::IO));
}

TEST_F(DispatcherFixture, CheckQueuingFromSameCoroutine)
{
    _dispatcher->post(0, false, [](CoroContext<int>::Ptr ctx)->int {
        ctx->postFirst(1, true, DummyCoro)->then(DummyCoro)->finally(DummyCoro)->end();
        return 0;
    });
    _dispatcher->drain();
    
    //Posted
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::Coro, 1).postedCount());
    EXPECT_EQ((size_t)4, _dispatcher->stats(IQueue::QueueType::Coro).postedCount()); //total
    
    //High priority
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::Coro, 1).highPriorityCount());
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::Coro).highPriorityCount()); //total
}

TEST_F(DispatcherFixture, CheckIoQueuingFromACoroutine)
{
    _dispatcher->post(0, false, [](CoroContext<int>::Ptr ctx)->int {
        ctx->postAsyncIo(1, true, DummyIoTask);
        ctx->postAsyncIo(2, false, DummyIoTask);
        ctx->postAsyncIo(3, true, DummyIoTask);
        return 0;
    });
    _dispatcher->drain();
    
    //Posted
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro, 0).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 1).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 2).postedCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 3).postedCount());
    
    //High priority
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::Coro, 0).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 1).highPriorityCount());
    EXPECT_EQ((size_t)0, _dispatcher->stats(IQueue::QueueType::IO, 2).highPriorityCount());
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::IO, 3).highPriorityCount());
    
    //Completed count
    EXPECT_EQ((size_t)1, _dispatcher->stats(IQueue::QueueType::Coro).completedCount());
    EXPECT_EQ((size_t)3, _dispatcher->stats(IQueue::QueueType::IO).completedCount());
    
    //Remaining
    EXPECT_EQ((size_t)0, _dispatcher->size());
}

TEST_F(DispatcherFixture, CheckCoroutineErrors)
{
    std::string s("original"); //string must remain unchanged
    
    _dispatcher->post([](CoroContext<int>::Ptr ctx, std::string& str)->int {
        ctx->yield();
        return 1; //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    _dispatcher->post([](CoroContext<int>::Ptr ctx, std::string& str)->int {
        ctx->yield();
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    _dispatcher->postAsyncIo([](ThreadPromise<int>::Ptr, std::string& str)->int {
        std::this_thread::sleep_for(ms(10));
        return 1; //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    _dispatcher->postAsyncIo([](ThreadPromise<int>::Ptr, std::string& str)->int {
        std::this_thread::sleep_for(ms(10));
        throw std::exception(); //error! coroutine must stop here
        str = "changed";
        return 0;
    }, s);
    
    _dispatcher->drain();
    
    //Error count
    EXPECT_EQ((size_t)2, _dispatcher->stats(IQueue::QueueType::Coro).errorCount());
    EXPECT_EQ((size_t)2, _dispatcher->stats(IQueue::QueueType::IO).errorCount() +
                         _dispatcher->stats(IQueue::QueueType::IO).sharedQueueErrorCount());
    EXPECT_STREQ("original", s.c_str());
    
    //Remaining
    EXPECT_EQ((size_t)0, _dispatcher->size());
}

struct NonCopyable {
    NonCopyable(const std::string& str) : _str(str){}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = default;
    std::string _str;
};

TEST(ParamtersTest, CheckParameterPassingInCoroutines)
{
    //Test pass by value, reference and address.
    int a = 5;
    std::string str = "original";
    NonCopyable nc("move");
    double dbl = 4.321;
    
    auto func = [&](CoroContext<int>::Ptr, int byVal, std::string& byRef, std::tuple<NonCopyable&&>& byRvalue, double* byAddress)->int {
        //modify all passed-in values
        byVal = 6; UNUSED(byVal);
        byRef = "changed";
        EXPECT_EQ(byRef.data(), str.data());
        *byAddress = 6.543;
        NonCopyable s = std::move(std::get<0>(byRvalue));
        EXPECT_STREQ("move", s._str.c_str());
        return 0;
    };
    
    auto t = std::forward_as_tuple(std::move(nc));
    TaskDispatcher& dispatcher = Dispatcher::instance();
    dispatcher.post(func, a, str, t, &dbl);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(5, a);
    EXPECT_STREQ("changed", str.c_str());
    EXPECT_EQ(0, (int)nc._str.size());
    EXPECT_DOUBLE_EQ(6.543, dbl);
}

TEST(ExecutionTest, DrainAllTasks)
{
    //Turn the drain on and make sure we cannot queue any tasks
    TaskDispatcher& dispatcher = Dispatcher::instance();
    
    //Post a bunch of coroutines to run and wait for completion.
    for (int i = 0; i < 100; ++i)
    {
        dispatcher.post(DummyCoro);
    }
    
    dispatcher.drain();
    EXPECT_EQ((size_t)0, dispatcher.size(IQueue::QueueType::Coro));
    EXPECT_EQ((size_t)0, dispatcher.size());
}

TEST(ExecutionTest, YieldingBetweenTwoCoroutines)
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
    
    TaskDispatcher& dispatcher = Dispatcher::instance();
    
    dispatcher.post(3, false, func, testSet);
    dispatcher.post(3, false, func2, testSet);
    dispatcher.drain();
    
    std::set<int> validation{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(validation, testSet);
}

TEST(ExecutionTest, ChainCoroutinesFromDispatcher)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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

TEST(ExecutionTest, ChainCoroutinesFromCoroutineContext)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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
        ctx->postFirst(func2, v, i)->then(func2, v, i)->then(func2, v, i)->
             then(func2, v, i)->onError(func2, v, err)->finally(func2, v, final)->end(); //OnError *should not* run
        return 0;
    };
    
    dispatcher.post(func, v, i);
    dispatcher.drain();
    
    //Validate values
    EXPECT_EQ(validation, v);
}

TEST(ExecutionTest, OnErrorTaskRuns)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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

TEST(ExecutionTest, FinallyAlwaysRuns)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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

TEST(ExecutionTest, CoroutineSleep)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(100));
        return 0;
    });
    
    auto start = std::chrono::high_resolution_clock::now();
    ctx->wait(); //block until value is available
    auto end = std::chrono::high_resolution_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)100);
}

TEST(PromiseTest, GetFutureFromCoroutine)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        return ctx->set(55); //Set the promise
    });
    EXPECT_EQ(55, ctx->get()); //block until value is available
    EXPECT_THROW(ctx->get(), FutureAlreadyRetrievedException);
}

TEST(PromiseTest, GetFutureFromIoTask)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    ThreadContext<int>::Ptr ctx = dispatcher.post([](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<double>::Ptr fut = ctx->postAsyncIo<double>([](ThreadPromise<double>::Ptr promise)->int{
            return promise->set(33.22);
        });
        return ctx->set((int)fut->get(ctx)); //forward the promise
    });
    EXPECT_EQ(33, ctx->get()); //block until value is available
}

TEST(PromiseTest, BufferedFuture)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    ThreadContext<Buffer<int>>::Ptr ctx = dispatcher.post<Buffer<int>>([](CoroContext<Buffer<int>>::Ptr ctx)->int{
        for (int d = 0; d < 5; d++)
        {
            ctx->push(d);
            ctx->yield(); //simulate some arbitrary delay
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
    std::vector<int> validate{0,1,2,3,4};
    EXPECT_EQ(validate, v);
}

TEST(PromiseTest, GetFutureReference)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        return ctx->set(55); //Set the promise
    });
    EXPECT_EQ(55, ctx->getRef()); //block until value is available
    EXPECT_NO_THROW(ctx->getRef());
    EXPECT_NO_THROW(ctx->get());
    EXPECT_THROW(ctx->get(), FutureAlreadyRetrievedException);
}

TEST(PromiseTest, GetIntermediateFutures)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    auto ctx = dispatcher.postFirst([](CoroContext<int>::Ptr ctx)->int {
        return ctx->set(55); //Set the promise
    })->then<double>([](CoroContext<double>::Ptr ctx)->int {
        return ctx->set(22.33); //Set the promise
    })->then<std::string>([](CoroContext<std::string>::Ptr ctx)->int {
        return ctx->set("future"); //Set the promise
    })->then<std::list<int>>([](CoroContext<std::list<int>>::Ptr ctx)->int {
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

TEST(PromiseTest, GetPreviousFutures)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    auto ctx = dispatcher.postFirst([](CoroContext<int>::Ptr ctx)->int {
        return ctx->set(55); //Set the promise
    })->then<double>([](CoroContext<double>::Ptr ctx)->int {
        EXPECT_EQ(55, ctx->getPrev<int>());
        return ctx->set(22.33); //Set the promise
    })->then<std::string>([](CoroContext<std::string>::Ptr ctx)->int {
        EXPECT_DOUBLE_EQ(22.33, ctx->getPrev<double>());
        return ctx->set("future"); //Set the promise
    })->then<std::list<int>>([](CoroContext<std::list<int>>::Ptr ctx)->int {
        EXPECT_STREQ("future", ctx->getPrevRef<std::string>().c_str());
        return ctx->set(std::list<int>{1,2,3}); //Set the promise
    })->end();
    
    std::list<int> validate{1,2,3};
    EXPECT_EQ(validate, ctx->get()); //ok - can read value again
    EXPECT_STREQ("future", ctx->getAt<std::string>(2).c_str());
}

TEST(PromiseTest, BrokenPromiseInAsyncIo)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    ThreadContext<int>::Ptr ctx = dispatcher.post([](CoroContext<int>::Ptr ctx)->int{
        //post an IO task and get future from there
        CoroFuture<double>::Ptr fut = ctx->postAsyncIo<double>([](ThreadPromise<double>::Ptr promise)->int{
            ITerminate::Guard guard(*promise);
            //Do not set the promise so that we break it
            return 0;
        });
        EXPECT_THROW(fut->get(ctx), BrokenPromiseException); //broken promise
        return 0;
    });
}

TEST(PromiseTest, BreakPromiseByThrowingError)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr)->int{
        throw std::runtime_error("don't set the promise");
    });
    EXPECT_THROW(ctx->getRef(), std::runtime_error);
    EXPECT_THROW(ctx->get(), std::runtime_error);
}

TEST(PromiseTest, PromiseBrokenWhenOnError)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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

TEST(PromiseTest, SetExceptionInPromise)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
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

TEST(PromiseTest, FutureTimeout)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(300));
        return 0;
    });
    
    auto start = std::chrono::high_resolution_clock::now();
    std::future_status status = ctx->waitFor(ms(100)); //block until value is available or 100ms have expired
    auto end = std::chrono::high_resolution_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_LT(elapsed, (size_t)300);
    EXPECT_EQ(status, std::future_status::timeout);
}

TEST(PromiseTest, FutureWithoutTimeout)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    IThreadContext<int>::Ptr ctx = dispatcher.post([](ICoroContext<int>::Ptr ctx)->int{
        ctx->sleep(ms(100));
        return 0;
    });
    
    auto start = std::chrono::high_resolution_clock::now();
    std::future_status status = ctx->waitFor(ms(300)); //block until value is available or 300ms have expired
    auto end = std::chrono::high_resolution_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)100);
    EXPECT_LT(elapsed, (size_t)300);
    EXPECT_EQ(status, std::future_status::ready);
}

TEST(PromiseTest, WaitForAllFutures)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    auto func = [](CoroContext<int>::Ptr ctx)->int {
        ctx->sleep(ms(50));
        return 0;
    };
    
    auto ctx = dispatcher.postFirst(func)->then(func)->then(func)->then(func)->end();
    auto start = std::chrono::high_resolution_clock::now();
    ctx->waitAll(); //block until value is available or 4x50ms has expired
    auto end = std::chrono::high_resolution_clock::now();
    
    //check elapsed time
    size_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    EXPECT_GE(elapsed, (size_t)200);
}

TEST(MutexTest, LockingAndUnlocking)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    std::vector<int> v;
    
    quantum::Mutex m;
    
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

TEST(MutexTest, SignalWithConditionVariable)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    std::vector<int> v;
    
    quantum::Mutex m;
    quantum::ConditionVariable cv;
    
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

TEST(StressTest, ParallelFibonacciSerie)
{
    TaskDispatcher& dispatcher = Dispatcher::instance();
    
    for (int i = 0; i < 1; ++i)
    {
        ThreadContext<size_t>::Ptr tctx = dispatcher.post<size_t>(sequential_fib, fibInput);
        
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

TEST(StressTest, RecursiveFibonacciSerie)
{
    ThreadContext<size_t>::Ptr tctx = Dispatcher::instance().post<size_t>(recursive_fib, fibInput);
    EXPECT_EQ((size_t)fibValues[fibInput], tctx->get());
}

TEST(StressTest, AsyncIo)
{
    std::mutex m;
    std::set<std::pair<int, int>> s; //queueId,iteration
    std::vector<std::pair<int, int>> v;
    v.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        int queueId = i % Dispatcher::instance().getNumIoThreads();
        Dispatcher::instance().postAsyncIo<int>(queueId, false, [&m,&v,&s,queueId,i](ThreadPromise<int>::Ptr promise){
            {
            std::lock_guard<std::mutex> lock(m);
            s.insert(std::make_pair(queueId, i));
            v.push_back(std::make_pair(queueId, i));
            }
            return promise->set(0);
        });
    }
    Dispatcher::instance().drain();
    EXPECT_EQ(1000, s.size()); //all elements unique
    EXPECT_EQ(1000, v.size()); //all elements unique
}

//This test **must** come last to make Valgrind happy.
TEST(TestCleanup, DeleteDispatcherInstance)
{
    Dispatcher::deleteInstance();
}








