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

template <class T>
FutureJoiner<T>::FutureJoiner(Dispatcher* dispatcher) :
    _threadDispatcher(dispatcher)
{
}

template <class T>
FutureJoiner<T>::FutureJoiner(CoroContextPtr<T> dispatcher) :
    _threadDispatcher(nullptr),
    _coroDispatcher(dispatcher)
{
}

template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::operator()(std::vector<ThreadContextPtr<T>>&& futures)
{
    return join<ThreadContext>(typename ThreadContext<T>::ContextTag{}, std::move(futures));
}

template <class T>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::operator()(std::vector<ThreadFuturePtr<T>>&& futures)
{
    return join<ThreadFuture>(typename ThreadFuture<T>::ContextTag{}, std::move(futures));
}

template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<T>::operator()(std::vector<CoroContextPtr<T>>&& futures)
{
    return join<CoroContext>(typename CoroContext<T>::ContextTag{}, std::move(futures));
}

template <class T>
CoroContextPtr<std::vector<T>>
FutureJoiner<T>::operator()(std::vector<CoroFuturePtr<T>>&& futures)
{
    return join<CoroFuture>(typename CoroFuture<T>::ContextTag{}, std::move(futures));
}

template <class T>
template <template<class> class FUTURE>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::join(ThreadContextTag,
                      std::vector<typename FUTURE<T>::Ptr>&& futures)
{
    assert(_threadDispatcher);
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return _threadDispatcher->template postAsyncIo<std::vector<T>>([containerPtr](ThreadPromisePtr<std::vector<T>> promise)
    {
        std::vector<T> result;
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get());
        }
        return promise->set(std::move(result));
    });
#else
    return _threadDispatcher->template postAsyncIo<std::vector<T>>([container{std::move(futures)}](ThreadPromisePtr<std::vector<T>> promise)
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

template <class T>
template <template<class> class FUTURE>
typename FUTURE<std::vector<T>>::Ptr
FutureJoiner<T>::join(CoroContextTag,
                      std::vector<typename FUTURE<T>::Ptr>&& futures)
{
    assert(_coroDispatcher);
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return _coroDispatcher->template post<std::vector<T>>([containerPtr](CoroContextPtr<std::vector<T>> ctx)
    {
        std::vector<T> result;
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get(ctx));
        }
        return ctx->set(std::move(result));
    });
#else
    return _coroDispatcher->template post<std::vector<T>>([container{std::move(futures)}](CoroContextPtr<std::vector<T>> ctx)
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

