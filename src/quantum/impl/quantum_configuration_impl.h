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
void Configuration::setNumCoroutineThreads(int num)
{
    _numCoroutineThreads = num;
}

inline
void Configuration::setNumIoThreads(int num)
{
    _numIoThreads = num;
}

inline
void Configuration::setPinCoroutineThreadsToCores(bool value)
{
    _pinCoroutineThreadsToCores = value;
}

inline
void Configuration::setLoadBalanceSharedIoQueues(bool value)
{
    _loadBalanceSharedIoQueues = value;
}

inline
void Configuration::setLoadBalancePollIntervalMs(std::chrono::milliseconds interval)
{
    _loadBalancePollIntervalMs = interval;
}

inline
void Configuration::setLoadBalancePollIntervalBackoffPolicy(BackoffPolicy policy)
{
    _loadBalancePollIntervalBackoffPolicy = policy;
}

inline
void Configuration::setLoadBalancePollIntervalNumBackoffs(size_t numBackoffs)
{
    _loadBalancePollIntervalNumBackoffs = numBackoffs;
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

}
}
