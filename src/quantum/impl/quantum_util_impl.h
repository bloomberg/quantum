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
#include <quantum/impl/quantum_stl_impl.h>

namespace Bloomberg {
namespace quantum {

//TODO : rewrite this when lambda template argument capture is supported in C++20
//       so we don't have to pack and unpack a tuple

#if (__cplusplus == 201103L)

template<class RET, class FUNC, class ...ARGS>
constexpr std::function<void(Traits::Yield& yield)>
Util::BindCaller(std::shared_ptr<Context<RET>> ctx, FUNC&& func, ARGS&& ...args0)
{
    auto func0{std::forward<FUNC>(func)};
    std::tuple<ARGS...> tuple0{std::forward<ARGS>(args0)...};
    return [ctx, func0, tuple0] (Traits::Yield& yield)
    {
        std::function<void(ARGS...)> func1 = [ctx, &func0, &yield] (ARGS&&...args1)
        {
            try
            {
                ctx->setYieldHandle(yield); //capture coroutine yield
                yield.get() = func0(std::static_pointer_cast<CoroContext<RET>>(ctx),
                                    std::forward<ARGS>(args1)...);
            }
            catch(std::exception& ex)
            {
                UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught exception " << ex.what() << std::endl;
#endif
                ctx->setException(std::current_exception());
                yield.get() = (int)ITask::RetCode::Exception;
            }
            catch(...)
            {
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught unknown exception." << std::endl;
#endif
                ctx->setException(std::current_exception());
                yield.get() = (int)ITask::RetCode::Exception;
            }
        };
        apply(func1, const_cast<std::tuple<ARGS...>&>(tuple0));
    };
}

template<class RET, class FUNC, class ...ARGS>
constexpr std::function<int()>
Util::BindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func, ARGS&& ...args0)
{
    auto func0{std::forward<FUNC>(func)};
    std::tuple<ARGS...> tuple0{std::forward<ARGS>(args0)...};
    return [promise, func0, tuple0] ()->int
    {
        std::function<int(ARGS...)> func1 = [promise, &func0] (ARGS&&...args1)->int
        {
            try
            {
                return func0(std::static_pointer_cast<ThreadPromise<RET>>(promise),
                             std::forward<ARGS>(args1)...);
            }
            catch(std::exception& ex)
            {
                UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught exception : " << ex.what() << std::endl;
#endif
                promise->setException(std::current_exception());
                return (int)ITask::RetCode::Exception;
            }
            catch(...)
            {
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught unknown exception." << std::endl;
#endif
                promise->setException(std::current_exception());
                return (int)ITask::RetCode::Exception;
            }
        };
        return apply(func1, const_cast<std::tuple<ARGS...>&>(tuple0));
    };
}

#else //(__cplusplus > 201103L)

template<class RET, class FUNC, class ...ARGS>
constexpr std::function<void(Traits::Yield& yield)>
Util::BindCaller(std::shared_ptr<Context<RET>> ctx, FUNC&& func0, ARGS&& ...args0)
{
    return [ctx,
            func1{std::forward<FUNC>(func0)},
            tuple{std::tuple<ARGS...>(std::forward<ARGS>(args0)...)}] (Traits::Yield& yield)
    {
        std::function<void(ARGS...)> func2 = [ctx, &func1, &yield] (ARGS&&...args1)
        {
            try
            {
                ctx->setYieldHandle(yield); //capture coroutine yield
                yield.get() = func1(std::static_pointer_cast<CoroContext<RET>>(ctx),
                                    std::forward<ARGS>(args1)...);
            }
            catch(std::exception& ex)
            {
                UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught exception : " << ex.what() << std::endl;
#endif
                ctx->setException(std::current_exception());
                yield.get() = (int)ITask::RetCode::Exception;
            }
            catch(...)
            {
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught unknown exception." << std::endl;
#endif
                ctx->setException(std::current_exception());
                yield.get() = (int)ITask::RetCode::Exception;
            }
        };
        apply(func2, const_cast<std::tuple<ARGS...>&>(tuple));
    };
}

template<class RET, class FUNC, class ...ARGS>
constexpr std::function<int()>
Util::BindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func0, ARGS&& ...args0)
{
    return [promise,
            func1{std::forward<FUNC>(func0)},
            tuple{std::tuple<ARGS...>(std::forward<ARGS>(args0)...)}] ()->int
    {
        std::function<int(ARGS...)> func2 = [promise, &func1] (ARGS&&...args1)->int
        {
            try
            {
                return func1(std::static_pointer_cast<ThreadPromise<RET>>(promise),
                             std::forward<ARGS>(args1)...);
            }
            catch(std::exception& ex)
            {
                UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught exception : " << ex.what() << std::endl;
#endif
                promise->setException(std::current_exception());
                return (int)ITask::RetCode::Exception;
            }
            catch(...)
            {
#ifdef __QUANTUM_PRINT_DEBUG
                std::lock_guard<std::mutex> guard(Util::LogMutex());
                std::cerr << "Caught unknown exception." << std::endl;
#endif
                promise->setException(std::current_exception());
                return (int)ITask::RetCode::Exception;
            }
        };
        return apply(func2, const_cast<std::tuple<ARGS...>&>(tuple));
    };
}

#endif

#ifdef __QUANTUM_PRINT_DEBUG
std::mutex& Util::LogMutex()
{
    static std::mutex m;
    return m;
}
#endif

}}
