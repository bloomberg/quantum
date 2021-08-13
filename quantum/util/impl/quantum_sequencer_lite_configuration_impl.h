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

#include <quantum/util/quantum_sequence_key_statistics.h>
#include <quantum/interface/quantum_icoro_context_base.h>
#include <list>
#include <vector>
#include <atomic>
#include <memory>

namespace Bloomberg {
namespace quantum {

template <class SequenceKey>
struct SequencerLiteKeyData;

template <class SequenceKey>
struct SequencerLiteTask
{
    SequencerLiteTask(std::function<int(VoidContextPtr)>&& func,
                      bool universal,
                      void* opaque,
                      int queueId,
                      bool isHighPriority) :
        _func(std::move(func)),
        _pendingKeyCount(0),
        _universal(universal),
        _opaque(opaque),
        _queueId(queueId),
        _isHighPriority(isHighPriority)
    {}

    std::function<int(VoidContextPtr)> _func; // the function to run
    std::vector<SequencerLiteKeyData<SequenceKey>*> _keyData; // pointers to the key data of my keys
    unsigned int _pendingKeyCount; // number of key queues where I am not at the head
    bool _universal; // true of universal tasks
    void* _opaque; // opaque pointer passed by user
    int _queueId; // the queue to enqueue the task
    bool _isHighPriority; // high priority task
};

template <class SequenceKey>
struct SequencerLiteKeyData
{
    SequencerLiteKeyData() :
        _stats(std::make_shared<SequenceKeyStatisticsWriter>())
    {}
    std::list<std::shared_ptr<SequencerLiteTask<SequenceKey>>> _tasks; // task queue
    std::shared_ptr<SequenceKeyStatisticsWriter> _stats; // stats for all tasks sharing this key
};

inline const std::string&
SequencerLiteConfigurationSchemaProvider::getJsonSchema()
{
    static std::string schema = R"JSON(
    {
        "$schema" : "http://json-schema.org/draft-04/schema#",
        "$id" : "bloomberg:sequencerlite.quantum.json",
        "title": "Quantum sequencerlite settings",
        "type": "object",
        "properties": {
            "bucketCount": {
                "type": "number",
                "default": 100
            }
        },
        "additionalProperties": false,
        "required": []
    }
    )JSON";
    return schema;
}

inline const std::string&
SequencerLiteConfigurationSchemaProvider::getJsonSchemaUri()
{
    static std::string uri = "bloomberg:sequencerlite.quantum.json";
    return uri;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>&
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::setBucketCount(size_t bucketCount)
{
    _bucketCount = bucketCount;
    return *this;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
size_t
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::getBucketCount() const
{
    return _bucketCount;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::setHash(const Hash& hash)
{
    _hash = hash;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
const Hash&
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::getHash() const
{
    return _hash;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::setKeyEqual(const KeyEqual& keyEqual)
{
    _keyEqual = keyEqual;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
const KeyEqual&
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::getKeyEqual() const
{
    return _keyEqual;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::setAllocator(const Allocator& allocator)
{
    _allocator = allocator;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
const Allocator&
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::getAllocator() const
{
    return _allocator;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
void
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::setExceptionCallback(
    const ExceptionCallback&
    exceptionCallback)

{
    _exceptionCallback = exceptionCallback;
}

template <class SequenceKey, class Hash, class KeyEqual, class Allocator>
const typename SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::ExceptionCallback&
SequencerLiteConfiguration<SequenceKey, Hash, KeyEqual, Allocator>::getExceptionCallback() const
{
    return _exceptionCallback;
}

}}
