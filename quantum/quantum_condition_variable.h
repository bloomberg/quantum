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
#ifndef BLOOMBERG_QUANTUM_CONDITION_VARIABLE_H
#define BLOOMBERG_QUANTUM_CONDITION_VARIABLE_H

#include <list>
#include <atomic>
#include <quantum/quantum_mutex.h>
#include <quantum/quantum_yielding_thread.h>
#include <quantum/interface/quantum_icontext.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                   class ConditionVariable
//==============================================================================================
/// @class ConditionVariable
/// @brief This class represents a coroutine-compatible implementation of the std::condition_variable.
///        Most methods of the latter have been recreated with the same behavior. This object will yield
///        instead of blocking if called from a coroutine.
class ConditionVariable
{
public:
    /// @brief Default constructor.
    ConditionVariable();
    
    /// @warning Copy constructor is explicitly deleted.
    ConditionVariable(const ConditionVariable& other) = delete;
    
    /// @warning Move constructor is explicitly deleted.
    ConditionVariable(ConditionVariable&& other) = delete;
    
    /// @warning Assignment operator is explicitly deleted.
    ConditionVariable& operator=(const ConditionVariable& other) = delete;
    
    /// @warning Move assignment operator is explicitly deleted.
    ConditionVariable& operator=(ConditionVariable&& other) = delete;
    
    /// @brief Destructor.
    /// @warning Deleting this object while there are still waiting threads results in undefined behavior. Ensure that
    ///          all threads have been notified before doing so. Once the destructor is called, no other wait attempts
    ///          should be made on this object.
    ~ConditionVariable();
    
    /// @brief Notify one waiting thread or coroutine.
    void notifyOne();
    
    /// @brief Notify one waiting thread or coroutine.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    void notifyOne(ICoroSync::Ptr sync);
    
    /// @brief Notify all waiting threads and coroutines.
    void notifyAll();
    
    /// @brief Notify all waiting threads and coroutines.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    void notifyAll(ICoroSync::Ptr sync);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll().
    /// @details When this function returns, the mutex is guaranteed to be locked.
    /// @param[in] mutex Mutex object which is locked by the current thread.
    /// @note This function should be called from a regular thread not from a coroutine.
    void wait(Mutex& mutex);
    
    /// @brief Yield the current coroutine until the condition is signalled via notifyOne() or notifyAll().
    /// @details When this function returns, the mutex is guaranteed to be locked.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @note This function should be called from a coroutine.
    void wait(ICoroSync::Ptr sync,
              Mutex& mutex);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll().
    /// @details When this function returns, the mutex is guaranteed to be locked. This function calls wait() in a loop
    ///          until the predicate returns true. This ensures that the condition has not changed after notifyOne()
    ///          or notifyAll() have been called. The internal logic is equivalent to:
    /// @code
    ///     while(!predicate())
    ///     {
    ///         wait(mutex);
    ///     }
    /// @endcode
    /// @tparam PREDICATE Callable function type having the following signature 'bool f()'.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] predicate Function or functor to be tested as exit condition of the endless while loop.
    /// @note This function should be called from a regular thread not from a coroutine.
    template <class PREDICATE = bool()>
    void wait(Mutex& mutex,
              PREDICATE predicate);
    
    /// @brief Yield the current coroutine until the condition is signalled via notifyOne() or notifyAll(). When this
    ///        function returns, the mutex is guaranteed to be locked.
    /// @details When this function returns, the mutex is guaranteed to be locked. This function calls wait() in a loop
    ///          until the predicate returns true. This ensures that the condition has not changed after notifyOne()
    ///          or notifyAll() have been called. The internal logic is equivalent to:
    /// @code
    ///     while(!predicate())
    ///     {
    ///         wait(mutex);
    ///     }
    /// @endcode
    /// @tparam PREDICATE Callable function type having the following signature 'bool f()'.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] predicate Function or functor to be tested as exit condition of the endless while loop.
    /// @note This function should be called from a coroutine.
    template <class PREDICATE = bool()>
    void wait(ICoroSync::Ptr sync,
              Mutex& mutex,
              PREDICATE predicate);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll() or until 'time'
    ///        duration expires.
    /// @details When this function returns, the mutex is guaranteed to be locked.
    /// @tparam REP An arithmetic type such as int or double representing the number of ticks.
    /// @tparam PERIOD A std::ratio representing the tick period such as ticks per second.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] time Maximum duration for which to wait on this condition.
    /// @return True if the mutex was acquired before 'time' expired or false otherwise.
    /// @note This function should be called from a regular thread not from a coroutine.
    template <class REP, class PERIOD>
    bool waitFor(Mutex& mutex,
                 const std::chrono::duration<REP, PERIOD>& time);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll() or until 'time'
    ///        duration expires.
    /// @details When this function returns, the mutex is guaranteed to be locked.
    /// @tparam REP An arithmetic type such as int or double representing the number of ticks.
    /// @tparam PERIOD A std::ratio representing the tick period such as ticks per second.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] time Maximum duration for which to wait on this condition.
    /// @return True if the mutex was acquired before 'time' expired or false otherwise.
    /// @note This function should be called from a coroutine.
    template <class REP, class PERIOD>
    bool waitFor(ICoroSync::Ptr sync,
                 Mutex& mutex,
                 const std::chrono::duration<REP, PERIOD>& time);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll() or until 'time'
    ///        duration expires.
    /// @details When this function returns, the mutex is guaranteed to be locked. This function calls
    ///          wait() in a loop until the predicate returns true or 'time' expires. This ensures that the condition
    ///          has not changed after notifyOne() or notifyAll() have been called. The internal logic is equivalent to:
    /// @code
    ///     while(!predicate())
    ///     {
    ///         waitFor(mutex, time);
    ///     }
    /// @endcode
    /// @tparam REP An arithmetic type such as int or double representing the number of ticks.
    /// @tparam PERIOD A std::ratio representing the tick period such as ticks per second.
    /// @tparam PREDICATE Callable function type having the following signature 'bool f()'.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] time Maximum duration for which to wait on this condition.
    /// @param[in] predicate Function or functor to be tested as exit condition of the endless while loop.
    /// @return True if the mutex was acquired before 'time' expired, otherwise the predicate result after timeout.
    /// @note This function should be called from a regular thread not from a coroutine.
    template <class REP, class PERIOD, class PREDICATE = bool()>
    bool waitFor(Mutex& mutex,
                 const std::chrono::duration<REP, PERIOD>& time,
                 PREDICATE predicate);
    
    /// @brief Block the current thread until the condition is signalled via notifyOne() or notifyAll() or until 'time'
    ///        duration expires.
    /// @details When this function returns, the mutex is guaranteed to be locked. This function calls
    ///          wait() in a loop until the predicate returns true or 'time' expires. This ensures that the condition
    ///          has not changed after notifyOne() or notifyAll() have been called. The internal logic is equivalent to:
    /// @code
    ///     while(!predicate())
    ///     {
    ///         waitFor(mutex, time);
    ///     }
    /// @endcode
    /// @tparam REP An arithmetic type such as int or double representing the number of ticks.
    /// @tparam PERIOD A std::ratio representing the tick period such as ticks per second.
    /// @tparam PREDICATE Callable function type having the following signature 'bool f()'.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    /// @param[in] mutex Mutex object which is locked by the current coroutine.
    /// @param[in] time Maximum duration for which to wait on this condition.
    /// @param[in] predicate Function or functor to be tested as exit condition of the endless while loop.
    /// @return True if the mutex was acquired before 'time' expired, otherwise the predicate result after timeout.
    /// @note This function should be called from a coroutine.
    template <class REP, class PERIOD, class PREDICATE = bool()>
    bool waitFor(ICoroSync::Ptr sync,
                 Mutex& mutex,
                 const std::chrono::duration<REP, PERIOD>& time,
                 PREDICATE predicate);
    
private:
    void notifyOneImpl(ICoroSync::Ptr sync);
    
    void notifyAllImpl(ICoroSync::Ptr sync);
    
    void waitImpl(ICoroSync::Ptr sync,
                  Mutex& mutex);
    
    template <class PREDICATE = bool()>
    void waitImpl(ICoroSync::Ptr sync,
                  Mutex& mutex,
                  PREDICATE predicate);
    
    template <class REP, class PERIOD>
    bool waitForImpl(ICoroSync::Ptr sync,
                     Mutex& mutex,
                     std::chrono::duration<REP, PERIOD>& time);
    
    template <class REP, class PERIOD, class PREDICATE = bool()>
    bool waitForImpl(ICoroSync::Ptr sync,
                     Mutex& mutex,
                     const std::chrono::duration<REP, PERIOD>& time,
                     PREDICATE predicate);
    
    //MEMBERS
    Mutex                           _thisLock; //sync access to this object
    std::list<std::atomic_int*>     _waiters;
    std::atomic_bool                _destroyed;
};

}}

#include <quantum/impl/quantum_condition_variable_impl.h>

#endif //BLOOMBERG_QUANTUM_CONDITION_VARIABLE_H
