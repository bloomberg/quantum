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
#ifndef BLOOMBERG_QUANTUM_AUXILIARY_H
#define BLOOMBERG_QUANTUM_AUXILIARY_H

namespace Bloomberg {
namespace quantum {

/// @brief Function that checks if intersection exists between two bit fields
/// @tparam BitField Type of the bit field.
/// @param[in] lhs bit field.
/// @param[in] func bit field.
/// @note For internal use only.
/// @note BitField type must be convertible to int.
/// @return true if the intersection exists, false otherwise
template <typename BitField>
bool isIntersection(BitField lhs, BitField rhs);

/// @brief Function that returnes an exception safe version of the given function
/// @tparam Args Argument types passed to FUNC.
/// @param[in] func function which will become exception safe.
/// @note For internal use only.
/// @return wrapped exception safe function if exists, empty functor otherwise
template <typename... Args>
std::function<void(Args...)> makeExceptionSafe(const std::function<void(Args...)>& func);

}}

#include <quantum/impl/quantum_auxiliary_impl.h>

#endif //BLOOMBERG_QUANTUM_AUXILIARY_H
