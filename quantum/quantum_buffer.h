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
#ifndef BLOOMBERG_QUANTUM_FUTURE_ITERATOR_H
#define BLOOMBERG_QUANTUM_FUTURE_ITERATOR_H

#include <iostream>
#include <deque>
#include <quantum/quantum_future_state.h>
#include <quantum/quantum_traits.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                       enum BufferStatus
//==============================================================================================
/// @enum BufferStatus
/// @brief Defines the result of the operation on the buffer object.
enum class BufferStatus
{
    DataReceived,   ///< Data has been successfully read from the buffer.
    DataPosted,     ///< Data has been successfully written to the buffer.
    DataPending,    ///< Buffer is empty and more data is on the way.
    Closed          ///< Buffer is closed. Push operations are not allowed. Pull operations are allowed until buffer is emtpy.
};

//==============================================================================================
//                                        class Buffer
//==============================================================================================
/// @class Buffer
/// @brief Container which allows buffered access to a series of values. Values are pushed-in (written)
///        by a producer(s) and pulled-out (read) by a consumer(s).
/// @tparam T Type of the contained value.
/// @warning This class is not thread safe.
template <class T, class ALLOCATOR>
class Buffer
{
public:
    using ValueType = T; ///< Type definition for the contained value.
    
    /// @brief Constructor
    Buffer(const ALLOCATOR& alloc = ALLOCATOR());
    
    /// @brief Pushes a value at the end of the buffer. This increases the size of the buffer by one.
    /// @tparam V Type of the contained value. Must be inferred and always equal to T.
    /// @param[in] value Value pushed into the buffer
    /// @return Result of the operation. See BufferStatus above for more details.
    template <class V = T>
    BufferStatus push(V&& value);
    
    /// @brief Pulls the next value from the buffer. This decreases the size of the buffer by one.
    /// @param[out] value Reference to the variable which will contain the next valid value if return value is 'DataReceived'.
    /// @return Result of the operation. See BufferStatus above for more details.
    BufferStatus pull(T& value);
    
    /// @brief Close the buffer. Once this method is called, push operations are no longer permitted. Pull operations
    ///        are permitted until the buffer empties.
    void close();
    
    /// @brief Indicates if the buffer is closed.
    bool isClosed() const;
    
    /// @brief Indicates the number of values stored in the buffer.
    /// @return Number of values in the buffer.
    size_t size() const;
    
    /// @brief Helper function equivalent to size() == 0;
    /// @return True if empty, false otherwise.
    bool empty() const;
    
private:
    std::deque<T,ALLOCATOR>     _buffer;
    bool                        _isClosed;
};

}}

#include <quantum/impl/quantum_buffer_impl.h>

#endif //BLOOMBERG_QUANTUM_FUTURE_ITERATOR_H
