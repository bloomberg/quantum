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

namespace Bloomberg {
namespace quantum {

inline
const std::string& ConfigurationSchemaProvider::getJsonSchema()
{
    static std::string schema = R"JSON(
    {
        "$schema" : "http://json-schema.org/draft-04/schema#",
        "$id" : "bloomberg:quantum.json",
        "title": "Quantum library settings",
        "type": "object",
        "properties": {
            "numCoroutineThreads": {
                "type": "number",
                "default": -1
            },
            "numIoThreads": {
                "type": "number",
                "default": 5
            },
            "pinToCores": {
                "type": "boolean",
                "default": false
            },
            "loadBalanceSharedIoQueues": {
                "type": "boolean",
                "default": false
            },
            "loadBalancePollIntervalMs": {
                "type": "number",
                "default": 100
            },
            "loadBalancePollIntervalBackoffPolicy": {
                "type": "string",
                "enum": [
                    "exponential",
                    "linear"
                ],
                "default": "linear"
            },
            "loadBalancePollIntervalNumBackoffs": {
                "type": "number",
                "default": 0
            },
            "coroQueueIdRangeForAnyLow": {
                "type": "number",
                "default": -1
            },
            "coroQueueIdRangeForAnyHigh": {
                "type": "number",
                "default": -1
            },
            "coroSharingForAny": {
                "type": "boolean",
                "default": false
            }
        },
        "additionalProperties": false,
        "required": []
    }
    )JSON";
    return schema;
}

inline
const std::string& ConfigurationSchemaProvider::getJsonSchemaUri()
{
    static std::string uri = "bloomberg:quantum.json";
    return uri;
}

inline
Configuration& Configuration::setNumCoroutineThreads(int num)
{
    _numCoroutineThreads = num;
    return *this;
}

inline
Configuration& Configuration::setNumIoThreads(int num)
{
    _numIoThreads = num;
    return *this;
}

inline
Configuration& Configuration::setPinCoroutineThreadsToCores(bool value)
{
    _pinCoroutineThreadsToCores = value;
    return *this;
}

inline
Configuration& Configuration::setLoadBalanceSharedIoQueues(bool value)
{
    _loadBalanceSharedIoQueues = value;
    return *this;
}

inline
Configuration& Configuration::setLoadBalancePollIntervalMs(std::chrono::milliseconds interval)
{
    _loadBalancePollIntervalMs = interval;
    return *this;
}

inline
Configuration& Configuration::setLoadBalancePollIntervalBackoffPolicy(BackoffPolicy policy)
{
    _loadBalancePollIntervalBackoffPolicy = policy;
    return *this;
}

inline
Configuration& Configuration::setLoadBalancePollIntervalNumBackoffs(size_t numBackoffs)
{
    _loadBalancePollIntervalNumBackoffs = numBackoffs;
    return *this;
}

inline
Configuration& Configuration::setCoroQueueIdRangeForAny(const std::pair<int, int>& coroQueueIdRangeForAny)
{
     _coroQueueIdRangeForAny = coroQueueIdRangeForAny;
     return *this;
}

inline
Configuration& Configuration::setCoroutineSharingForAny(bool sharing)
{
     _coroutineSharingForAny = sharing;
     return *this;
}

inline
int Configuration::getNumCoroutineThreads() const
{
    return _numCoroutineThreads;
}

inline
int Configuration::getNumIoThreads() const
{
    return _numIoThreads;
}

inline
bool Configuration::getPinCoroutineThreadsToCores() const
{
    return _pinCoroutineThreadsToCores;
}

inline
bool Configuration::getLoadBalanceSharedIoQueues() const
{
    return _loadBalanceSharedIoQueues;
}

inline
std::chrono::milliseconds Configuration::getLoadBalancePollIntervalMs() const
{
    return _loadBalancePollIntervalMs;
}

inline
Configuration::BackoffPolicy Configuration::getLoadBalancePollIntervalBackoffPolicy() const
{
    return _loadBalancePollIntervalBackoffPolicy;
}

inline
size_t Configuration::getLoadBalancePollIntervalNumBackoffs() const
{
    return _loadBalancePollIntervalNumBackoffs;
}

inline
const std::pair<int, int>& Configuration::getCoroQueueIdRangeForAny() const
{
    return  _coroQueueIdRangeForAny;
}

inline
bool Configuration::getCoroutineSharingForAny() const
{
    return _coroutineSharingForAny;
}
    
}
}
