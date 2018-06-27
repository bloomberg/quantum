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
#ifndef QUANTUM_PROMISE_H
#define QUANTUM_PROMISE_H

#include <quantum/quantum_future.h>
#include <quantum/interface/quantum_ipromise.h>
#include <quantum/quantum_util.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                class Promise
//==============================================================================================
/// @class Promise
/// @brief Class representing a promised value.
/// @tparam T Type of value returned by the promise.
/// @note See IPromiseBase, IThreadPromise and ICoroPromise interfaces for usage details.
template <class T>
class Promise : public IPromiseBase,
                public IThreadPromise<Promise, T>,
                public ICoroPromise<Promise, T>
{
public:
    using Ptr = std::shared_ptr<Promise<T>>;
    
    //Constructor
    Promise();
    
    //Destructor
    ~Promise();
    
    //Future interface accessors
    IThreadFutureBase::Ptr getIThreadFutureBase() const final;
    ICoroFutureBase::Ptr getICoroFutureBase() const final;
    typename IThreadFuture<T>::Ptr getIThreadFuture() const;
    typename ICoroFuture<T>::Ptr getICoroFuture() const;
    
    //ITerminate
    void terminate() final;
    
    //IPromiseBase
    bool valid() const final;
    int setException(std::exception_ptr ex) final;
    
    //IThreadPromise
    template <class V = T>
    int set(V&& value);
    
    template <class BUF = T, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::ValueType>
    void push(V &&value);
    
    //ICoroPromise
    template <class V = T>
    int set(ICoroSync::Ptr sync, V&& value);
    
    template <class BUF = T, class V = typename std::enable_if_t<Traits::IsBuffer<BUF>::value, BUF>::ValueType>
    void push(ICoroSync::Ptr sync, V &&value);
    
    template <class BUF = T, class = std::enable_if_t<Traits::IsBuffer<BUF>::value>>
    int closeBuffer();
    
    //===================================
    //           NEW / DELETE
    //===================================
    static void* operator new(size_t size);
    static void operator delete(void* p);
    static void deleter(Promise<T>* p);
    
private:
    std::shared_ptr<SharedState<T>> _sharedState;
    std::atomic_flag                _terminated;
};

}}

#include <quantum/impl/quantum_promise_impl.h>

#endif //QUANTUM_PROMISE_H
