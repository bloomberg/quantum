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

//==============================================================================================
//                                   class Capture
//==============================================================================================
template <typename RET, typename FUNC, typename ... ARGS>
template <typename F, typename ... T>
Capture<RET,FUNC,ARGS...>::Capture(F&& func, T&&...args) :
    _func(std::forward<F>(func)),
    _args(std::forward<T>(args)...) //pack
{}

template <typename RET, typename FUNC, typename ... ARGS>
template <typename ... T>
RET Capture<RET,FUNC,ARGS...>::operator()(T&&...t) {
     return apply<RET>(_func, std::move(_args), std::forward<T>(t)...); //fwd
}

template <typename RET, typename FUNC, typename ... ARGS>
Capture<RET,FUNC,ARGS...>
makeCapture(FUNC&& func, ARGS&& ... args)
{
    return Capture<RET,FUNC,ARGS...>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

//==============================================================================================
//                                   class Function
//==============================================================================================

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::Function(RET(*ptr)(ARGS...)) :
    _callable(reinterpret_cast<void*>(ptr))
{
    _invoker = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<Func>(ptr))(std::forward<ARGS>(args)...);
    };
}

template <typename RET, typename ... ARGS>
template <typename FUNCTOR>
Function<RET(ARGS...)>::Function(FUNCTOR&& functor)
{
    initFunctor(std::forward<FUNCTOR>(functor), std::is_lvalue_reference<FUNCTOR>());
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::Function(Function<RET(ARGS...)>&& other)
{
    *this = std::move(other);
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>&
Function<RET(ARGS...)>::operator=(Function<RET(ARGS...)>&& other)
{
    if (this != &other)
    {
        this->~Function(); //delete current
        _invoker = other._invoker;
        _destructor = other._destructor;
        _deleter = other._deleter;
        if (other._callable == other._storage.data()) {
            //copy byte-wise data
            //Note that in this case the destructor will be called on a seemingly different object than the constructor
            //however this is valid.
            _storage = other._storage;
            _callable = _storage.data();
        }
        else {
            _callable = other._callable; //steal buffer
        }
        other._callable = nullptr; //disable other callable
    }
    return *this;
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::~Function()
{
    if (_destructor) _destructor(_callable);
    if (_deleter) _deleter(_callable);
}

template <typename RET, typename ... ARGS>
RET Function<RET(ARGS...)>::operator()(ARGS...args) {
    return _invoker(_callable, std::forward<ARGS>(args)...);
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::operator bool() const {
    return !!_callable;
}

template <typename RET, typename ... ARGS>
template <typename FUNCTOR>
void Function<RET(ARGS...)>::initFunctor(FUNCTOR&& functor, std::true_type)
{
    _callable = std::addressof(functor);
    _invoker = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<FUNCTOR*>(ptr))(std::forward<ARGS>(args)...);
    };
}

template <typename RET, typename ... ARGS>
template <typename FUNCTOR>
void Function<RET(ARGS...)>::initFunctor(FUNCTOR&& functor, std::false_type)
{
    _destructor = [](void* ptr){
        if (!ptr) return;
        reinterpret_cast<FUNCTOR*>(ptr)->~FUNCTOR(); //invoke destructor
    };
    if (sizeof(FUNCTOR) <= size) {
        new (_storage.data()) FUNCTOR(std::forward<FUNCTOR>(functor));
        _callable = _storage.data();
    }
    else {
        _callable = new char[sizeof(FUNCTOR)];
        new (_callable) FUNCTOR(std::forward<FUNCTOR>(functor));
        _deleter = deleter;
    }
    _invoker = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<FUNCTOR*>(ptr))(std::forward<ARGS>(args)...);
    };
}

}
}
