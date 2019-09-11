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
#ifndef BLOOMBERG_QUANTUM_LOCAL_H
#define BLOOMBERG_QUANTUM_LOCAL_H

#include <quantum/quantum_traits.h>
#include <string>

namespace Bloomberg {
namespace quantum {
namespace local {

/// @brief Accesses the pointer to a coro-local-variable
/// @param[in] key the variable name
/// @return the pointer to the coro-local-variable with the name @see key
/// @note If the variable with the name @see key has not been created within the current
///       coroutine, a pointer to it will be allocated, set to nullptr, and a reference to the
///       pointer will be returned here for reading/writing. 
///       If the variable with the name @see key has already been created within the current
///       coroutine, then a reference to the previously set pointer will be returned.
/// @note If the function is called outside of a coroutine, then the thread-local storage is
///       used with the semantics described above.
/// @note Upon the termination of the coroutine, the storage occupied by the coro-local-variable
///       pointers will be freed. It is up to the user of the API to free the actual variable
///       storage.
template <typename T>
T*& variable(const std::string& key);

/// @brief Get the current coroutine context
/// @return The coroutine context if this function is called inside a coroutine or null otherwise
VoidContextPtr context();

}}}

#include <quantum/impl/quantum_local_impl.h>

#endif //BLOOMBERG_QUANTUM_LOCAL_H
