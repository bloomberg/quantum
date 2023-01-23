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

#include <quantum_perf_utils.h>

#ifdef __linux__
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#endif // __linux__

namespace Bloomberg {
namespace quantum {

ProcStats getProcStats()
{
#ifdef __linux__
    std::ostringstream buf;
    buf << "/proc/" << getpid() << "/stat";
    std::ifstream file(buf.str());
    if (not file)
    {
        return ProcStats();
    }

    std::string stoken;
    for(int i = 0; i < 13; ++i)
    {
        if (not (file >> stoken))
        {
            return ProcStats();
        }
    }
    // token 14: utime
    // token 15: stime
    ProcStats stats;
    if (not (file >> stats._userModeTime) or
        not (file >> stats._kernelModeTime))
    {
        return ProcStats();
    }
    return stats;
#else
    return ProcStats();
#endif // __linux__
}

ProcStats operator- (const ProcStats& s1, const ProcStats& s2)
{
    ProcStats s;
    s._kernelModeTime = s1._kernelModeTime - s2._kernelModeTime;
    s._userModeTime = s1._userModeTime - s2._userModeTime;
    return s;
}


Timer::Timer()
: _start(std::chrono::steady_clock::now())
{
    _elapsed = std::chrono::steady_clock::duration::zero();
}

Timer::~Timer()
{
    _elapsed = std::chrono::steady_clock::now() - _start;
}

std::chrono::steady_clock::duration Timer::_elapsed;

}}
