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
#ifndef BLOOMBERG_QUANTUM_TASK_STATE_HANDLER_H
#define BLOOMBERG_QUANTUM_TASK_STATE_HANDLER_H

#include <functional>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                       enum TaskType
//==============================================================================================
/// @enum TaskType
/// @brief Defines the types of the tasks.
/// @note This enum should be used to specify the type of the task
///       for which a task state handler is called
enum class TaskType : int {
    None        = 0,      ///< No task type
    Coroutine   = 1 << 0, ///< Coroutine
    IoTask      = 1 << 1, ///< Blocking IO (or long running) task
    All         = Coroutine |
                  IoTask  ///< All task types
};

//==============================================================================================
//                                       enum TaskState
//==============================================================================================
/// @enum TaskState
/// @brief Defines the states of the task.
/// @note Schema of the state machine: [Initialized -> Started -> *[Suspended -> Resumed] -> Stopped]
///       Firsly, the task is created and before it's scheduled it dwells in 'Initialized' state.
///       When the task is scheduled to be executed for the first time it switches to 'Started' state.
///       If the task isn't completed (the execution control is passed to another coroutine) it goes into 'Suspended' state,
///       otherwise upon the completion or termination due to an exception it switches to 'Stopped' state.
///       If the task dwells in 'Suspended' state and scheduled for execution it goes into 'Resumed' state.
///       The switch between 'Suspended' and 'Resumed' states may take several times until the task eventually switches over to 'Stopped' state.
/// @note 'Initialized' state is required for the state machine validation and won't be handled
/// @note 'Suspend' and 'Resumed' states don't exist for the blocking IO task and therefore won't be handled
/// @warning No mutexes or shared structued should ever be used as it will slow the performance significantly or
///          grind the whole execution to a halt. Only thread local variables or pre-allocated arrays that could use
///          the queue id as index.
enum class TaskState : int {
    None        = 0,      ///< No task state
    Initialized = 1 << 0, ///< Task is initialized and isn't started yet.
    Started     = 1 << 1, ///< Task is started
    Suspended   = 1 << 2, ///< Task isn't completed and the control of execution is passed to another coroutine
    Resumed     = 1 << 3, ///< Task is scheduled to be executed after suspension
    Stopped     = 1 << 4, ///< Task is completed or terminated
    All         = Started |
                  Suspended |
                  Resumed |
                  Stopped ///< All task states
};

/// @brief Callback for handling task states
/// @param[in] taskId The id of the currently executing coroutine or IO task
/// @param[in] queueId The id of the queue where the task should run
/// @param[in] type The processed task type
/// @param[in] state The processed task state
/// @note Technical task states such as 'None' and 'Initialized' won't be processed by the handler.
using TaskStateHandler = std::function<void(size_t taskId, int queueId, TaskType type, TaskState state)>;

//==============================================================================================
//                                    class TaskStateConfiguration
//==============================================================================================
/// @class TaskStateConfiguration
/// @brief Contains parameters for task states handling.
class TaskStateConfiguration {
public:
    /// @brief Set the task state handler.
    /// @param[in] handler The task state handler. Default is no handler.
    /// @return A reference to itself
    TaskStateConfiguration& setTaskStateHandler(const TaskStateHandler& handler);

    /// @brief Set the task states that will be handled by the task state handler.
    /// @param[in] states The task states that will be handled. Default is TaskState::None.
    /// @return A reference to itself
    TaskStateConfiguration& setHandledTaskStates(TaskState states);

    /// @brief Set the task types that will be handled by the task state handler.
    /// @param[in] types The task types that will be handled. Default is TaskType::None.
    /// @return A reference to itself
    TaskStateConfiguration& setHandledTaskTypes(TaskType types);

    /// @brief Get the task state handler.
    /// @return The task state handler.
    const TaskStateHandler& getTaskStateHandler() const;

    /// @brief Get the handled task states.
    /// @return The handled task states.
    TaskState getHandledTaskStates() const;

    /// @brief Get the handled task types.
    /// @return The handled task types.
    TaskType getHandledTaskTypes() const;

private:
    TaskStateHandler handler_; // task state handler
    TaskState handledStates_{TaskState::None}; // task states that will be handled
    TaskType handledTypes_{TaskType::None}; // task types that will be handled
};

/// @brief Function that handle a given task state
/// @param[in] stateHandler task state handler.
/// @param[in] taskId id of the currently executing coroutine or IO task.
/// @param[in] queueId id of the queue where the task should run on.
/// @param[in] handledType task type that will be handled by given task state handler.
/// @param[in] handledStates task states that will be handled by given task state handler.
/// @param[in] nextState future task state which will be handled.
/// @param[out] currentState current task state that will be updated to next task state.
/// @note For internal use only.
void handleTaskState(const TaskStateHandler& stateHandler,
                     size_t taskId,
                     int queueId,
                     TaskType handledType,
                     TaskState handledStates,
                     TaskState nextState,
                     TaskState& currentState);
}}

#include <quantum/impl/quantum_task_state_handler_impl.h>

#endif //BLOOMBERG_QUANTUM_TASK_STATE_HANDLER_H
