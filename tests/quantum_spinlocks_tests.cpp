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

using namespace Bloomberg::quantum;

#ifdef BOOST_USE_VALGRIND
    int spins = 100;
#else
    int spins = 1000000;
#endif

TEST(Spinlock, Spinlock)
{
    int num = spins;
    int val = 0;
    SpinLock spin;
    std::thread t1([&, num]() mutable {
        while (num--) {
            SpinLock::Guard guard(spin);
            ++val;
        }
    });
    std::thread t2([&, num]() mutable {
        while (num--) {
            SpinLock::Guard guard(spin);
            --val;
        }
    });
    t1.join();
    t2.join();
    EXPECT_EQ(0, val);
}

TEST(ReadWriteSpinLock, LockReadMultipleTimes)
{
    ReadWriteSpinLock spin;
    EXPECT_EQ(0, spin.numReaders());
    EXPECT_FALSE(spin.isLocked());
    spin.lockRead();
    EXPECT_TRUE(spin.isLocked());
    EXPECT_EQ(1, spin.numReaders());
    spin.lockRead();
    EXPECT_TRUE(spin.isLocked());
    EXPECT_EQ(2, spin.numReaders());
    spin.unlockRead();
    spin.unlockRead();
    EXPECT_EQ(0, spin.numReaders());
    EXPECT_FALSE(spin.isLocked());
}

TEST(ReadWriteSpinLock, LockReadAndWrite)
{
    int num = spins;
    int val = 0;
    ReadWriteSpinLock spin;
    std::thread t1([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::ReadGuard guard(spin);
        }
    });
    std::thread t2([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::ReadGuard guard(spin);
        }
    });
    std::thread t3([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::ReadGuard guard(spin);
        }
    });
    std::thread t4([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::WriteGuard guard(spin);
            ++val;
        }
    });
    std::thread t5([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::WriteGuard guard(spin);
            --val;
        }
    });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    EXPECT_EQ(0, val);
}

TEST(ReadWriteSpinLock, LockReadAndWriteList)
{
    int num = spins;
    std::list<int> val;
    ReadWriteSpinLock spin;
    bool exit = false;
    std::thread t1([&]() mutable {
        while (!exit) {
            ReadWriteSpinLock::ReadGuard guard(spin);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t2([&]() mutable {
        while (!exit) {
            ReadWriteSpinLock::ReadGuard guard(spin);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t3([&]() mutable {
        while (!exit) {
            ReadWriteSpinLock::ReadGuard guard(spin);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t4([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::WriteGuard guard(spin);
            val.push_back(1);
        }
    });
    std::thread t5([&, num]() mutable {
        while (num) {
            ReadWriteSpinLock::WriteGuard guard(spin);
            if (!val.empty()) {
                val.pop_back();
                --num;
            }
        }
    });
    t4.join();
    t5.join();
    exit = true;
    t1.join();
    t2.join();
    t3.join();
    EXPECT_EQ(0, val.size());
}
