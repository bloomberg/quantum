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
#ifndef BLOOMBERG_QUANTUM_FUTURE_STATE_H
#define BLOOMBERG_QUANTUM_FUTURE_STATE_H

#include <future>
#include <exception>
#include <map>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                enum FutureState
//==============================================================================================
/// @enum FutureState
/// @brief Represents the internal state of a future/promise pair. Modeled after std::future_errc.
enum class FutureState
{
    PromiseNotSatisfied = 1,        ///< Future value has not yet been set.
    PromiseAlreadySatisfied,        ///< Future value has been set but not yet consumed.
    BrokenPromise,                  ///< Promise could not be fulfilled.
    FutureAlreadyRetrieved,         ///< Future value has been consumed. In the case of a buffer, no pulling is allowed.
    NoState,                        ///< Shared state between Promise and Future is invalid.
    BufferingData,                  ///< Buffered future is being streamed.
    BufferClosed                    ///< Buffer is closed for pushing data. Data can still be pulled.
};

//==============================================================================================
//                                struct FutureException
//==============================================================================================
/// @struct FutureException
/// @brief Exception thrown by a Future or Promise object during various errors.
struct FutureException : public std::exception
{
    /// @brief Constructor.
    /// @param[in] error Enum representing the error state.
    FutureException(FutureState error) :
        _error(error)
    {}
    
    /// @brief Get the encapsulated error message from the exception object.
    /// @return Error message.
    const char* what() const noexcept override
    {
        static std::map<FutureState, const char*> msg{
            {FutureState::PromiseNotSatisfied,      "Promise not yet satisfied"},
            {FutureState::PromiseAlreadySatisfied,  "Promise already satisfied"},
            {FutureState::BrokenPromise,            "Broken promise"},
            {FutureState::FutureAlreadyRetrieved,   "Future already retrieved"},
            {FutureState::NoState,                  "Invalid state"},
            {FutureState::BufferingData,            "Buffering future data"},
            {FutureState::BufferClosed,             "Buffer closed"}
        };
        return msg[_error];
    }
    
private:
    //Members
    FutureState _error;
};


//==============================================================================================
//                              Specialised FutureException
//==============================================================================================
struct PromiseNotSatisfiedException : public FutureException
{
    PromiseNotSatisfiedException() :
        FutureException(FutureState::PromiseNotSatisfied)
    {}
};

struct PromiseAlreadySatisfiedException : public FutureException
{
    PromiseAlreadySatisfiedException() :
        FutureException(FutureState::PromiseAlreadySatisfied)
    {}
};

struct BrokenPromiseException : public FutureException
{
    BrokenPromiseException() :
        FutureException(FutureState::BrokenPromise)
    {}
};

struct FutureAlreadyRetrievedException : public FutureException
{
    FutureAlreadyRetrievedException() :
        FutureException(FutureState::FutureAlreadyRetrieved)
    {}
};

struct NoStateException : public FutureException
{
    NoStateException() :
        FutureException(FutureState::NoState)
    {}
};

struct BufferingDataException : public FutureException
{
    BufferingDataException() :
        FutureException(FutureState::BufferingData)
    {}
};

struct BufferClosedException : public FutureException
{
    BufferClosedException() :
        FutureException(FutureState::BufferClosed)
    {}
};

inline
void ThrowFutureException(FutureState state)
{
    switch (state)
    {
        case FutureState::PromiseNotSatisfied: throw PromiseNotSatisfiedException();
        case FutureState::PromiseAlreadySatisfied: throw PromiseAlreadySatisfiedException();
        case FutureState::BrokenPromise: throw BrokenPromiseException();
        case FutureState::FutureAlreadyRetrieved: throw FutureAlreadyRetrievedException();
        case FutureState::NoState: throw NoStateException();
        case FutureState::BufferingData: throw BufferingDataException();
        case FutureState::BufferClosed: throw BufferClosedException();
        default: throw std::logic_error("Invalid future state");
    }
}

}}

#endif //BLOOMBERG_QUANTUM_FUTURE_STATE_H
