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
#ifndef BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H
#define BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H

#include <functional>
#include <memory>
#include <stdexcept>

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
class SequencerLiteConfiguration : public SequencerLiteConfigurationSchemaProvider
{
public:
    /// @brief Callback for unhandled exceptions in tasks posted to Sequencer
    /// @param exception pointer to the thrown exception
    /// @param opaque opaque data passed when posting a task
    using ExceptionCallback = std::function<void(std::exception_ptr exception, void* opaque)>;

    /// @brief Sets the minimal number of buckets to be used for the context hash map
    /// @param bucketCount the bucket number
    /// @return A reference to itself
    SequencerLiteConfiguration& setBucketCount(size_t bucketCount);

    /// @brief gets the minimal number of buckets to be used for the context hash map
    /// @return the bucket number
    size_t getBucketCount() const;

    /// @brief Sets the hash function to be used for the context hash map
    /// @param hash the hash function
    void setHash(const Hash& hash);

    /// @brief Gets the hash function to be used for the context hash map
    /// @return the hash function
    const Hash& getHash() const;

    /// @brief Sets the comparison function to be used for all SequenceKey comparisons for the context hash map
    /// @param keyEqual the comparison function
    void setKeyEqual(const KeyEqual& keyEqual);

    /// @brief Gets the comparison function to be used for all SequenceKey comparisons for the context hash map
    /// @return the comparison function
    const KeyEqual& getKeyEqual() const;

    /// @brief Sets the allocator for all SequenceKey comparisons for the context hash map
    /// @param allocator the allocator
    void setAllocator(const Allocator& allocator);

    /// @brief Gets the allocator for all SequenceKey comparisons for the context hash map
    /// @return the allocator
    const Allocator& getAllocator() const;

    /// @brief Sets the exception callback for Scheduler
    /// @param exceptionCallback the callback to set
    void setExceptionCallback(const ExceptionCallback& exceptionCallback);

    /// @brief Gets the exception callback for Scheduler
    /// @return the current callback
    const ExceptionCallback& getExceptionCallback() const;

private:
    size_t              _bucketCount{100};
    Hash                _hash;
    KeyEqual            _keyEqual;
    Allocator           _allocator;
    ExceptionCallback   _exceptionCallback;
};

}}

#include <quantum/util/impl/quantum_sequencer_lite_configuration_impl.h>

#endif //BLOOMBERG_QUANTUM_SEQUENCER_LITE_CONFIGURATION_H
