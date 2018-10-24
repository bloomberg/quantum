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
Util::bindCaller(std::shared_ptr<Context<RET>> ctx, FUNC&& func, ARGS&& ...args0)
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
Util::bindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func, ARGS&& ...args0)
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
Util::bindCaller(std::shared_ptr<Context<RET>> ctx, FUNC&& func0, ARGS&& ...args0)
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
Util::bindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func0, ARGS&& ...args0)
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

template <class RET, class INPUT_IT>
int Util::forEachCoro(CoroContextPtr<std::vector<RET>> ctx,
                      INPUT_IT inputIt,
                      size_t num,
                      const Functions::ForEachFunc<RET, INPUT_IT>& func)
{
    std::vector<CoroContextPtr<RET>> asyncResults;
    asyncResults.reserve(num);
    for (size_t i = 0; i < num; ++i, ++inputIt)
    {
        //Run the function
        asyncResults.emplace_back(ctx->template post<RET>([inputIt, &func](CoroContextPtr<RET> ctx)->int
        {
            return ctx->set(func(*inputIt));
        }));
    }
    std::vector<RET> result;
    result.reserve(num);
    for (auto&& futureValue : asyncResults)
    {
        result.emplace_back(futureValue->get(ctx));
    }
    return ctx->set(std::move(result));
}

template <class RET, class INPUT_IT>
int Util::forEachBatchCoro(CoroContextPtr<std::vector<std::vector<RET>>> ctx,
                           INPUT_IT inputIt,
                           size_t num,
                           const Functions::ForEachFunc<RET, INPUT_IT>& func,
                           size_t numCoroutineThreads)
{
    size_t numPerBatch = num/numCoroutineThreads;
    size_t remainder = num%numCoroutineThreads;
    std::vector<CoroContextPtr<std::vector<RET>>> asyncResults;
    asyncResults.reserve(numCoroutineThreads);
    
    // Post unto all the coroutine threads.
    for (size_t i = 0; i < numCoroutineThreads; ++i)
    {
        //get the begin and end iterators for each batch
        size_t batchSize = (i < remainder) ? numPerBatch + 1 : numPerBatch;
        if (!batchSize)
        {
            break; //nothing to do
        }
        asyncResults.emplace_back(ctx->template post<std::vector<RET>>([inputIt, batchSize, &func](CoroContextPtr<std::vector<RET>> ctx)->int
        {
            std::vector<RET> result;
            auto it = inputIt;
            for (size_t j = 0; j < batchSize; ++j, ++it)
            {
                result.emplace_back(func(*it));
            }
            return ctx->set(std::move(result));
        }));
        std::advance(inputIt, batchSize);
    }
    
    std::vector<std::vector<RET>> result;
    result.reserve(numCoroutineThreads);
    for (auto&& futureBatch : asyncResults)
    {
        result.emplace_back(futureBatch->get(ctx));
    }
    return ctx->set(std::move(result));
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class INPUT_IT>
int Util::mapReduceCoro(CoroContextPtr<std::map<KEY, REDUCED_TYPE>> ctx,
                        INPUT_IT inputIt,
                        size_t num,
                        const Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT>& mapper,
                        const Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE>& reducer)
{
    // Typedefs
    using MappedResult = std::pair<KEY, MAPPED_TYPE>;
    using MapperOutput = std::vector<MappedResult>;
    using IndexerInput = std::vector<MapperOutput>;
    using IndexerOutput = std::map<KEY, std::vector<MAPPED_TYPE>>;
    using ReducedResult = std::pair<KEY, REDUCED_TYPE>;
    using ReducedResults = std::vector<ReducedResult>;
    using ReducerOutput = std::map<KEY, REDUCED_TYPE>;
    
    // Map stage
    IndexerInput indexerInput = ctx->template forEach<MapperOutput>(inputIt, num, mapper)->get(ctx);
    
    // Index stage
    IndexerOutput indexerOutput;
    for (auto&& mapperOutput : indexerInput)
    {
        for (auto&& mapperResult : mapperOutput) {
            indexerOutput[std::move(mapperResult.first)].emplace_back(std::move(mapperResult.second));
        }
    }
    
    // Reduce stage
    ReducedResults reducedResults = ctx->template forEach<ReducedResult>
        (indexerOutput.begin(), indexerOutput.size(), reducer)->get(ctx);
    
    ReducerOutput reducerOutput;
    for (auto&& reducedResult : reducedResults)
    {
        reducerOutput.emplace(std::move(reducedResult.first), std::move(reducedResult.second));
    }
    
    return ctx->set(std::move(reducerOutput));
}

template <class KEY,
          class MAPPED_TYPE,
          class REDUCED_TYPE,
          class INPUT_IT>
int Util::mapReduceBatchCoro(CoroContextPtr<std::map<KEY, REDUCED_TYPE>> ctx,
                             INPUT_IT inputIt,
                             size_t num,
                             const Functions::MapFunc<KEY, MAPPED_TYPE, INPUT_IT>& mapper,
                             const Functions::ReduceFunc<KEY, MAPPED_TYPE, REDUCED_TYPE>& reducer)
{
    // Typedefs
    using MappedResult = std::pair<KEY, MAPPED_TYPE>;
    using MapperOutput = std::vector<MappedResult>;
    using IndexerInput = std::vector<std::vector<MapperOutput>>;
    using IndexerOutput = std::map<KEY, std::vector<MAPPED_TYPE>>;
    using ReducedResult = std::pair<KEY, REDUCED_TYPE>;
    using ReducedResults = std::vector<std::vector<ReducedResult>>;
    using ReducerOutput = std::map<KEY, REDUCED_TYPE>;
    
    // Map stage
    IndexerInput indexerInput = ctx->template forEachBatch<MapperOutput>(inputIt, num, mapper)->get(ctx);
    
    // Index stage
    IndexerOutput indexerOutput;
    for (auto&& partialMapOutput : indexerInput)
    {
        for (auto&& mapperOutput : partialMapOutput)
        {
            for (auto&& mapperResult : mapperOutput) {
                indexerOutput[std::move(mapperResult.first)].emplace_back(std::move(mapperResult.second));
            }
        }
    }
    
    // Reduce stage
    ReducedResults reducedResults = ctx->template forEachBatch<ReducedResult>
        (indexerOutput.begin(), indexerOutput.size(), reducer)->get(ctx);
    
    ReducerOutput reducerOutput;
    for (auto&& partialReducedResult : reducedResults)
    {
        for (auto&& reducedResult : partialReducedResult)
        {
            reducerOutput.emplace(std::move(reducedResult.first), std::move(reducedResult.second));
        }
    }
    
    return ctx->set(std::move(reducerOutput));
}

#ifdef __QUANTUM_PRINT_DEBUG
std::mutex& Util::LogMutex()
{
    static std::mutex m;
    return m;
}
#endif

}}
