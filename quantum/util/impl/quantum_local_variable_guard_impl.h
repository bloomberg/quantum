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
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################

#include <quantum/util/quantum_local_variable_guard.h>

namespace Bloomberg {
namespace quantum {
namespace local {

template<typename T>
VariableGuard<T>::VariableGuard(const std::string& key, T* value) :
    _storage(variable<T>(key)), 
    _prev(_storage)
{
    _storage = value;
}

template<typename T>
VariableGuard<T>::~VariableGuard()
{
    _storage = _prev;
}
    
}}}
