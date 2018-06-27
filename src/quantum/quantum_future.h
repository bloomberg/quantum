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
#ifndef QUANTUM_FUTURE_H
#define QUANTUM_FUTURE_H

#include <exception>
#include <quantum/quantum_shared_state.h>
#include <quantum/interface/quantum_icontext.h>
#include <quantum/interface/quantum_ifuture.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                   class Future
//==============================================================================================
/// @class Future
/// @brief Class representing a promised future. Can only be instantiated via a Promise object.
/// @tparam T Type of value returned by the future object.
/// @note See IThreadFuture and ICoroFuture interfaces for usage details. An instance of this class
///       can only be obtained via a Promise object and cannot be created on its own.
template <class T>
class Future : public IThreadFuture<T>,
               public ICoroFuture<T>
{
public:
    template <class F> friend class Promise;
    using ptr = std::shared_ptr<Future<T>>;
    
    //Default constructor with empty state
    Future() = default;
    
    bool valid() const final;
    
    //IThreadFutureBase
    void wait() const final;
    std::future_status waitFor(size_t timeMs) const final;
    
    //IThreadFuture
    T get() final;
    const T& getRef() const final;
    
    template <class BUF = T, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    V pull(bool& isBufferClosed);
    
    //ICoroFutureBase
    void wait(ICoroSync::ptr sync) const final;
    std::future_status waitFor(ICoroSync::ptr sync, size_t timeMs) const final;
    
    //ICoroFuture
    T get(ICoroSync::ptr sync) final;
    const T& getRef(ICoroSync::ptr sync) const final;
    
    template <class BUF = T, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::value_type>
    V pull(ICoroSync::ptr sync, bool& isBufferClosed);
    
private:
    explicit Future(std::shared_ptr<SharedState<T>> sharedState);
    
    //Members
    std::shared_ptr<SharedState<T>>     _sharedState;
};

}}

#include <quantum/impl/quantum_future_impl.h>

#endif //QUANTUM_FUTURE_H
