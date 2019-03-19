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
#ifndef BLOOMBERG_QUANTUM_THREAD_TRAITS_H
#define BLOOMBERG_QUANTUM_THREAD_TRAITS_H

#include <cstdint>
#include <chrono>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 struct ThreadTraits
//==============================================================================================
/// @struct ThreadTraits.
/// @brief Allows application-wide settings for the various thread settings used by Quantum.
struct ThreadTraits
{
    /// @brief Dictates how long any thread should sleep on blocking calls when interacting
    ///        with coroutines (e.g. mutexes, condition variables, etc).
    /// @return The modifiable sleep interval in microseconds.
    /// @note: When set to 0, threads will yield() instead of sleeping which results in
    ///        increased performance at the detriment of higher CPU load. Default is 0ms.
    static std::chrono::milliseconds& yieldSleepIntervalMs()
    {
        static std::chrono::milliseconds value(0);
        return value;
    }
    static std::chrono::microseconds& yieldSleepIntervalUs()
    {
        static std::chrono::microseconds value(10);
        return value;
    }
};

}
}


#endif //BLOOMBERG_QUANTUM_THREAD_TRAITS_H
