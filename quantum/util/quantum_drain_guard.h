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
#ifndef BLOOMBERG_QUANTUM_DRAIN_GUARD_H
#define BLOOMBERG_QUANTUM_DRAIN_GUARD_H

namespace Bloomberg {
namespace quantum {

struct DrainGuard
{
    DrainGuard(std::atomic_bool& drain,
               bool reactivate = true) :
        _drain(drain),
        _reactivate(reactivate)
    {
        _drain = true;
    }
    
    ~DrainGuard()
    {
        if (_reactivate)
        {
            _drain = false;
        }
    }
    std::atomic_bool& _drain;
    bool              _reactivate;
};

}
}

#endif //BLOOMBERG_QUANTUM_DRAIN_GUARD_H
