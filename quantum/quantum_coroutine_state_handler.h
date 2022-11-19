/*
** Copyright 2022 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_QUANTUM_STATE_HANDLER_H
#define BLOOMBERG_QUANTUM_STATE_HANDLER_H

#include <functional>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                       enum CoroutineState
//==============================================================================================
/// @enum CoroutineState
/// @brief Defines the states of the coroutine.
enum class CoroutineState : int {
    Constructed,    ///< Before the first time execution
    Resumed,        ///< Before resuming
    Suspended,      ///< After suspension
    Destructed      ///< After the last execution
};

/// @brief Callback for handling coroutine states
/// @param state coroutine state
using CoroutineStateHandler = std::function<void(CoroutineState state)>;

}}

#endif //BLOOMBERG_QUANTUM_STATE_HANDLER_H
