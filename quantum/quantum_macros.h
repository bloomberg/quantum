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
#ifndef BLOOMBERG_QUANTUM_MACROS_H
#define BLOOMBERG_QUANTUM_MACROS_H

#if defined(__GNUC__) || defined(__clang__)
    #define QUANTUM_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define QUANTUM_DEPRECATED __declspec(deprecated)
#else
    #define QUANTUM_DEPRECATED
#endif

#define QUANTUM_BACKOFF_LINEAR 0
#define QUANTUM_BACKOFF_EXPONENTIAL 1
#define QUANTUM_BACKOFF_EQUALSTEP 2
#define QUANTUM_BACKOFF_RANDOM 3

#endif //BLOOMBERG_QUANTUM_MACROS_H
