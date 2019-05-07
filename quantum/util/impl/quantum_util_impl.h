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
#include <quantum/util/quantum_future_joiner.h>

namespace Bloomberg {
namespace quantum {

template <typename RET, typename CAPTURE>
int bindCoro(Traits::Yield& yield,
             std::shared_ptr<CoroContext<RET>> ctx,
             CAPTURE&& capture)
{
    try
    {
        ctx->setYieldHandle(yield); //set coroutine yield
        yield.get() = std::forward<CAPTURE>(capture)();
        return 0;
    }
    catch(const boost::coroutines2::detail::forced_unwind&) {
        throw;
    }
    catch(std::exception& ex)
    {
        UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr << "Caught exception : " << ex.what() << std::endl;
#endif
        ctx->setException(std::current_exception());
    }
    catch(...)
    {
#ifdef __QUANTUM_PRINT_DEBUG
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr << "Caught unknown exception." << std::endl;
#endif
        ctx->setException(std::current_exception());
    }
    yield.get() = (int)ITask::RetCode::Exception;
    return (int)ITask::RetCode::Exception;
}

template <typename RET, typename CAPTURE>
int bindIo(std::shared_ptr<Promise<RET>> promise,
           CAPTURE&& capture)
{
    try
    {
        return std::forward<CAPTURE>(capture)();
    }
    catch(std::exception& ex)
    {
        UNUSED(ex);
#ifdef __QUANTUM_PRINT_DEBUG
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr << "Caught exception : " << ex.what() << std::endl;
#endif
        promise->setException(std::current_exception());
    }
    catch(...)
    {
#ifdef __QUANTUM_PRINT_DEBUG
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr << "Caught unknown exception." << std::endl;
#endif
        promise->setException(std::current_exception());
    }
    return (int)ITask::RetCode::Exception;
}

template<class RET, class FUNC, class ...ARGS>
Function<int(Traits::Yield&)>
Util::bindCaller(std::shared_ptr<Context<RET>> context, FUNC&& func, ARGS&& ...args)
{
    auto capture = makeCapture(std::forward<FUNC>(func), std::shared_ptr<Context<RET>>(context), std::forward<ARGS>(args)...);
    return makeCapture(bindCoro<RET, decltype(capture)>, std::shared_ptr<Context<RET>>(context), std::move(capture));
}

template<class RET, class FUNC, class ...ARGS>
Function<int()>
Util::bindIoCaller(std::shared_ptr<Promise<RET>> promise, FUNC&& func, ARGS&& ...args)
{
    auto capture = makeCapture(std::forward<FUNC>(func), std::shared_ptr<Promise<RET>>(promise), std::forward<ARGS>(args)...);
    return makeCapture(bindIo<RET, decltype(capture)>, std::shared_ptr<Promise<RET>>(promise), std::move(capture));
}

template <class RET, class INPUT_IT, class FUNC>
int Util::forEachCoro(CoroContextPtr<std::vector<RET>> ctx,
                      INPUT_IT inputIt,
                      size_t num,
                      FUNC&& func)
{
    std::vector<CoroContextPtr<RET>> asyncResults;
    asyncResults.reserve(num);
    for (size_t i = 0; i < num; ++i, ++inputIt)
    {
        //Run the function
        asyncResults.emplace_back(ctx->template post<RET>([inputIt, &func](CoroContextPtr<RET> ctx) mutable ->int
        {
            return ctx->set(std::forward<FUNC>(func)(Util::makeVoidContext<RET>(ctx), *inputIt));
        }));
    }
    return ctx->set(FutureJoiner<RET>()(*ctx, std::move(asyncResults))->get(ctx));
}

template <class RET, class INPUT_IT, class FUNC>
int Util::forEachBatchCoro(CoroContextPtr<std::vector<std::vector<RET>>> ctx,
                           INPUT_IT inputIt,
                           size_t num,
                           FUNC&& func,
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
        asyncResults.emplace_back(ctx->template post<std::vector<RET>>([inputIt, batchSize, &func](CoroContextPtr<std::vector<RET>> ctx) mutable->int
        {
            std::vector<RET> result;
            result.reserve(batchSize);
            for (size_t j = 0; j < batchSize; ++j, ++inputIt)
            {
                result.emplace_back(std::forward<FUNC>(func)(Util::makeVoidContext<std::vector<RET>>(ctx), *inputIt));
            }
            return ctx->set(std::move(result));
        }));
        std::advance(inputIt, batchSize);
    }
    return ctx->set(FutureJoiner<std::vector<RET>>()(*ctx, std::move(asyncResults))->get(ctx));
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

template <typename RET>
VoidContextPtr Util::makeVoidContext(CoroContextPtr<RET> ctx)
{
    return std::reinterpret_pointer_cast<CoroContext<Void>, CoroContext<RET>>(ctx);
}

#ifdef __QUANTUM_PRINT_DEBUG
std::mutex& Util::LogMutex()
{
    static std::mutex m;
    return m;
}
#endif

}}
