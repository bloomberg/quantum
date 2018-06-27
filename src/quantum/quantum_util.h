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
#ifndef QUANTUM_UTIL_H
#define QUANTUM_UTIL_H

#include <tuple>
#include <functional>
#include <utility>
#include <iostream>
#include <quantum/quantum_traits.h>
#include <quantum/interface/quantum_itask.h>
#include <quantum/interface/quantum_icontext.h>
#include <quantum/interface/quantum_ipromise.h>

namespace Bloomberg {
namespace quantum {

//fwd declarations
template <class RET> class Context;
template <class RET> class Promise;

//==============================================================================================
//                                 struct Util
//==============================================================================================
/// @struct Util.
/// @brief Utility to bind a user callable function unto a coroutine or an IO task.
/// @note For internal use only.
struct Util
{
    template<class RET, class FUNC, class ...ARGS>
    static constexpr std::function<void(Traits::yield_t& yield)>
    BindCaller(std::shared_ptr<Context<RET>> ctx, FUNC&& func0, ARGS&& ...args0);
    
    template<class RET, class FUNC, class ...ARGS>
    static constexpr std::function<int()>
    BindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func0, ARGS&& ...args0);
    
#ifdef _QUANTUM_PRINT_DEBUG_
    //Synchronize logging
    static std::mutex& LogMutex();
#endif
};

}}

#include <quantum/impl/quantum_util_impl.h>

#endif //QUANTUM_UTIL_H
