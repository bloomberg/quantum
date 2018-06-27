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

template <class RET, class FUNC, class ... ARGS>
Task::Task(std::shared_ptr<Context<RET>> ctx,
           ITask::Type type,
           FUNC&& func,
           ARGS&&... args) :
    _ctx(ctx),
    _coro(Util::BindCaller(ctx,
                           std::forward<FUNC>(func),
                           std::forward<ARGS>(args)...)),
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
    _coro(Util::BindCaller(ctx,
                           std::forward<FUNC>(func),
                           std::forward<ARGS>(args)...)),
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
ITaskContinuation::ptr Task::getNextTask() { return _next; }

inline
void Task::setNextTask(ITaskContinuation::ptr nextTask) { _next = nextTask; }

inline
ITaskContinuation::ptr Task::getPrevTask() { return _prev.lock(); }

inline
void Task::setPrevTask(ITaskContinuation::ptr prevTask) { _prev = prevTask; }

inline
ITaskContinuation::ptr Task::getFirstTask()
{
    return (_type == Type::First) ? shared_from_this() : getPrevTask()->getFirstTask();
}

inline
ITaskContinuation::ptr Task::getErrorHandlderOrFinalTask()
{
    if ((_type == Type::ErrorHandler) || (_type == Type::Final))
    {
        return shared_from_this();
    }
    else if (_next)
    {
        ITaskContinuation::ptr task = _next->getErrorHandlderOrFinalTask();
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

}}
