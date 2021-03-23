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
#include <quantum/quantum_local.h>
#include <quantum/quantum_future.h>
#include <quantum/quantum_context.h>

namespace Bloomberg {
namespace quantum {

//Interface conversion helpers for allowing a thread context (or thread future)
//to be used inside a coroutine and vice-versa.
namespace cast {
    template <typename T>
    static CoroContextPtr<T> context(const ThreadContextPtr<T>& ctx) {
        return std::static_pointer_cast<Context<T>>(ctx);
    }
    template <typename T>
    static CoroFuturePtr<T> context(const ThreadFuturePtr<T>& ctx) {
        return std::static_pointer_cast<Future<T>>(ctx);
    }
    template <typename T>
    static ThreadContextPtr<T> context(const CoroContextPtr<T>& ctx) {
        return std::static_pointer_cast<Context<T>>(ctx);
    }
    template <typename T>
    static ThreadFuturePtr<T> context(const CoroFuturePtr<T>& ctx) {
        return std::static_pointer_cast<Future<T>>(ctx);
    }
};

#if (__cplusplus >= 201703L)

// Visitor helpers
template<typename... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename T>
GenericFuture<T>::GenericFuture()
{}

template <typename T>
GenericFuture<T>::GenericFuture(const Promise<T>& p)
{
    if (local::context()) {
        _context = p.getICoroFuture();
    }
    else {
        _context = p.getIThreadFuture();
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadContextPtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadFuturePtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f,
                                ICoroSyncPtr sync) :
    _context(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f,
                                ICoroSyncPtr sync) :
    _context(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(const GenericFuture& other) :
    _context(other._context),
    _sync(other._sync)
{}

template <typename T>
GenericFuture<T>::GenericFuture(GenericFuture&& other) :
    _context(std::move(other._context)),
    _sync(std::move(other._sync))
{}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(const GenericFuture& other)
{
    if (this == &other) {
        return *this;
    }
    _context = other._context;
    _sync = other._sync;
    return *this;
}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(GenericFuture&& other)
{
    if (this == &other) {
        return *this;
    }
    _context = std::move(other._context);
    _sync = std::move(other._sync);
    return *this;
}

template <typename T>
GenericFuture<T>::~GenericFuture()
{}

template <typename T>
bool GenericFuture<T>::valid() const
{
    return std::visit([](const auto& ctx)->bool { return ctx ? ctx->valid() : false; }, _context);
}

template <typename T>
void GenericFuture<T>::wait() const
{
    std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx) {
            if (local::context()) {
                cast::context<T>(ctx)->wait(local::context());
            }
            else {
                ctx->wait();
            }
        },
        [](const ThreadFuturePtr<T>& ctx) {
            if (local::context()) {
                cast::context<T>(ctx)->wait(local::context());
            }
            else {
                ctx->wait();
            }
        },
        [this](const CoroContextPtr<T>& ctx) {
            if (local::context()) {
                ctx->wait(_sync ? _sync : local::context());
            }
            else {
                cast::context<T>(ctx)->wait();
            }
        },
        [this](const CoroFuturePtr<T>& ctx) {
            if (local::context()) {
                ctx->wait(_sync ? _sync : local::context());
            }
            else {
                cast::context<T>(ctx)->wait();
            }
        }
    }, _context);
}

template <typename T>
std::future_status GenericFuture<T>::waitFor(std::chrono::milliseconds timeMs) const
{
    return std::visit(Overloaded {
        [&](const ThreadContextPtr<T>& ctx)->decltype(auto) {
            if (local::context()) {
                return cast::context<T>(ctx)->waitFor(local::context(), timeMs);
            }
            else {
                return ctx->waitFor(timeMs);
            }
        },
        [&](const ThreadFuturePtr<T>& ctx)->decltype(auto) {
            if (local::context()) {
                return cast::context<T>(ctx)->waitFor(local::context(), timeMs);
            }
            else {
                return ctx->waitFor(timeMs);
            }
        },
        [&, this](const CoroContextPtr<T>& ctx)->decltype(auto) {
            if (local::context()) {
                return ctx->waitFor(_sync ? _sync : local::context(), timeMs);
            }
            else {
                return cast::context<T>(ctx)->waitFor(timeMs);
            }
        },
        [&, this](const CoroFuturePtr<T>& ctx)->decltype(auto) {
            if (local::context()) {
                return ctx->waitFor(_sync ? _sync : local::context(), timeMs);
            }
            else {
                return cast::context<T>(ctx)->waitFor(timeMs);
            }
        }
    }, _context);
}

template <typename T>
template <typename V>
NonBufferRetType<V> GenericFuture<T>::get()
{
    return std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->get(local::context());
            }
            else {
                return ctx->get();
            }
        },
        [](const ThreadFuturePtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->get(local::context());
            }
            else {
                return ctx->get();
            }
        },
        [this](const CoroContextPtr<T>& ctx) {
            if (local::context()) {
                return ctx->get(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(ctx)->get();
            }
        },
        [this](const CoroFuturePtr<T>& ctx) {
            if (local::context()) {
                return ctx->get(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(ctx)->get();
            }
        }
    }, _context);
}

template <typename T>
template <typename V>
const NonBufferRetType<V>& GenericFuture<T>::getRef() const
{
    return std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->getRef(local::context());
            }
            else {
                return ctx->get();
            }
        },
        [](const ThreadFuturePtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->getRef(local::context());
            }
            else {
                return ctx->get();
            }
        },
        [this](const CoroContextPtr<T>& ctx) {
            if (local::context()) {
                return ctx->getRef(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(ctx)->getRef();
            }
        },
        [this](const CoroFuturePtr<T>& ctx) {
            if (local::context()) {
                return ctx->getRef(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(ctx)->getRef();
            }
        }
    }, _context);
}

template <typename T>
template <typename V>
BufferRetType<V> GenericFuture<T>::pull(bool& isBufferClosed)
{
    return std::visit(Overloaded {
        [&](const ThreadContextPtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->pull(local::context(), isBufferClosed);
            }
            else {
                return ctx->pull(isBufferClosed);
            }
        },
        [&](const ThreadFuturePtr<T>& ctx) {
            if (local::context()) {
                return cast::context<T>(ctx)->pull(local::context(), isBufferClosed);
            }
            else {
                return ctx->pull(isBufferClosed);
            }
        },
        [&, this](const CoroContextPtr<T>& ctx) {
            if (local::context()) {
                return ctx->pull(_sync ? _sync : local::context(), isBufferClosed);
            }
            else {
                return cast::context<T>(ctx)->pull(isBufferClosed);
            }
        },
        [&, this](const CoroFuturePtr<T>& ctx) {
            if (local::context()) {
                return ctx->pull(_sync ? _sync : local::context(), isBufferClosed);
            }
            else {
                return cast::context<T>(ctx)->pull(isBufferClosed);
            }
        }
    }, _context);
}

#else

template <typename T>
GenericFuture<T>::GenericFuture() :
    _type(Type::Invalid),
    _threadContext()
{}

template <typename T>
GenericFuture<T>::GenericFuture(const Promise<T>& p) :
    _type(local::context() ? Type::CoroFuture : Type::ThreadFuture)
{
    if (_type == Type::ThreadFuture)
    {
        _threadFuture = p.getIThreadFuture();
    }
    else
    {
        _coroFuture = p.getICoroFuture();
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadContextPtr<T> f) :
    _type(Type::ThreadContext),
    _threadContext(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadFuturePtr<T> f) :
    _type(Type::ThreadFuture),
    _threadFuture(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f) :
    _type(Type::CoroContext),
    _coroContext(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f,
                                ICoroSyncPtr sync) :
    _type(Type::CoroContext),
    _coroContext(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f) :
    _type(Type::CoroFuture),
    _coroFuture(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f,
                                ICoroSyncPtr sync) :
    _type(Type::CoroFuture),
    _coroFuture(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(const GenericFuture& other) :
    _type(other._type),
    _threadContext(other._threadContext), //any union member is ok
    _sync(other._sync)
{}

template <typename T>
GenericFuture<T>::GenericFuture(GenericFuture&& other) :
    _type(other._type),
    _threadContext(std::move(other._threadContext)), //any union member is ok
    _sync(std::move(other._sync))
{}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(const GenericFuture& other)
{
    if (this == &other) {
        return *this;
    }
    _type = other._type;
    _threadContext = other._threadContext; //any union member is ok
    _sync = other._sync;
    return *this;
}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(GenericFuture&& other)
{
    if (this == &other) {
        return *this;
    }
    _type = other._type;
    _threadContext = std::move(other._threadContext); //any union member is ok
    _sync = std::move(other._sync);
    return *this;
}

template <typename T>
GenericFuture<T>::~GenericFuture()
{
    switch (_type) {
        case Type::ThreadContext:
            _threadContext.reset();
            break;
        case Type::ThreadFuture:
            _threadFuture.reset();
            break;
        case Type::CoroContext:
            _coroContext.reset();
            break;
        case Type::CoroFuture:
            _coroFuture.reset();
            break;
        default:
            break;
    }
}

template <typename T>
bool GenericFuture<T>::valid() const
{
    switch (_type) {
        case Type::ThreadContext:
            return _threadContext->valid();
        case Type::ThreadFuture:
            return _threadFuture->valid();
        case Type::CoroContext:
            return _coroContext->valid();
        case Type::CoroFuture:
            return _coroFuture->valid();
        default:
            return false;
    }
}

template <typename T>
void GenericFuture<T>::wait() const
{
    switch (_type) {
        case Type::ThreadContext:
            if (local::context()) {
                cast::context<T>(_threadContext)->wait(local::context());
            }
            else {
                _threadContext->wait();
            }
            break;
        case Type::ThreadFuture:
            if (local::context()) {
                cast::context<T>(_threadFuture)->wait(local::context());
            }
            else {
                _threadFuture->wait();
            }
            break;
        case Type::CoroContext:
            if (local::context()) {
                _coroContext->wait(_sync ? _sync : local::context());
            }
            else {
                cast::context<T>(_coroContext)->wait();
            }
            break;
        case Type::CoroFuture:
            if (local::context()) {
                _coroFuture->wait(_sync ? _sync : local::context());
            }
            else {
                cast::context<T>(_coroFuture)->wait();
            }
            break;
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
std::future_status GenericFuture<T>::waitFor(std::chrono::milliseconds timeMs) const
{
    switch (_type) {
        case Type::ThreadContext:
            if (local::context()) {
                return cast::context<T>(_threadContext)->waitFor(local::context(), timeMs);
            }
            else {
                return _threadContext->waitFor(timeMs);
            }
        case Type::ThreadFuture:
            if (local::context()) {
                return cast::context<T>(_threadFuture)->waitFor(local::context(), timeMs);
            }
            else {
                return _threadFuture->waitFor(timeMs);
            }
        case Type::CoroContext:
            if (local::context()) {
                return _coroContext->waitFor(_sync ? _sync : local::context(), timeMs);
            }
            else {
                return cast::context<T>(_coroContext)->waitFor(timeMs);
            }
        case Type::CoroFuture:
            if (local::context()) {
                return _coroFuture->waitFor(_sync ? _sync : local::context(), timeMs);
            }
            else {
                return cast::context<T>(_coroFuture)->waitFor(timeMs);
            }
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
NonBufferRetType<V> GenericFuture<T>::get()
{
    switch (_type) {
        case Type::ThreadContext:
            if (local::context()) {
                return cast::context<T>(_threadContext)->get(local::context());
            }
            else {
                return _threadContext->get();
            }
            break;
        case Type::ThreadFuture:
            if (local::context()) {
                return cast::context<T>(_threadFuture)->get(local::context());
            }
            else {
                return _threadFuture->get();
            }
            break;
        case Type::CoroContext:
            if (local::context()) {
                return _coroContext->get(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(_coroContext)->get();
            }
            break;
        case Type::CoroFuture:
            if (local::context()) {
                return _coroFuture->get(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(_coroFuture)->get();
            }
            break;
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
const NonBufferRetType<V>& GenericFuture<T>::getRef() const
{
    switch (_type) {
        case Type::ThreadContext:
            if (local::context()) {
                return cast::context<T>(_threadContext)->getRef(local::context());
            }
            else {
                return _threadContext->getRef();
            }
            break;
        case Type::ThreadFuture:
            if (local::context()) {
                return cast::context<T>(_threadFuture)->getRef(local::context());
            }
            else {
                return _threadFuture->getRef();
            }
            break;
        case Type::CoroContext:
            if (local::context()) {
                return _coroContext->getRef(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(_coroContext)->getRef();
            }
            break;
        case Type::CoroFuture:
            if (local::context()) {
                return _coroFuture->getRef(_sync ? _sync : local::context());
            }
            else {
                return cast::context<T>(_coroFuture)->getRef();
            }
            break;
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
BufferRetType<V> GenericFuture<T>::pull(bool& isBufferClosed)
{
    switch (_type) {
        case Type::ThreadContext:
            if (local::context()) {
                return cast::context<T>(_threadContext)->pull(local::context(), isBufferClosed);
            }
            else {
                return _threadContext->pull(isBufferClosed);
            }
        case Type::ThreadFuture:
            if (local::context()) {
                return cast::context<T>(_threadFuture)->pull(local::context(), isBufferClosed);
            }
            else {
                return _threadFuture->pull(isBufferClosed);
            }
        case Type::CoroContext:
            if (local::context()) {
                return _coroContext->pull(_sync ? _sync : local::context(), isBufferClosed);
            }
            else {
                return cast::context<T>(_coroContext)->pull(isBufferClosed);
            }
        case Type::CoroFuture:
            if (local::context()) {
                return _coroFuture->pull(_sync ? _sync : local::context(), isBufferClosed);
            }
            else {
                return cast::context<T>(_coroFuture)->pull(isBufferClosed);
            }
        default:
            throw FutureException(FutureState::NoState);
    }
}

#endif //(__cplusplus >= 201703L)
    
}}
