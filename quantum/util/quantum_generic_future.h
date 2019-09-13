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
#ifndef BLOOMBERGLP_QUANTUM_GENERIC_FUTURE_H
#define BLOOMBERGLP_QUANTUM_GENERIC_FUTURE_H

#include <quantum/interface/quantum_ifuture.h>
#include <quantum/interface/quantum_icontext.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                      class GenericFuture
//==============================================================================================
/// @class GenericFuture
/// @brief Utility class that may contain one of four different future types.
/// @details This class can be used to wrap different futures at runtime, when the return type
///          of a function may vary depending on the function being invoked in a coroutine context or not
///          e.g. by using local::context() to detect coroutine presence.
template <typename T>
class GenericFuture : public IThreadFutureBase
{
public:
    enum class Type : int { ThreadContext, ThreadFuture, CoroContext, CoroFuture, Invalid };
    
    /// @brief Constructors/Destructor
    GenericFuture();
    GenericFuture(ThreadContextPtr<T> f);
    GenericFuture(ThreadFuturePtr<T> f);
    GenericFuture(CoroContextPtr<T> f, ICoroSyncPtr sync);
    GenericFuture(CoroFuturePtr<T> f, ICoroSyncPtr sync);
    GenericFuture(GenericFuture&& other);
    GenericFuture& operator=(GenericFuture&& other);
    ~GenericFuture();
    
    //==============================================================================================
    //                                   IThreadFutureBase
    //==============================================================================================
    /// @brief See IThreadFutureBase::valid()
    bool valid() const final;
    
    /// @brief See IThreadFutureBase::wait()
    void wait() const final;
    
    /// @brief See IThreadFutureBase::waitFor(timeMs)
    std::future_status waitFor(std::chrono::milliseconds timeMs) const final;
        
    /// @brief See IThreadFuture::get()
    template <class V = T>
    NonBufferRetType<V> get();
    
    /// @brief See IThreadFuture::getRef()
    template <class V = T>
    const NonBufferRetType<V>& getRef() const;
    
    /// @brief See IThreadFuture::pull()
    template <class V = T>
    BufferRetType<V> pull(bool& isBufferClosed);
    
private:
    Type                    _type;
    union {
        ThreadContextPtr<T> _threadContext;
        ThreadFuturePtr<T>  _threadFuture;
        CoroContextPtr<T>   _coroContext;
        CoroFuturePtr<T>    _coroFuture;
    };
    ICoroSyncPtr            _sync;
};

}}

#include <quantum/util/impl/quantum_generic_future_impl.h>

#endif //BLOOMBERGLP_QUANTUM_GENERIC_FUTURE_H
