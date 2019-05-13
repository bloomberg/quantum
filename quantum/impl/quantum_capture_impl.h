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
Capture<RET,FUNC,ARGS...>::Capture(FUNC&& func, ARGS&&...args) :
    _func(std::forward<FUNC>(func)),
    _args(std::forward<ARGS>(args)...) //pack
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
    _callable(reinterpret_cast<void*>(ptr)),
    _deleter(dummyDeleter)
{
    _callback = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<Func>(ptr))(std::forward<ARGS>(args)...);
    };
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::Function(const Function<RET(ARGS...)>& other)
{
    *this = other;
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::Function(Function<RET(ARGS...)>&& other)
{
    *this = std::move(other);
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>&
Function<RET(ARGS...)>::operator=(const Function<RET(ARGS...)>& other)
{
    if (this != &other) {
        _callback = other._callback;
        _deleter = other._deleter;
        if (other._callable == other._storage.data()) {
            _storage = other._storage;
            _callable = _storage.data();
        }
        else {
            _callable = other._callable;
        }
    }
    return *this;
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>&
Function<RET(ARGS...)>::operator=(Function<RET(ARGS...)>&& other)
{
    *this = other;
    if (this != &other) {
        other._callable = nullptr; //disable
    }
    return *this;
}

template <typename RET, typename ... ARGS>
Function<RET(ARGS...)>::~Function()
{
    _deleter(_callable);
}

template <typename RET, typename ... ARGS>
template <typename FUNCTOR>
Function<RET(ARGS...)>::Function(FUNCTOR&& functor)
{
    initFunctor(std::forward<FUNCTOR>(functor), std::is_lvalue_reference<FUNCTOR>());
}

template <typename RET, typename ... ARGS>
RET Function<RET(ARGS...)>::operator()(ARGS...args) {
    return _callback(_callable, std::forward<ARGS>(args)...);
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
    _deleter = dummyDeleter;
    _callback = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<std::remove_reference_t<FUNCTOR>*>(ptr))(std::forward<ARGS>(args)...);
    };
}

template <typename RET, typename ... ARGS>
template <typename FUNCTOR>
void Function<RET(ARGS...)>::initFunctor(FUNCTOR&& functor, std::false_type)
{
    if (sizeof(FUNCTOR) <= size) {
        new (_storage.data()) FUNCTOR(std::forward<FUNCTOR>(functor));
        _callable = _storage.data();
        _deleter = dummyDeleter;
    }
    else {
        _callable = new char[sizeof(FUNCTOR)];
        new (_callable) FUNCTOR(std::forward<FUNCTOR>(functor));
        _deleter = deleter;
    }
    _callback = [](void* ptr, ARGS...args)->RET {
        return (*reinterpret_cast<std::remove_reference_t<FUNCTOR>*>(ptr))(std::forward<ARGS>(args)...);
    };
}

}
}
