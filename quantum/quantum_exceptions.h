/*
** Copyright 2026 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_QUANTUM_EXCEPTIONS_H
#define BLOOMBERG_QUANTUM_EXCEPTIONS_H

#include <stdexcept>

namespace Bloomberg {
namespace quantum {

class DispatcherDrainingException : public std::runtime_error{
public:
    /// @brief Constructor.
    DispatcherDrainingException() : std::runtime_error{"Posting is disabled"}{}

    using std::runtime_error::what;
};

class SequencerDrainingException : public std::runtime_error{
public:
    /// @brief Constructor.
    SequencerDrainingException() : std::runtime_error{"Sequencer is disabled"}{}

    using std::runtime_error::what;
};

}}

#endif //BLOOMBERG_QUANTUM_EXCEPTIONS_H
