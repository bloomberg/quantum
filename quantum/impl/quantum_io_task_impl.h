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

namespace Bloomberg {
namespace quantum {

#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    #ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        using IoTaskAllocator = HeapAllocator<IoTask>;
    #else
        using IoTaskAllocator = StackAllocator<IoTask, __QUANTUM_IO_TASK_ALLOC_SIZE>;
    #endif
#else
    using IoTaskAllocator = StlAllocator<IoTask>;
#endif

template <class RET, class FUNC, class ... ARGS>
IoTask::IoTask(std::true_type,
               std::shared_ptr<Promise<RET>> promise,
               int queueId,
               bool isHighPriority,
               FUNC&& func,
               ARGS&&... args) :
    _func(Util::bindIoCaller(promise, std::forward<FUNC>(func), std::forward<ARGS>(args)...)),
    _terminated(false),
    _queueId(queueId),
    _isHighPriority(isHighPriority)
{
}

template <class RET, class FUNC, class ... ARGS>
IoTask::IoTask(std::false_type,
               std::shared_ptr<Promise<RET>> promise,
               int queueId,
               bool isHighPriority,
               FUNC&& func,
               ARGS&&... args) :
    _func(Util::bindIoCaller2(promise, std::forward<FUNC>(func), std::forward<ARGS>(args)...)),
    _terminated(false),
    _queueId(queueId),
    _isHighPriority(isHighPriority)
{
}

inline
IoTask::~IoTask()
{
    terminate();
}

inline
void IoTask::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        //not used
    }
}

inline
int IoTask::run()
{
    return _func ? _func() : (int)ITask::RetCode::NotCallable;
}

inline
void IoTask::setQueueId(int queueId)
{
    _queueId = queueId;
}

inline
int IoTask::getQueueId()
{
    return _queueId;
}

inline
ITask::Type IoTask::getType() const
{
    return ITask::Type::IO;
}

inline
bool IoTask::isBlocked() const
{
    return false;
}

inline
bool IoTask::isSleeping(bool)
{
    return false;
}

inline
bool IoTask::isHighPriority() const
{
    return _isHighPriority;
}

inline
bool IoTask::isSuspended() const
{
    return false;
}

inline
void* IoTask::operator new(size_t)
{
    return Allocator<IoTaskAllocator>::instance(AllocatorTraits::ioTaskAllocSize()).allocate();
}

inline
void IoTask::operator delete(void* p)
{
    Allocator<IoTaskAllocator>::instance(AllocatorTraits::ioTaskAllocSize()).deallocate(static_cast<IoTask*>(p));
}

inline
void IoTask::deleter(IoTask* p)
{
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    Allocator<IoTaskAllocator>::instance(AllocatorTraits::ioTaskAllocSize()).dispose(p);
#else
    delete p;
#endif
}

}}

