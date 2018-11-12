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
#include <type_traits>

namespace Bloomberg {
namespace quantum {

template <class DISPATCHER>
FutureJoiner<DISPATCHER>::FutureJoiner(DISPATCHER& dispatcher) :
    _dispatcher(dispatcher)
{
}

template <class DISPATCHER>
template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<DISPATCHER>::operator()(std::vector<ThreadContextPtr<T>>&& futures)
{
    static_assert(std::is_same<typename DISPATCHER::ContextTag, typename ThreadContext<T>::ContextTag>::value, "Wrong call context");
    return join<ThreadContext,T>(typename DISPATCHER::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<DISPATCHER>::join(std::vector<ThreadContextPtr<T>>&& futures)
{
    static_assert(std::is_same<typename DISPATCHER::ContextTag, typename ThreadContext<T>::ContextTag>::value, "Wrong call context");
    return join<ThreadContext,T>(typename DISPATCHER::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<DISPATCHER>::operator()(std::vector<ThreadFuturePtr<T>>&& futures)
{
    static_assert(std::is_same<typename DISPATCHER::ContextTag, typename ThreadContext<T>::ContextTag>::value, "Wrong call context");
    return join<ThreadFuture,T>(typename DISPATCHER::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<DISPATCHER>::join(std::vector<ThreadFuturePtr<T>>&& futures)
{
    static_assert(std::is_same<typename DISPATCHER::ContextTag, typename ThreadContext<T>::ContextTag>::value, "Wrong call context");
    return join<ThreadFuture,T>(typename DISPATCHER::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<DISPATCHER>::operator()(std::vector<CoroContextPtr<T>>&& futures)
{
    return join<CoroContext,T>(typename CoroContext<T>::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<DISPATCHER>::join(std::vector<CoroContextPtr<T>>&& futures)
{
    return join<CoroContext,T>(typename CoroContext<T>::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<DISPATCHER>::operator()(std::vector<CoroFuturePtr<T>>&& futures)
{
    return join<CoroFuture,T>(typename CoroFuture<T>::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<DISPATCHER>::join(std::vector<CoroFuturePtr<T>>&& futures)
{
    return join<CoroFuture,T>(typename CoroFuture<T>::ContextTag{}, std::move(futures));
}

template <class DISPATCHER>
template <template<class> class FUTURE, class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<DISPATCHER>::join(ThreadContextTag,
                               std::vector<typename FUTURE<T>::Ptr>&& futures)
{
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return _dispatcher.template postAsyncIo<std::vector<T>>([containerPtr](ThreadPromisePtr<std::vector<T>> promise)
    {
        std::vector<T> result;
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get());
        }
        return promise->set(std::move(result));
    });
#else
    return _dispatcher.template postAsyncIo<std::vector<T>>([container{std::move(futures)}](ThreadPromisePtr<std::vector<T>> promise)
    {
        std::vector<T> result;
        for (auto&& f : container)
        {
            result.emplace_back(f->get());
        }
        return promise->set(std::move(result));
    });
#endif
}

template <class DISPATCHER>
template <template<class> class FUTURE, class T>
typename FUTURE<std::vector<T>>::Ptr
FutureJoiner<DISPATCHER>::join(CoroContextTag,
                               std::vector<typename FUTURE<T>::Ptr>&& futures)
{
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return _dispatcher.template post<std::vector<T>>([containerPtr](CoroContextPtr<std::vector<T>> ctx)
    {
        std::vector<T> result;
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get(ctx));
        }
        return ctx->set(std::move(result));
    });
#else
    return _dispatcher.template post<std::vector<T>>([container{std::move(futures)}](CoroContextPtr<std::vector<T>> ctx)
    {
        std::vector<T> result;
        for (auto&& f : container)
        {
            result.emplace_back(f->get(ctx));
        }
        return ctx->set(std::move(result));
    });
#endif
}

}} //namespace

