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
#ifndef BLOOMBERG_QUANTUM_PROMISE_H
#define BLOOMBERG_QUANTUM_PROMISE_H

#include <quantum/quantum_future.h>
#include <quantum/interface/quantum_ipromise.h>

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
    
    //Move constructor
    Promise(Promise<T>&& other) :
        IThreadPromise<Promise, T>(this),
        ICoroPromise<Promise, T>(this),
        _sharedState(std::move(other._sharedState)),
        _terminated(other._terminated.load()){
    }
    
    //Move assignment
    Promise& operator=(Promise<T>&& other)
    {
        if (this != &other) {
            _sharedState = std::move(other._sharedState);
            _terminated = other._terminated.load();
        }
        return *this;
    }
    
    //Destructor
    ~Promise();
    
    //Future interface accessors
    IThreadFutureBase::Ptr getIThreadFutureBase() const final;
    ICoroFutureBase::Ptr getICoroFutureBase() const final;
    ThreadFuturePtr<T> getIThreadFuture() const;
    CoroFuturePtr<T> getICoroFuture() const;
    
    //ITerminate
    void terminate() final;
    
    //IPromiseBase
    bool valid() const final;
    int setException(std::exception_ptr ex) final;
    
    //IThreadPromise
    template <class V, class = NonBufferType<T,V>>
    int set(V&& value);
    
    template <class V, class = BufferType<T,V>>
    void push(V&& value);
    
    //ICoroPromise
    template <class V, class = NonBufferType<T,V>>
    int set(ICoroSync::Ptr sync, V&& value);
    
    template <class V, class = BufferType<T,V>>
    void push(ICoroSync::Ptr sync, V&& value);
    
    template <class V = T, class = BufferRetType<V>>
    int closeBuffer();
    
    //===================================
    //           NEW / DELETE
    //===================================
    static void* operator new(size_t size);
    static void operator delete(void* p);
    static void deleter(Promise<T>* p);
    
private:
    std::shared_ptr<SharedState<T>> _sharedState;
    std::atomic_bool                _terminated;
};

template <class T>
using PromisePtr = typename Promise<T>::Ptr;

}}

#include <quantum/impl/quantum_promise_impl.h>

#endif //BLOOMBERG_QUANTUM_PROMISE_H
