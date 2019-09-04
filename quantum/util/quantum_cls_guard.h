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
#ifndef BLOOMBERG_QUANTUM_CLS_GUARD_H
#define BLOOMBERG_QUANTUM_CLS_GUARD_H

#include <string>

namespace Bloomberg {
namespace quantum {
namespace cls {

//==============================================================================================
//                                class Guard
//==============================================================================================
/// @class Guard
/// @brief Class a guard for a coro-local-storage variable. Saves a coro-local-storage upon
/// construction, and recovers it to the previous value upon destruction.
/// @tparam T Type of variable
template<typename T>
class Guard
{    
public:
    // @brief Constructs an instance of guard saving a value into a coro-local-storage variable.
    // @param[in] key variable name
    // @param[in] value value to be saved to the variable with the name @see key
    Guard(const std::string& key, T* value);
    // @brief Destroys the guard instance recoving the previous variable value
    ~Guard();

    Guard() = delete;
    Guard(const Guard&) = delete;
    Guard(Guard&&) = delete;
    
    Guard& operator = (const Guard&) = delete;
    Guard& operator = (Guard&&) = delete;

private:
    T*& _storage;       // storage for the variable
    T*  _prev;          // previous variable value
};
    
}}}

#include <quantum/util/impl/quantum_cls_guard_impl.h>

#endif //BLOOMBERG_QUANTUM_CLS_GUARD_H
