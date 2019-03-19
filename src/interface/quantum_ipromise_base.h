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
#ifndef BLOOMBERG_QUANTUM_IPROMISE_BASE_H
#define BLOOMBERG_QUANTUM_IPROMISE_BASE_H

#include <exception>
#include <quantum/interface/quantum_iterminate.h>
#include <quantum/interface/quantum_ifuture.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                interface IPromiseBase
//==============================================================================================
/// @interface IPromiseBase.
/// @brief Exposes methods to manipulate and access a promise.
struct IPromiseBase : public ITerminate
{
    using Ptr = std::shared_ptr<IPromiseBase>;
    
    /// @brief Determines if this promise still has a shared state with the corresponding future object.
    /// @return True if valid, false otherwise.
    virtual bool valid() const = 0;
    
    /// @brief Set an exception in this promise.
    /// @details When setting an exception inside a promise, any attempt to read a value from the associated future
    ///          will re-throw this exception. Any threads or coroutines already blocked on this future will
    ///          immediately unblock and re-throw.
    /// @param[in] ex An exception pointer which has been caught via std::current_exception.
    /// @return 0 on success
    virtual int setException(std::exception_ptr ex) = 0;
    
    /// @brief Get a thread-compatible interface used to access the associated future.
    /// @return An interface to the associated future.
    virtual IThreadFutureBase::Ptr getIThreadFutureBase() const = 0;
    
    /// @brief Get a coroutine-compatible interface used to access the associated future.
    /// @return An interface to the associated future.
    virtual ICoroFutureBase::Ptr getICoroFutureBase() const = 0;
};

using IPromiseBasePtr = IPromiseBase::Ptr;

}}

#endif //BLOOMBERG_QUANTUM_IPROMISE_BASE_H
