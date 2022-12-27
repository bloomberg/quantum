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
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################

namespace Bloomberg {
namespace quantum {

template <typename BitField>
bool isIntersection(BitField lhs, BitField rhs)
{
    return static_cast<int>(lhs) & static_cast<int>(rhs);
}

template <typename... Args>
std::function<void(Args...)> makeExceptionSafe(const std::function<void(Args...)>& func)
{
    if (not func)
    {
        return func;
    }

    return [func] (auto&&... args) -> decltype(auto)
    {
        try
        {
            func(std::forward<decltype(args)>(args)...);
        }
        catch (const std::exception& ex)
        {
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Function " << typeid(FuncType)::name() << " threw exception: " << ex.what() << std::endl;
#endif
        }
        catch (...)
        {
#ifdef __QUANTUM_PRINT_DEBUG
            std::lock_guard<std::mutex> guard(Util::LogMutex());
            std::cerr << "Function " << typeid(FuncType)::name() << " threw exception" << std::endl;
#endif
        }
    };
}

}}
