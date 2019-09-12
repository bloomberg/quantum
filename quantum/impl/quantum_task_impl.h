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
#include <quantum/quantum_allocator.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    #ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        using TaskAllocator = HeapAllocator<Task>;
    #else
        using TaskAllocator = StackAllocator<Task, __QUANTUM_TASK_ALLOC_SIZE>;
    #endif
#else
    using TaskAllocator = StlAllocator<Task>;
#endif

template <class RET, class FUNC, class ... ARGS>
Task::Task(std::false_type,
           std::shared_ptr<Context<RET>> ctx,
           int queueId,
           bool isHighPriority,
           ITask::Type type,
           FUNC&& func,
           ARGS&&... args) :
    _coroContext(ctx),
    _coro(Allocator<CoroStackAllocator>::instance(AllocatorTraits::defaultCoroPoolAllocSize()),
          Util::bindCaller(ctx, std::forward<FUNC>(func), std::forward<ARGS>(args)...)),
    _queueId(queueId),
    _isHighPriority(isHighPriority),
    _type(type),
    _terminated(false),
    _suspendedState((int)State::Suspended),
    _coroLocalStorage()
{}

template <class RET, class FUNC, class ... ARGS>
Task::Task(std::true_type,
           std::shared_ptr<Context<RET>> ctx,
           int queueId,
           bool isHighPriority,
           ITask::Type type,
           FUNC&& func,
           ARGS&&... args) :
    _coroContext(ctx),
    _coro(Allocator<CoroStackAllocator>::instance(AllocatorTraits::defaultCoroPoolAllocSize()),
          Util::bindCaller2(ctx, std::forward<FUNC>(func), std::forward<ARGS>(args)...)),
    _queueId(queueId),
    _isHighPriority(isHighPriority),
    _type(type),
    _terminated(false),
    _suspendedState((int)State::Suspended),
    _coroLocalStorage()
{}

inline
Task::~Task()
{
    terminate();
}

inline
void Task::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        if (_coroContext) _coroContext->terminate();
    }
}

inline
int Task::run()
{
    SuspensionGuard guard(_suspendedState);
    if (guard)
    {
        if (!_coro)
        {
            return (int)ITask::RetCode::NotCallable;
        }
        if (isBlocked())
        {
            return (int)ITask::RetCode::Blocked;
        }
        if (isSleeping(true))
        {
            return (int)ITask::RetCode::Sleeping;
        }
        
        int rc = (int)ITask::RetCode::Running;
        _coro(rc);
        if (!_coro)
        {
            guard.set((int)State::Terminated);
        }
        return rc;
    }
    return (int)ITask::RetCode::AlreadyResumed;
}

inline
void Task::setQueueId(int queueId)
{
    _queueId = queueId;
}

inline
int Task::getQueueId()
{
    return _queueId;
}

inline
ITask::Type Task::getType() const { return _type; }

inline
ITaskContinuation::Ptr Task::getNextTask() { return _next; }

inline
void Task::setNextTask(ITaskContinuation::Ptr nextTask) { _next = nextTask; }

inline
ITaskContinuation::Ptr Task::getPrevTask() { return _prev.lock(); }

inline
void Task::setPrevTask(ITaskContinuation::Ptr prevTask) { _prev = prevTask; }

inline
ITaskContinuation::Ptr Task::getFirstTask()
{
    return (_type == Type::First) ? shared_from_this() : getPrevTask()->getFirstTask();
}

inline
ITaskContinuation::Ptr Task::getErrorHandlerOrFinalTask()
{
    if ((_type == Type::ErrorHandler) || (_type == Type::Final))
    {
        return shared_from_this();
    }
    else if (_next)
    {
        ITaskContinuation::Ptr task = _next->getErrorHandlerOrFinalTask();
        if ((_next->getType() != Type::ErrorHandler) && (_next->getType() != Type::Final))
        {
            _next->terminate();
            _next.reset(); //release next task
        }
        return task;
    }
    return nullptr;
}

inline
bool Task::isBlocked() const
{
    return _coroContext ? _coroContext->isBlocked() : false; //coroutine is waiting on some signal
}

inline
bool Task::isSleeping(bool updateTimer)
{
    return _coroContext ? _coroContext->isSleeping(updateTimer) : false; //coroutine is sleeping
}

inline
bool Task::isHighPriority() const
{
    return _isHighPriority;
}

inline
bool Task::isSuspended() const
{
    return _suspendedState == (int)State::Suspended;
}

inline
Task::CoroLocalStorage& Task::getCoroLocalStorage()
{
    return _coroLocalStorage;
}

inline
ITaskAccessor::Ptr Task::getTaskAccessor() const
{
    return _coroContext;
}

inline
void* Task::operator new(size_t)
{
    return Allocator<TaskAllocator>::instance(AllocatorTraits::taskAllocSize()).allocate();
}

inline
void Task::operator delete(void* p)
{
    Allocator<TaskAllocator>::instance(AllocatorTraits::taskAllocSize()).deallocate(static_cast<Task*>(p));
}

inline
void Task::deleter(Task* p)
{
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    Allocator<TaskAllocator>::instance(AllocatorTraits::taskAllocSize()).dispose(p);
#else
    delete p;
#endif
}
    
}}
