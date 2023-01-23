/*
** Copyright 2022 Bloomberg Finance L.P.
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

#include <quantum/quantum_auxiliary.h>

namespace Bloomberg {
namespace quantum {

inline
TaskStateConfiguration& TaskStateConfiguration::setTaskStateHandler(const TaskStateHandler& handler)
{
    handler_ = handler;
    return *this;
}

inline
TaskStateConfiguration& TaskStateConfiguration::setHandledTaskStates(TaskState states)
{
    handledStates_ = states;
    return *this;
}

inline
TaskStateConfiguration& TaskStateConfiguration::setHandledTaskTypes(TaskType types)
{
    handledTypes_ = types;
    return *this;
}

inline
const TaskStateHandler& TaskStateConfiguration::getTaskStateHandler() const
{
    return handler_;
}

inline
TaskState TaskStateConfiguration::getHandledTaskStates() const
{
    return handledStates_;
}

inline
TaskType TaskStateConfiguration::getHandledTaskTypes() const
{
    return handledTypes_;
}

inline
bool isValidTaskStateOrded(TaskState currentState, TaskState nextState)
{
    switch (nextState) {
        case TaskState::Started:
            return TaskState::Initialized == currentState;

        case TaskState::Suspended:
            return TaskState::Started == currentState || TaskState::Resumed == currentState;

        case TaskState::Resumed:
            return TaskState::Suspended == currentState;

        case TaskState::Stopped:
            return TaskState::Started == currentState || TaskState::Resumed == currentState;

        case TaskState::Initialized:
        case TaskState::None:
            return false;

        default:
            return false;
    }
}

inline
void handleTaskState(const TaskStateHandler& stateHandler,
                     size_t taskId,
                     int queueId,
                     TaskType handledType,
                     TaskState handledStates,
                     TaskState nextState,
                     TaskState& currentState)
{
    if (not isValidTaskStateOrded(currentState, nextState))
    {
#ifdef __QUANTUM_PRINT_DEBUG
        std::lock_guard<std::mutex> guard(Util::LogMutex());
        std::cerr   << "Invalid task state order"
                    << ", task id: " << taskId
                    << ", next task state: "
                    << std::to_string(static_cast<int>(nextState))
                    << ", current task state: "
                    << std::to_string(static_cast<int>(currentState)) << std::endl;
        assert(false);
#endif
        return;
    }
    currentState = nextState;

    if (not stateHandler)
    {
        return;
    }

    if (not isIntersection(handledStates, nextState))
    {
        return;
    }

    stateHandler(taskId, queueId, handledType, nextState);
}

}}
