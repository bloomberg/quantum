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
#ifndef BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H
#define BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H

#include <quantum/util/quantum_sequencer_configuration_base.h>

namespace Bloomberg {
namespace quantum {

template<typename SequenceKey>
struct SequencerLiteKeyData;

//==============================================================================================
//                                    class SequencerLiteConfigurationSchemaProvider
//==============================================================================================
// @class SequencerLiteConfigurationSchemaProvider
// @brief Provides static accessors to a json schema representing a SequencerLiteConfiguration object
struct SequencerLiteConfigurationSchemaProvider
{
    /// @brief Get the JSON schema corresponding to this configuration object.
    /// @return The draft-04 compatible schema.
    static const std::string& getJsonSchema();

    /// @brief Get the schema URI used to resolve remote JSON references '$ref'.
    /// @return The URI.
    static const std::string& getJsonSchemaUri();
};

//==============================================================================================
//                                      class SequencerLiteConfiguration
//==============================================================================================
/// @class SequencerLiteConfiguration.
/// @brief Implementation of a configuration class for SequencerLite
/// @tparam SequenceKey Type of the key based that sequenced tasks are associated with in SequencerLite
/// @tparam Hash Hash-function used for storing instances of SequenceKey in hash maps in SequencerLite
/// @tparam KeyEqual The equal-function used for storing instances of SequenceKey in hash maps in SequencerLite
/// @tparam Allocator The allocator used for storing instances of SequenceKey in hash maps in SequencerLite
template <class SequenceKey,
          class Hash = std::hash<SequenceKey>,
          class KeyEqual = std::equal_to<SequenceKey>,
          class Allocator = std::allocator<std::pair<const SequenceKey, SequencerLiteKeyData<SequenceKey>>>>
class SequencerLiteConfiguration : public SequencerConfigurationBase<SequenceKey, Hash, KeyEqual, Allocator>,
    public SequencerLiteConfigurationSchemaProvider
{};

}}

#include <quantum/util/impl/quantum_sequencer_lite_configuration_impl.h>

#endif //BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H
