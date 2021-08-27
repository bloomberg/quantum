/*
** Copyright 2021 Bloomberg Finance L.P.
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

#ifndef BLOOMBERG_QUANTUM_PERF_UTILS_H
#define BLOOMBERG_QUANTUM_PERF_UTILS_H

namespace Bloomberg {
namespace quantum {

struct ProcStats
{
    double _kernelModeTime = 0.0;   // time spent in kernel mode
    double _userModeTime = 0.0;     // time spent in user mode
};

ProcStats operator-(const ProcStats& s1, const ProcStats& s2);

ProcStats getProcStats();

}}

#endif // BLOOMBERG_QUANTUM_PERF_UTILS_H
