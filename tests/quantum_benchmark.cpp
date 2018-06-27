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
#if 0

#include <iostream>
#include <list>
#include <chrono>
#include <quantum/quantum_dispatcher.h>

namespace quantum = Bloomberg::quantum;

int dispatcher_fib(quantum::ICoroContext<size_t>::ptr ctx, size_t fib) {
    if (fib < 2)
    {
        ctx->set(1);
    }
    else
    {
        ctx->set(ctx->post<size_t>(dispatcher_fib, fib - 1)->get(ctx) +
                 ctx->post<size_t>(dispatcher_fib, fib - 2)->get(ctx));
    }
    return 0; //OK
};

void dispatcher_test_sequential_fib(int numThreads, bool pinToCores, int iterations, size_t fibNumber)
{
    quantum::TaskDispatcher pool(numThreads, 5, pinToCores);
    
    std::chrono::system_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        quantum::IThreadContext<size_t>::ptr tctx = pool.post<size_t>([fibNumber](quantum::ICoroContext<size_t>::ptr ctx)->int {
            size_t a = 0, b = 1, c, i;
            for (i = 2; i <= fibNumber; i++)
            {
                c = a + b;
                a = b;
                b = c;
            }
            ctx->set(c);
            return 0; //OK
        });
    }
    std::chrono::system_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    pool.drain();
    std::chrono::system_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    
    long enqueueMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    long totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count();
    
    pool.terminate();
    
    std::cout << "\n========== DISPATCHER TEST (FIB SEQUENTIAL) ===========" << std::endl;
    std::cout << "threads(" << numThreads << ") "
              << "pin(" << pinToCores << ") "
              << "iterations(" << iterations << ") "
              << "fib(" << fibNumber << ") "
              << "enqueuMs(" << enqueueMs << ") "
              << "execMs(" << totalMs << ")" << std::endl;
}

void dispatcher_test_recursive_fib(int numThreads, bool pinToCores, int iterations, size_t fibNumber)
{
    quantum::TaskDispatcher pool(numThreads, 5, pinToCores);
    
    std::chrono::system_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    quantum::IThreadContext<size_t>::ptr tctx;
    for (int i = 0; i < iterations; ++i)
    {
        tctx = pool.post<size_t>(dispatcher_fib, fibNumber);
    }
    std::chrono::system_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    pool.drain();
    std::chrono::system_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    
    long enqueueMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    long totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count();
    
    pool.terminate();
    //pool.stats().print();
    
    std::cout << "\n========== DISPATCHER TEST (FIB RECURSIVE) ===========" << std::endl;
    std::cout << "threads(" << numThreads << ") "
              << "pin(" << pinToCores << ") "
              << "iterations(" << iterations << ") "
              << "fib(" << fibNumber << ") "
              << "enqueuMs(" << enqueueMs << ") "
              << "execMs(" << totalMs << ")" << std::endl;
}

void dispatcher_benchmark(int num)
{
    int max_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < num; ++i )
    {
        std::cout << "\n########### RUNNING DISPATCHER LOOP " << i << " ###########" << std::endl;
        dispatcher_test_sequential_fib(1, false, 10000, 1000000);
        dispatcher_test_sequential_fib(1, true, 10000, 1000000);
        dispatcher_test_sequential_fib(max_threads/2, false, 10000, 1000000);
        dispatcher_test_sequential_fib(max_threads/2, true, 10000, 1000000);
        dispatcher_test_sequential_fib(max_threads, false, 10000, 1000000);
        dispatcher_test_sequential_fib(max_threads, true, 10000, 1000000);

        dispatcher_test_recursive_fib(1, false, 1, 25);
        dispatcher_test_recursive_fib(1, true, 1, 25);
        dispatcher_test_recursive_fib(max_threads/2, false, 1, 25);
        dispatcher_test_recursive_fib(max_threads/2, true, 1, 25);
        dispatcher_test_recursive_fib(max_threads, false, 1, 25);
        dispatcher_test_recursive_fib(max_threads, true, 1, 25);
    }
}

#endif
