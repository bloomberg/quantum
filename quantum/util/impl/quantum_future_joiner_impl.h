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

template <typename T>
template <class DISPATCHER, class>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::operator()(DISPATCHER& dispatcher, std::vector<ThreadContextPtr<T>>&& futures)
{
   return join<ThreadContext>(ThreadContextTag{}, dispatcher, std::move(futures));
}

template <typename T>
template <class DISPATCHER, class>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::operator()(DISPATCHER& dispatcher, std::vector<ThreadFuturePtr<T>>&& futures)
{
    return join<ThreadFuture>(ThreadContextTag{}, dispatcher, std::move(futures));
}

template <typename T>
template <class DISPATCHER, class>
CoroContextPtr<std::vector<T>>
FutureJoiner<T>::operator()(DISPATCHER& dispatcher, std::vector<CoroContextPtr<T>>&& futures)
{
    return join<CoroContext>(CoroContextTag{}, dispatcher, std::move(futures));
}

template <typename T>
template <class DISPATCHER, class>
CoroContextPtr<std::vector<T>>
FutureJoiner<T>::operator()(DISPATCHER& dispatcher, std::vector<CoroFuturePtr<T>>&& futures)
{
    return join<CoroFuture>(CoroContextTag{}, dispatcher, std::move(futures));
}

template <typename T>
template <template<class> class FUTURE, class DISPATCHER>
ThreadFuturePtr<std::vector<T>>
FutureJoiner<T>::join(ThreadContextTag, DISPATCHER& dispatcher, std::vector<typename FUTURE<T>::Ptr>&& futures)
{
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return dispatcher.template postAsyncIo2([containerPtr]()->std::vector<T>
    {
        std::vector<T> result;
        result.reserve(containerPtr->size());
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get());
        }
        return result;
    });
#else
    return dispatcher.template postAsyncIo2([container{std::move(futures)}]()->std::vector<T>
    {
        std::vector<T> result;
        result.reserve(container.size());
        for (auto&& f : container)
        {
            result.emplace_back(f->get());
        }
        return result;
    });
#endif
}

template <typename T>
template <template<class> class FUTURE, class DISPATCHER>
CoroContextPtr<std::vector<T>>
FutureJoiner<T>::join(CoroContextTag, DISPATCHER& dispatcher, std::vector<typename FUTURE<T>::Ptr>&& futures)
{
#if (__cplusplus == 201103L)
    std::shared_ptr<std::vector<typename FUTURE<T>::Ptr>> containerPtr(new std::vector<typename FUTURE<T>::Ptr>(std::move(futures)));
    return dispatcher.template post2([containerPtr](VoidContextPtr ctx)->std::vector<T>
    {
        std::vector<T> result;
        result.reserve(containerPtr->size());
        for (auto&& f : *containerPtr)
        {
            result.emplace_back(f->get(ctx));
        }
        return result;
    });
#else
    return dispatcher.template post2([container{std::move(futures)}](VoidContextPtr ctx)->std::vector<T>
    {
        std::vector<T> result;
        result.reserve(container.size());
        for (auto&& f : container)
        {
            result.emplace_back(f->get(ctx));
        }
        return result;
    });
#endif
}

}} //namespace

