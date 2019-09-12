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
#ifndef BLOOMBERG_QUANTUM_H
#define BLOOMBERG_QUANTUM_H

#define __NATIVESTD__
#include <quantum/interface/quantum_icontext.h>
#include <quantum/interface/quantum_icontext_base.h>
#include <quantum/interface/quantum_icoro_context.h>
#include <quantum/interface/quantum_icoro_context_base.h>
#include <quantum/interface/quantum_icoro_future.h>
#include <quantum/interface/quantum_icoro_future_base.h>
#include <quantum/interface/quantum_icoro_promise.h>
#include <quantum/interface/quantum_icoro_sync.h>
#include <quantum/interface/quantum_ifuture.h>
#include <quantum/interface/quantum_ipromise.h>
#include <quantum/interface/quantum_ipromise_base.h>
#include <quantum/interface/quantum_iqueue.h>
#include <quantum/interface/quantum_iqueue_statistics.h>
#include <quantum/interface/quantum_itask.h>
#include <quantum/interface/quantum_itask_accessor.h>
#include <quantum/interface/quantum_itask_continuation.h>
#include <quantum/interface/quantum_iterminate.h>
#include <quantum/interface/quantum_ithread_context.h>
#include <quantum/interface/quantum_ithread_context_base.h>
#include <quantum/interface/quantum_ithread_future.h>
#include <quantum/interface/quantum_ithread_future_base.h>
#include <quantum/interface/quantum_ithread_promise.h>
#include <quantum/quantum_allocator.h>
#include <quantum/quantum_allocator_traits.h>
#include <quantum/quantum_buffer.h>
#include <quantum/quantum_capture.h>
#include <quantum/quantum_condition_variable.h>
#include <quantum/quantum_configuration.h>
#include <quantum/quantum_context.h>
#include <quantum/quantum_contiguous_pool_manager.h>
#include <quantum/quantum_coroutine_pool_allocator.h>
#include <quantum/quantum_dispatcher.h>
#include <quantum/quantum_dispatcher_core.h>
#include <quantum/quantum_functions.h>
#include <quantum/quantum_future.h>
#include <quantum/quantum_future_state.h>
#include <quantum/quantum_io_queue.h>
#include <quantum/quantum_io_task.h>
#include <quantum/quantum_local.h>
#include <quantum/quantum_macros.h>
#include <quantum/quantum_mutex.h>
#include <quantum/quantum_promise.h>
#include <quantum/quantum_queue_statistics.h>
#include <quantum/quantum_read_write_spinlock.h>
#include <quantum/quantum_shared_state.h>
#include <quantum/quantum_spinlock.h>
#include <quantum/quantum_stack_allocator.h>
#include <quantum/quantum_stack_traits.h>
#include <quantum/quantum_task.h>
#include <quantum/quantum_task_queue.h>
#include <quantum/quantum_thread_traits.h>
#include <quantum/quantum_traits.h>
#include <quantum/quantum_yielding_thread.h>
#include <quantum/util/quantum_drain_guard.h>
#include <quantum/util/quantum_future_joiner.h>
#include <quantum/util/quantum_generic_future.h>
#include <quantum/util/quantum_local_variable_guard.h>
#include <quantum/util/quantum_sequence_key_statistics.h>
#include <quantum/util/quantum_sequencer.h>
#include <quantum/util/quantum_sequencer_configuration.h>
#include <quantum/util/quantum_util.h>

#endif //BLOOMBERG_QUANTUM_H
