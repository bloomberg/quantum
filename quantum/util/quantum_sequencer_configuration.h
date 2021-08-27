/*
** Copyright 2021 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_QUANTUM_SEQUENCER_CONFIGURATION_H
#define BLOOMBERG_QUANTUM_SEQUENCER_CONFIGURATION_H

#include <quantum/util/quantum_sequencer_configuration_base.h>

namespace Bloomberg {
namespace quantum {

struct SequenceKeyData;

//==============================================================================================
//                                    class SequencerConfigurationSchemaProvider
//==============================================================================================
/// @class SequencerConfigurationSchemaProvider
/// @brief Provides static accessors to a json schema representing a SequencerConfiguration object
struct SequencerConfigurationSchemaProvider
{
    /// @brief Get the JSON schema corresponding to this configuration object.
    /// @return The draft-04 compatible schema.
    static const std::string& getJsonSchema();

    /// @brief Get the schema URI used to resolve remote JSON references '$ref'.
    /// @return The URI.
    static const std::string& getJsonSchemaUri();
};

//==============================================================================================
//                                      class SequencerConfiguration
//==============================================================================================
/// @class SequencerConfiguration.
/// @brief Implementation of a configuration class for Sequencer
/// @tparam SequenceKey Type of the key based that sequenced tasks are associated with in Sequencer
/// @tparam Hash Hash-function used for storing instances of SequenceKey in hash maps in Sequencer
/// @tparam KeyEqual The equal-function used for storing instances of SequenceKey in hash maps in Sequencer
/// @tparam Allocator The allocator used for storing instances of SequenceKey in hash maps in Sequencer
template <class SequenceKey,
          class Hash = std::hash<SequenceKey>,
          class KeyEqual = std::equal_to<SequenceKey>,
          class Allocator = std::allocator<std::pair<const SequenceKey, SequenceKeyData>>>
class SequencerConfiguration : public SequencerConfigurationBase<SequenceKey, Hash, KeyEqual, Allocator>,
    public SequencerConfigurationSchemaProvider
{
public:
    /// @brief Sets the id of the control queue
    /// @param controlQueueId the queue id
    /// @remark Sequencer typically processes tasks with the lower latency when the control queue is
    ///         dedicated for the sequencer control tasks only, and no other tasks are enqueued into it.
    /// @return A reference to itself
    SequencerConfiguration& setControlQueueId(int controlQueueId);

    /// @brief Gets the id of the control queue
    /// @return the queue id
    int getControlQueueId() const;

private:
    int                 _controllerQueueId{0};
};

}}

#include <quantum/util/impl/quantum_sequencer_configuration_impl.h>

#endif //BLOOMBERG_QUANTUM_SEQUENCER_CONFIGURATION_H
