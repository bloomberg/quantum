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
#include <chrono>
#include <ctime>
#include <thread>
#include <memory>

using namespace Bloomberg::quantum;

#ifdef BOOST_USE_VALGRIND
    int spins = 100;
#else
    int spins = 100000;
#endif
    int val = 0;
    int numThreads = 20;
    int numLockAcquires = 100;

void runnable(SpinLock* exclusiveLock) {
    int locksTaken = 0;
    while (locksTaken < numLockAcquires) {
        exclusiveLock->lock();
        locksTaken++;
        val++;
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        exclusiveLock->unlock();
    }
}

void runThreads(int num)
{
    SpinLock exclusiveLock;
    //Create 50 threads
    exclusiveLock.lock(); //lock it so that all threads block
    std::vector<std::shared_ptr<std::thread>> threads;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::make_shared<std::thread>(runnable, &exclusiveLock));
    }
    exclusiveLock.unlock(); //unlock to start contention
    for (auto& t : threads) {
        t->join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Total spin time " << num << ": "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()
            << "ms" << std::endl;
}

void spinlockSettings(
        size_t min,
        size_t max,
        std::chrono::microseconds sleepUs,
        size_t numYields,
        int num,
        int enable)
{
    if (enable == -1 || enable == num) {
        val = 0;
        SpinLockTraits::minSpins() = min;
        SpinLockTraits::maxSpins() = max;
        SpinLockTraits::numYieldsBeforeSleep() = numYields;
        SpinLockTraits::sleepDuration() = sleepUs;
        runThreads(num);
        EXPECT_EQ(numThreads*numLockAcquires, val);
    }
}

TEST(Spinlock, Spinlock)
{
    val = 0;
    SpinLock spin;
    std::thread t1([&]() mutable {
        int num = spins;
        while (num-- > 0) {
            SpinLock::Guard guard(spin);
            //std::this_thread::yield();
            ++val;
        }
    });
    std::thread t2([&]() mutable {
        int num = spins;
        while (num-- > 0) {
            SpinLock::Guard guard(spin);
            //std::this_thread::yield();
            --val;
        }
    });
    t1.join();
    t2.join();
    EXPECT_EQ(0, val);
}

TEST(Spinlock, Guards)
{
    SpinLock spin;
    { //Basic
        SpinLock::Guard guard(spin);
        EXPECT_TRUE(guard.ownsLock());
    }
    EXPECT_FALSE(spin.isLocked());
    
    { //TryLock
        spin.lock();
        SpinLock::Guard guard(spin, lock::tryToLock);
        EXPECT_FALSE(guard.ownsLock());
        spin.unlock();
    }
    EXPECT_FALSE(spin.isLocked());
    
    { //AdoptLock
        spin.lock();
        SpinLock::Guard guard(spin, lock::adoptLock);
        EXPECT_TRUE(guard.ownsLock());
    }
    EXPECT_FALSE(spin.isLocked());
    
    { //AdoptLock -- unlocked
        SpinLock::Guard guard(spin, lock::adoptLock);
        EXPECT_FALSE(guard.ownsLock());
    }
    EXPECT_FALSE(spin.isLocked());
    
    { //DeferLock
        spin.lock();
        SpinLock::Guard guard(spin, lock::deferLock);
        EXPECT_FALSE(guard.ownsLock());
        spin.unlock();
        guard.lock();
        EXPECT_TRUE(guard.ownsLock());
    }
    EXPECT_FALSE(spin.isLocked());
    
    { //DeferLock -- unlocked
        SpinLock::Guard guard(spin, lock::deferLock);
        EXPECT_FALSE(guard.ownsLock());
    }
    EXPECT_FALSE(spin.isLocked());
}

TEST(Spinlock, HighContention)
{
    val = 0;
    int enable = -1;
    int i = 0;
    spinlockSettings(500, 10000, std::chrono::microseconds(100), 2, i++, enable); //0
    spinlockSettings(0, 20000, std::chrono::microseconds(100), 3, i++, enable); //1
    spinlockSettings(100, 5000, std::chrono::microseconds(200), 3, i++, enable); //2
    spinlockSettings(500, 200000, std::chrono::microseconds(0), 5, i++, enable); //3
    spinlockSettings(500, 20000, std::chrono::microseconds(1000), 0, i++, enable); //4
    spinlockSettings(500, 2000, std::chrono::microseconds(0), 0, i++, enable); //5
    spinlockSettings(0, 0, std::chrono::microseconds(10), 2000, i++, enable); //6
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
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
        }
    });
    std::thread t2([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
        }
    });
    std::thread t3([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
        }
    });
    std::thread t4([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireWrite);
            ++val;
        }
    });
    std::thread t5([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireWrite);
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
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t2([&]() mutable {
        while (!exit) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t3([&]() mutable {
        while (!exit) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireRead);
            auto it = val.rbegin();
            if (it != val.rend()) --it;
        }
    });
    std::thread t4([&, num]() mutable {
        while (num--) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireWrite);
            val.push_back(1);
        }
    });
    std::thread t5([&, num]() mutable {
        while (num) {
            ReadWriteSpinLock::Guard guard(spin, lock::acquireWrite);
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

TEST(ReadWriteSpinLock, SingleLocks)
{
    ReadWriteSpinLock lock;

    EXPECT_FALSE(lock.isLocked());
    EXPECT_FALSE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(0, lock.numReaders());

    lock.lockRead();
    EXPECT_TRUE(lock.isLocked());
    EXPECT_TRUE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(1, lock.numReaders());

    lock.unlockRead();
    EXPECT_FALSE(lock.isLocked());
    EXPECT_FALSE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(0, lock.numReaders());

    lock.lockWrite();
    EXPECT_TRUE(lock.isLocked());
    EXPECT_FALSE(lock.isReadLocked());
    EXPECT_TRUE(lock.isWriteLocked());
    EXPECT_EQ(0, lock.numReaders());
}

TEST(ReadWriteSpinLock, UnlockingUnlockedIsNoOp)
{
    ReadWriteSpinLock lock;

    ASSERT_FALSE(lock.isLocked());

    lock.unlockRead();
    EXPECT_FALSE(lock.isLocked());

    lock.unlockWrite();
    EXPECT_FALSE(lock.isLocked());
}

TEST(ReadWriteSpinLock, TryLocks)
{
    ReadWriteSpinLock lock;

    ASSERT_FALSE(lock.isLocked());

    EXPECT_TRUE(lock.tryLockRead());
    EXPECT_TRUE(lock.isReadLocked());
    EXPECT_FALSE(lock.tryLockWrite());

    lock.unlockRead();
    EXPECT_TRUE (lock.tryLockWrite());
    EXPECT_TRUE(lock.isWriteLocked());

    EXPECT_FALSE(lock.tryLockRead());
}

TEST(ReadWriteSpinLock, Guards)
{
    ReadWriteSpinLock lock;

    ASSERT_FALSE(lock.isLocked());

    // Guard
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead);
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());

    // Guard, TryLock
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead, lock::tryToLock);
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, AdoptLock - unlocked
    {
        ReadWriteSpinLock::Guard guard(lock, lock::adoptLock);
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, AdoptLock - read locked
    {
        lock.lockRead();
        ReadWriteSpinLock::Guard guard(lock, lock::adoptLock);
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, AdoptLock - write locked
    {
        lock.lockWrite();
        ReadWriteSpinLock::Guard guard(lock, lock::adoptLock);
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, DeferLock - unlocked
    {
        ReadWriteSpinLock::Guard guard(lock, lock::deferLock);
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, DeferLock - read locked
    {
        lock.lockRead();
        ReadWriteSpinLock::Guard guard(lock, lock::deferLock);
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        lock.unlockRead();
        guard.lockRead();
        EXPECT_TRUE(guard.ownsReadLock());
    }
    EXPECT_FALSE(lock.isLocked());
    
    // Guard, DeferLock - write locked
    {
        lock.lockWrite();
        ReadWriteSpinLock::Guard guard(lock, lock::deferLock);
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        lock.unlockWrite();
        guard.lockWrite();
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(lock.isLocked());

    // WriteGuard
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireWrite);
        EXPECT_TRUE(lock.isWriteLocked());
    }
    EXPECT_FALSE(lock.isLocked());

    // WriteGuard, TryLock
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(lock.isWriteLocked());
    }
    EXPECT_FALSE(lock.isLocked());

    // Guard, WriteGuard, TryLock (fails)
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead);
        EXPECT_TRUE(lock.isReadLocked());
        ReadWriteSpinLock::Guard writeGuard(lock, lock::acquireWrite, lock::tryToLock);
        EXPECT_FALSE(lock.isWriteLocked());
    }
    EXPECT_FALSE(lock.isLocked());

    // Guard, unlock, WriteGuard
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead);
        EXPECT_TRUE(lock.isReadLocked());
        guard.unlock();
        EXPECT_FALSE(lock.isLocked());
        ReadWriteSpinLock::Guard writeGuard(lock, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(lock.isWriteLocked());
    }
    EXPECT_FALSE(lock.isLocked());

    // Guard, unlock, WriteGuard, unlock
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead);
        EXPECT_TRUE(lock.isReadLocked());
        guard.unlock();
        EXPECT_FALSE(lock.isLocked());
        ReadWriteSpinLock::Guard writeGuard(lock, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(lock.isWriteLocked());
        writeGuard.unlock();
        EXPECT_FALSE(lock.isLocked());
    }
    
    // Guard upgrade to Write
    {
        ReadWriteSpinLock::Guard guard(lock, lock::acquireRead);
        EXPECT_TRUE(lock.isReadLocked());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        EXPECT_TRUE(guard.ownsLock());
        guard.upgradeToWrite();
        EXPECT_TRUE(lock.isWriteLocked());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
        EXPECT_TRUE(guard.ownsLock());
        guard.unlock();
    }
    EXPECT_FALSE(lock.isLocked());
}

TEST(ReadWriteSpinLock, UpgradeLock)
{
    ReadWriteSpinLock lock;
    lock.lockRead();
    lock.lockRead();
    lock.lockRead();
    EXPECT_TRUE(lock.isLocked());
    EXPECT_TRUE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(3, lock.numReaders());
    EXPECT_EQ(0, lock.numPendingWriters());
    std::thread t([&lock]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(2, lock.numReaders());
        lock.unlockRead();
        lock.unlockRead();
    });
    lock.upgradeToWrite();
    EXPECT_EQ(0, lock.numPendingWriters()); //no other pending writers
    EXPECT_TRUE(lock.isWriteLocked());
    lock.unlockWrite();
    EXPECT_FALSE(lock.isLocked());
    EXPECT_FALSE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(0, lock.numReaders());
    t.join();
}

TEST(ReadWriteSpinLock, UpgradeSingleReader)
{
    ReadWriteSpinLock lock;
    lock.lockRead();
    lock.unlockWrite(); //no-op
    EXPECT_TRUE(lock.isReadLocked());
    lock.upgradeToWrite();
    EXPECT_EQ(0, lock.numPendingWriters()); //upgrade was direct to write (no pending)
    EXPECT_TRUE(lock.isWriteLocked());
    lock.unlockRead(); //no-op
    EXPECT_TRUE(lock.isWriteLocked());
    lock.unlockWrite();
    EXPECT_FALSE(lock.isLocked());
    EXPECT_FALSE(lock.isReadLocked());
    EXPECT_FALSE(lock.isWriteLocked());
    EXPECT_EQ(0, lock.numReaders());
}

TEST(ReadWriteSpinLock, TryUpgradeSingleReader)
{
    ReadWriteSpinLock lock;
    lock.lockRead();
    lock.unlockWrite(); //no-op
    EXPECT_TRUE(lock.isReadLocked());
    EXPECT_TRUE(lock.tryUpgradeToWrite());
    EXPECT_TRUE(lock.isWriteLocked());
    lock.unlockWrite();
    EXPECT_FALSE(lock.isLocked());
}

TEST(ReadWriteSpinLock, UpgradeMultipleReaders)
{
    ReadWriteSpinLock lock;
    lock.lockRead();
    lock.lockRead();
    EXPECT_TRUE(lock.isReadLocked());
    EXPECT_FALSE(lock.tryUpgradeToWrite()); //2 readers is impossible
    EXPECT_FALSE(lock.isWriteLocked());
    lock.unlockRead(); //only a single reader left
    EXPECT_EQ(1, lock.numReaders());
    
    //start a bunch of parallel readers which will then upgrade to writers
    std::vector<std::thread> threads;
    auto timeToWake = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
    std::atomic_int count{0};
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&lock, &timeToWake, &count]() {
            lock.lockRead();
            ++count;
            std::this_thread::sleep_until(timeToWake);
            lock.upgradeToWrite();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            EXPECT_GE(lock.numPendingWriters(), 0);
            EXPECT_TRUE(lock.isWriteLocked());
            lock.unlockWrite();
        });
    }
    while (count < 10) {
        //wait for threads to lock read
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    lock.upgradeToWrite(); //this will block waiting for the other threads to finish
    EXPECT_TRUE(lock.isWriteLocked());
    lock.unlockWrite();
    for (auto&& t : threads) {
        t.join();
    }
    EXPECT_EQ(0, lock.numReaders());
    EXPECT_EQ(0, lock.numPendingWriters());
    EXPECT_FALSE(lock.isLocked());
}

TEST(ReadWriteSpinLock, UpgradingBlockedMultipleReaders)
{
    std::vector<int> values;
    ReadWriteSpinLock lock;
    lock.lockRead();
    lock.lockRead();
    
    //start a bunch of parallel readers which will then upgrade to writers
    std::vector<std::thread> threads;
    auto timeToWake = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
    std::atomic_int count{0};
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&lock, &timeToWake, &values, &count, i]() {
            if (i == 9) {
                while (count < 9) {
                    //make sure all other readers are blocked
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                lock.unlockRead(); //unblock the pending writer
            }
            else {
                std::this_thread::sleep_until(timeToWake);
                ++count;
                lock.lockRead(); //this will block until the writer upgrades
                lock.upgradeToWrite();
                values.push_back(i);
                lock.unlockWrite();
            }
        });
    }
    //this will block waiting for the other threads to finish
    lock.upgradeToWrite();
    EXPECT_TRUE(lock.isWriteLocked());
    values.push_back(-1);
    lock.unlockWrite();
    for (auto&& t : threads) {
        t.join();
    }
    EXPECT_EQ(0, lock.numReaders());
    EXPECT_EQ(0, lock.numPendingWriters());
    EXPECT_FALSE(lock.isLocked());
    EXPECT_EQ(-1, values.front());
}

//==============================================================================
//                         READWRITEMUTEX TESTS
//==============================================================================

TEST(ReadWriteMutex, SingleLocks)
{
    ReadWriteMutex mutex;

    EXPECT_FALSE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());

    mutex.lockRead();
    EXPECT_TRUE(mutex.isLocked());
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(1, mutex.numReaders());

    mutex.unlockRead();
    EXPECT_FALSE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());

    mutex.lockWrite();
    EXPECT_TRUE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_TRUE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());

    mutex.unlockWrite();
    EXPECT_FALSE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());

    mutex.lockRead();
    EXPECT_TRUE(mutex.isLocked());
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(1, mutex.numReaders());
    mutex.upgradeToWrite();
    EXPECT_TRUE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_TRUE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());

    mutex.unlockWrite();
    EXPECT_FALSE(mutex.isLocked());
    EXPECT_FALSE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.isWriteLocked());
    EXPECT_EQ(0, mutex.numReaders());
}

TEST(ReadWriteMutex, TryLocks)
{
    ReadWriteMutex mutex;

    ASSERT_FALSE(mutex.isLocked());

    EXPECT_TRUE(mutex.tryLockRead());
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.tryLockWrite());
    EXPECT_TRUE(mutex.isReadLocked());

    mutex.unlockRead();

    EXPECT_TRUE(mutex.tryLockWrite());
    EXPECT_TRUE(mutex.isWriteLocked());
    EXPECT_FALSE(mutex.tryLockRead());
    EXPECT_FALSE(mutex.isReadLocked());

    mutex.unlockWrite();

    mutex.lockRead();
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_FALSE(mutex.tryLockWrite());
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_TRUE(mutex.tryUpgradeToWrite());
    EXPECT_TRUE(mutex.isWriteLocked());
    EXPECT_FALSE(mutex.tryLockRead());
    EXPECT_FALSE(mutex.isReadLocked());

}

TEST(ReadWriteMutex, Guards)
{
    ReadWriteMutex mutex;

    ASSERT_FALSE(mutex.isLocked());

    // Guard
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, TryLock
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead, lock::tryToLock);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // WriteGuard
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireWrite);
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // WriteGuard, TryLock
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, WriteGuard, TryLock (fails)
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        ReadWriteMutex::Guard writeGuard(mutex, lock::acquireWrite, lock::tryToLock);
        EXPECT_FALSE(mutex.isWriteLocked());
        EXPECT_FALSE(writeGuard.ownsLock());
        EXPECT_FALSE(writeGuard.ownsReadLock());
        EXPECT_FALSE(writeGuard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, tryLockRead
    {
        mutex.lockWrite();
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead, lock::tryToLock);
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.tryLockRead());
        EXPECT_FALSE(guard.ownsLock());

        mutex.unlockWrite();
        mutex.lockRead();
        EXPECT_FALSE(guard.tryLockWrite());
        EXPECT_FALSE(guard.ownsLock());
        mutex.unlockRead();
    }
    
    // Guard, adoptLock (read)
    {
        mutex.lockRead();
        ReadWriteMutex::Guard guard(mutex, lock::adoptLock);
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, adoptLock (write)
    {
        mutex.lockWrite();
        ReadWriteMutex::Guard guard(mutex, lock::adoptLock);
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());
    
    // Guard, deferLock (read)
    {
        mutex.lockRead();
        ReadWriteMutex::Guard guard(mutex, lock::deferLock);
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        mutex.unlockRead();
        guard.lockRead();
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
    }
    EXPECT_FALSE(mutex.isLocked());
    
    // Guard, deferLock (write)
    {
        mutex.lockWrite();
        ReadWriteMutex::Guard guard(mutex, lock::deferLock);
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        mutex.unlockWrite();
        guard.lockWrite();
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, upgrade
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        guard.upgradeToWrite();
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, tryUpgrade (success)
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.tryUpgradeToWrite());
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
    }

    // Guard, tryUpgrade (failure)
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        mutex.lockRead();
        EXPECT_FALSE(guard.tryUpgradeToWrite());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        mutex.unlockRead();
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, unlock, WriteGuard
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        guard.unlock();
        EXPECT_FALSE(mutex.isLocked());
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        ReadWriteMutex::Guard writeGuard(mutex, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(writeGuard.ownsLock());
        EXPECT_FALSE(writeGuard.ownsReadLock());
        EXPECT_TRUE(writeGuard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard, unlock, WriteGuard, unlock
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        guard.unlock();
        EXPECT_FALSE(mutex.isLocked());
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        ReadWriteMutex::Guard writeGuard(mutex, lock::acquireWrite, lock::tryToLock);
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(writeGuard.ownsLock());
        EXPECT_FALSE(writeGuard.ownsReadLock());
        EXPECT_TRUE(writeGuard.ownsWriteLock());
        writeGuard.unlock();
        EXPECT_FALSE(mutex.isLocked());
        EXPECT_FALSE(writeGuard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_FALSE(mutex.isLocked());

    // Guard (read), release
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_TRUE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
        guard.release();
        EXPECT_TRUE(mutex.isReadLocked());
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_TRUE(mutex.isReadLocked());
    mutex.unlockRead();

    // Guard (write), release
    {
        ReadWriteMutex::Guard guard(mutex, lock::acquireWrite);
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_TRUE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_TRUE(guard.ownsWriteLock());
        guard.release();
        EXPECT_TRUE(mutex.isWriteLocked());
        EXPECT_FALSE(guard.ownsLock());
        EXPECT_FALSE(guard.ownsReadLock());
        EXPECT_FALSE(guard.ownsWriteLock());
    }
    EXPECT_TRUE(mutex.isWriteLocked());
    mutex.unlockWrite();
}

TEST(ReadWriteMutex, MultipleReadLocks) {
    ReadWriteMutex mutex;
    bool run = true;
    
    ASSERT_FALSE(mutex.isLocked());
    
    auto getReadLock = [&]() {
        ReadWriteMutex::Guard guard(mutex, lock::acquireRead);
        while (run) {}
    };
    
    std::thread t1(getReadLock);
    std::thread t2(getReadLock);
    std::thread t3(getReadLock);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_TRUE(mutex.isLocked());
    EXPECT_TRUE(mutex.isReadLocked());
    EXPECT_EQ(3, mutex.numReaders());
    
    run = false;
    
    t1.join();
    t2.join();
    t3.join();
    
    EXPECT_FALSE(mutex.isLocked());
    EXPECT_EQ(0, mutex.numReaders());
}
