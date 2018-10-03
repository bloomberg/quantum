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
#include <quantum/quantum_stack_allocator.h>
#include <quantum/quantum_heap_allocator.h>

namespace Bloomberg {
namespace quantum {

#ifndef __QUANTUM_TASK_ALLOC
    #define __QUANTUM_TASK_ALLOC __QUANTUM_DEFAULT_POOL_ALLOC_SIZE
#endif
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    #ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        using TaskAllocator = HeapAllocator<Task, __QUANTUM_TASK_ALLOC>;
    #else
        using TaskAllocator = StackAllocator<Task, __QUANTUM_TASK_ALLOC>;
    #endif
#else
    using TaskAllocator = std::allocator<Task>;
#endif

inline TaskAllocator& GetTaskAllocator() {
    static TaskAllocator allocator;
    return allocator;
}

inline Traits::CoroStackAllocator& GetCoroStackAllocator() {
    static Traits::CoroStackAllocator allocator;
    return allocator;
}

template <class RET, class FUNC, class ... ARGS>
Task::Task(std::shared_ptr<Context<RET>> ctx,
           ITask::Type type,
           FUNC&& func,
           ARGS&&... args) :
    _ctx(ctx),
    _coro(GetCoroStackAllocator(),
          std::move(Util::BindCaller(ctx,
                           std::forward<FUNC>(func),
                           std::forward<ARGS>(args)...))),
    _queueId((int)IQueue::QueueId::Any),
    _isHighPriority(false),
    _rc((int)ITask::RetCode::Running),
    _type(type),
    _terminated(ATOMIC_FLAG_INIT)
{}

template <class RET, class FUNC, class ... ARGS>
Task::Task(std::shared_ptr<Context<RET>> ctx,
           int queueId,
           bool isHighPriority,
           ITask::Type type,
           FUNC&& func,
           ARGS&&... args) :
    _ctx(ctx),
    _coro(GetCoroStackAllocator(),
          std::move(Util::BindCaller(ctx,
                           std::forward<FUNC>(func),
                           std::forward<ARGS>(args)...))),
    _queueId(queueId),
    _isHighPriority(isHighPriority),
    _rc((int)ITask::RetCode::Running),
    _type(type),
    _terminated(ATOMIC_FLAG_INIT)
{}

inline
Task::~Task()
{
    terminate();
}

inline
void Task::terminate()
{
    if (!_terminated.test_and_set())
    {
        if (_ctx) _ctx->terminate();
    }
}

inline
int Task::run()
{
    if (_coro)
    {
        _coro(_rc);
        return _rc;
    }
    return (int)ITask::RetCode::Success;
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
    return _ctx ? _ctx->isBlocked() : false; //context is waiting on some signal
}

inline
bool Task::isHighPriority() const
{
    return _isHighPriority;
}

inline
void* Task::operator new(size_t size)
{
    return GetTaskAllocator().allocate(size);
}

inline
void Task::operator delete(void* p)
{
    GetTaskAllocator().deallocate(static_cast<Task*>(p), 1);
}

inline
void Task::deleter(Task* p)
{
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    GetTaskAllocator().dispose(p);
#else
    delete p;
#endif
}

}}
