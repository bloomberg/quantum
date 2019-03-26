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
#ifndef BLOOMBERG_QUANTUM_ICONTEXT_BASE_H
#define BLOOMBERG_QUANTUM_ICONTEXT_BASE_H

#include <exception>
#include <memory>

namespace Bloomberg {
namespace quantum {

struct CoroContextTag{};
struct ThreadContextTag{};

//==============================================================================================
//                                  interface IContextBase
//==============================================================================================
/// @interface IContextBase
/// @brief This interface exposes shared functionality between IThreadContext and ICoroContext.
struct IContextBase
{
    using Ptr = std::shared_ptr<IContextBase>;
    
    /// @brief Virtual destructor.
    virtual ~IContextBase() = default;
    
    /// @brief Determines if the future object associated with this context has a valid shared state with the
    ///        corresponding promise.
    /// @return True if valid, false otherwise.
    virtual bool valid() const = 0;
    
    /// @brief Determines if the future object associated with the 'num'-th continuation context is still valid.
    /// @details When using coroutine continuations, any future in the continuation chain can be validated
    ///          using this function. Allowed range for num is [-1, total_continuations). -1 is equivalent
    ///          of calling valid() or validAt(total_continuations-1) on the last context in the chain
    ///          (i.e. the context which is returned via IThreadContext::end() or ICoroContext::end()).
    ///          Position 0 represents the first future in the chain.
    /// @param[in] num The number indicating which future to validate.
    /// @return True if valid, false otherwise.
    virtual bool validAt(int num) const = 0;
    
    /// @brief Set an exception in the promise associated with the current IThreadContext or ICoroContext.
    /// @details When setting an exception inside a promise, any attempt to read a value from the associated future
    ///          will re-throw this exception. Any threads or coroutines already blocked on this future will
    ///          immediately unblock and re-throw.
    /// @param[in] ex An exception pointer which has been caught via std::current_exception.
    /// @return 0 on success
    virtual int setException(std::exception_ptr ex) = 0;
};

using IContextBasePtr = IContextBase::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_ICONTEXT_BASE_H
