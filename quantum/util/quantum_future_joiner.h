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
#ifndef BLOOMBERG_QUANTUM_FUTURE_JOINER_H
#define BLOOMBERG_QUANTUM_FUTURE_JOINER_H

#include <quantum/interface/quantum_ithread_context.h>
#include <quantum/interface/quantum_ithread_future.h>
#include <quantum/interface/quantum_icoro_context.h>
#include <quantum/interface/quantum_icoro_future.h>
#include <quantum/impl/quantum_stl_impl.h>
#include <vector>
#include <type_traits>

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
template <typename T>
class FutureJoiner
{
public:
    /// @brief Join N thread futures.
    /// @param[in] futures A vector of thread futures of type T.
    /// @return A joined future to wait on.
    template <class DISPATCHER, class = std::enable_if_t<std::is_same<typename DISPATCHER::ContextTag,ThreadContextTag>::value>>
    ThreadFuturePtr<std::vector<T>> operator()(DISPATCHER& dispatcher, std::vector<ThreadContextPtr<T>>&& futures);
    
    template <class DISPATCHER, class = std::enable_if_t<std::is_same<typename DISPATCHER::ContextTag,ThreadContextTag>::value>>
    ThreadFuturePtr<std::vector<T>> operator()(DISPATCHER& dispatcher, std::vector<ThreadFuturePtr<T>>&& futures);
    
    template <class DISPATCHER, class = std::enable_if_t<std::is_same<typename DISPATCHER::ContextTag,CoroContextTag>::value>>
    CoroContextPtr<std::vector<T>> operator()(DISPATCHER& dispatcher, std::vector<CoroContextPtr<T>>&& futures);
    
    template <class DISPATCHER, class = std::enable_if_t<std::is_same<typename DISPATCHER::ContextTag,CoroContextTag>::value>>
    CoroContextPtr<std::vector<T>> operator()(DISPATCHER& dispatcher, std::vector<CoroFuturePtr<T>>&& futures);
    
private:
    template <template<class> class FUTURE, class DISPATCHER>
    ThreadFuturePtr<std::vector<T>> join(ThreadContextTag, DISPATCHER& dispatcher, std::vector<typename FUTURE<T>::Ptr>&& futures);
    
    template <template<class> class FUTURE, class DISPATCHER>
    CoroContextPtr<std::vector<T>> join(CoroContextTag, DISPATCHER& dispatcher, std::vector<typename FUTURE<T>::Ptr>&& futures);
};

}}

#include <quantum/util/impl/quantum_future_joiner_impl.h>

#endif //BLOOMBERG_QUANTUM_FUTURE_JOINER_H
