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

    /// @brief Accesses the pointer to a coroutine or IO task local variable
    /// @param[in] key The variable name
    /// @return the pointer to the local variable with the same name
    /// @note If the variable with the 'key' name has not been created within the current
    ///       coroutine or IO task, a pointer to it will be allocated, set to null, and a reference to the
    ///       pointer will be returned for reading/writing.
    ///       If the variable with the 'key' name has already been created,
    ///       then a reference to the previously set pointer will be returned.
    /// @note If the function is called outside of a coroutine or IO task, then a default thread-local
    ///       storage will be used with the semantics described above.
    /// @note Upon the termination of the coroutine, the storage occupied by the coro-local-variable
    ///       pointers will be freed. It is up to the user of the API to free the actual variable
    ///       storage.
    template <typename T>
    T*& variable(const std::string& key);
    
    /// @brief Get the current coroutine context
    /// @return The coroutine context if this function is called inside a coroutine or null otherwise
    VoidContextPtr context();
    
    /// @brief Returns the task id of the currently executing coroutine or IO task
    /// @return The task id
    TaskId taskId();

}}}

#include <quantum/impl/quantum_local_impl.h>

#endif //BLOOMBERG_QUANTUM_LOCAL_H
