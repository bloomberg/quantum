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
#ifndef QUANTUM_FUTURE_JOINER_H
#define QUANTUM_FUTURE_JOINER_H

#include <quantum/interface/quantum_ithread_context.h>
#include <quantum/interface/quantum_ithread_future.h>
#include <quantum/interface/quantum_icoro_context.h>
#include <quantum/interface/quantum_icoro_future.h>
#include <quantum/quantum_dispatcher.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                      class FutureJoiner
//==============================================================================================
/// @class FutureJoiner
/// @brief Utility class that joins N futures into a single one.
/// @details Instead of waiting for N futures to complete, the user can join them and wait
///          on a single future which returns N values.
/// @tparam T The type returned by the future.
template <class DISPATCHER>
class FutureJoiner
{
public:
    /// @brief Construct a joiner with a Dispatcher object.
    /// @param[in] dispatcher The address of a Dispatcher object.
    /// @note Use this constructor when joining N thread futures.
    explicit FutureJoiner(DISPATCHER& dispatcher);
    
    /// @brief Join N thread futures.
    /// @param[in] futures A vector of thread futures.
    /// @return A joined future to wait on.
    /// @note Call this from a thread context only.
    template <class T>
    ThreadFuturePtr<std::vector<T>> operator()(std::vector<ThreadContextPtr<T>>&& futures);
    template <class T>
    ThreadFuturePtr<std::vector<T>> join(std::vector<ThreadContextPtr<T>>&& futures);
    
    /// @brief Join N thread futures.
    /// @param[in] futures A vector of async IO futures.
    /// @return A joined future to wait on.
    /// @note Call this from a thread context only.
    template <class T>
    ThreadFuturePtr<std::vector<T>> operator()(std::vector<ThreadFuturePtr<T>>&& futures);
    template <class T>
    ThreadFuturePtr<std::vector<T>> join(std::vector<ThreadFuturePtr<T>>&& futures);
    
    /// @brief Join N coroutine futures.
    /// @param[in] futures A vector of coroutine futures.
    /// @return A joined future to wait on.
    /// @note Call this from a coroutine context only.
    template <class T>
    CoroContextPtr<std::vector<T>> operator()(std::vector<CoroContextPtr<T>>&& futures);
    template <class T>
    CoroContextPtr<std::vector<T>> join(std::vector<CoroContextPtr<T>>&& futures);
    
    /// @brief Join N coroutine futures.
    /// @param[in] futures A vector of coroutine async IO futures.
    /// @return A joined future to wait on.
    /// @note Call this from a coroutine context only.
    template <class T>
    CoroContextPtr<std::vector<T>> operator()(std::vector<CoroFuturePtr<T>>&& futures);
    template <class T>
    CoroContextPtr<std::vector<T>> join(std::vector<CoroFuturePtr<T>>&& futures);
    
private:
    template <template<class> class FUTURE, class T>
    ThreadFuturePtr<std::vector<T>> join(ThreadContextTag, std::vector<typename FUTURE<T>::Ptr>&& futures);
    
    template <template<class> class FUTURE, class T>
    typename FUTURE<std::vector<T>>::Ptr join(CoroContextTag, std::vector<typename FUTURE<T>::Ptr>&& futures);
    
    DISPATCHER& _dispatcher;
};

}}

#include <quantum/impl/quantum_future_joiner_impl.h>

#endif //QUANTUM_FUTURE_JOINER_H
