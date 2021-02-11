/*
** Copyright 2021 Bloomberg Finance L.P.
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
#include <quantum/quantum_dispatcher.h>
#include <quantum/util/quantum_generic_future.h>
#include <quantum_fixture.h>
#include <gtest/gtest.h>
#include <vector>

using namespace Bloomberg::quantum;
using ms = std::chrono::milliseconds;

TEST(GenericFuture, MakeFutureInCoroutineAndMainThread)
{
    Dispatcher& dispatcher = DispatcherSingleton::instance({false, false});
    GenericFuture<int> threadFuture = dispatcher.post([](VoidContextPtr ctx)->int {
        //post an IO task and get future from there
        GenericFuture<int> coroFuture(ctx->postAsyncIo([]()->int {
            return 33;
        }), ctx);
        return coroFuture.get(); //forward the promise to main thread
    });
    EXPECT_EQ(33, threadFuture.get()); //block until value is available
}

TEST(GenericFuture, WaitForCoroutineFutureInIoTask)
{
    Dispatcher& dispatcher = DispatcherSingleton::instance({false, false});
    GenericFuture<int> threadFuture = dispatcher.post([](VoidContextPtr ctx)->int {
        //post an IO task and get future from there
        volatile int i = 0;
        GenericFuture<int> coroFuture0(ctx->postAsyncIo([&i]()->int {
            while (i==0) {
                std::this_thread::sleep_for(ms(10));
            }
            return 33;
        }), ctx);
        
        //start another IO task and wait in the previous future
        GenericFuture<int> coroFuture1(ctx->postAsyncIo([&i, &coroFuture0]()->int {
            //wait on this future
            i = 1;
            return coroFuture0.get() + 10; //wait on the coroutine future
        }), ctx);
        
        return coroFuture1.get(); //return value to main thread
    });
    EXPECT_EQ(43, threadFuture.get()); //block until value is available
}

TEST(GenericFuture, WaitForIoFutureInCoroutine)
{
    Dispatcher& dispatcher = DispatcherSingleton::instance({false, false});
    volatile int i = 0;
    GenericFuture<int> ioFuture = dispatcher.postAsyncIo([&i]()->int {
        while (i==0) {
            std::this_thread::sleep_for(ms(10));
        }
        return 33;
    });
    
    GenericFuture<int> threadFuture = dispatcher.post([&i, &ioFuture](VoidContextPtr)->int {
        i = 1;
        return ioFuture.get() + 10;
    });
    
    EXPECT_EQ(43, threadFuture.get()); //block until value is available
}

TEST(GenericFuture, TestCopyable)
{
    Dispatcher& dispatcher = DispatcherSingleton::instance({false, false});
    std::vector<GenericFuture<int>> v;
    
    //create one future
    v.push_back(dispatcher.post([](VoidContextPtr)->int{ return 33; }));
    
    //copy it
    v.push_back(v.front());
    
    //read the value from the first future
    EXPECT_EQ(33, v.front().get());
    
    //read from the second future and we should throw
    EXPECT_THROW(v.back().get(), FutureAlreadyRetrievedException);
}