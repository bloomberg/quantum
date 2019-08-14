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
#ifndef BLOOMBERG_QUANTUM_CAPTURE_H
#define BLOOMBERG_QUANTUM_CAPTURE_H

#include <quantum/impl/quantum_stl_impl.h>
#include <quantum/quantum_traits.h>
#include <quantum/quantum_allocator_traits.h>
#include <type_traits>
#include <assert.h>
#include <utility>
#include <tuple>
#include <array>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                   class Capture
//==============================================================================================
/// @class Capture
/// @brief Class allowing lambda parameter captures.
/// @note For internal use only.
template <typename RET, typename FUNC, typename ... ARGS>
class Capture
{
public:
    template <typename F, typename ... T>
    Capture(F&& func, T&&...args);

    template <typename ... T>
    RET operator()(T&&...t);
private:
    FUNC                _func;
    std::tuple<ARGS...> _args;
};

//Helper function
template <typename RET, typename FUNC, typename ... ARGS>
Capture<RET,FUNC,ARGS...>
makeCapture(FUNC&& func, ARGS&& ... args);

//==============================================================================================
//                                   class Function
//==============================================================================================
/// @class Function
/// @brief Similar implementation to std::function except that it allows capture of non-copyable types.
/// @note For internal use only.
template <typename RET, typename ... ARGS>
class Function;

template <typename RET, typename ... ARGS>
class Function<RET(ARGS...)>
{
    static constexpr size_t size{__QUANTUM_FUNCTION_ALLOC_SIZE};
    using Func = RET(*)(ARGS...);
    using Callback = RET(*)(void*, ARGS...);
    using Destructor = void(*)(void*);
    using Deleter = void(*)(void*);
    
public:
    // Ctors
    Function(RET(*ptr)(ARGS...)); //construct with function pointer
    template <typename FUNCTOR>
    Function(FUNCTOR&& functor); //construct with functor
    Function(const Function<RET(ARGS...)>& other) = delete;
    Function(Function<RET(ARGS...)>&& other);
    Function& operator=(const Function<RET(ARGS...)>& other) = delete;
    Function& operator=(Function<RET(ARGS...)>&& other);
    ~Function();
    
    // Methods
    RET operator()(ARGS...args);
    explicit operator bool() const;
    
private:
    static void dummy(void*) {}
    static void deleter(void* p) { delete[] static_cast<char*>(p); }
    
    template <typename FUNCTOR>
    void initFunctor(FUNCTOR&& functor, std::true_type);
    
    template <typename FUNCTOR>
    void initFunctor(FUNCTOR&& functor, std::false_type);
    
    std::array<char, size>  _storage;
    void*                   _callable{nullptr};
    Callback                _invoker{nullptr};
    Destructor              _destructor{dummy};
    Deleter                 _deleter{dummy};
};

}}

#include <quantum/impl/quantum_capture_impl.h>

#endif //BLOOMBERG_QUANTUM_CAPTURE_H
