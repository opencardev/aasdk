// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
// Copyright (C) 2026 OpenCarDev (Matthew Hilton - matthilton2005@gmail.com)
//
// aasdk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// aasdk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

/**
 * @file Data.cpp
 * @brief Buffer management and binary data handling for protocol messages.
 *
 * Data types provide lightweight wrappers over raw byte buffers for transport
 * and message encoding/decoding. Supports zero-copy buffer views via DataBuffer
 * (mutable) and DataConstBuffer (read-only) with optional offset.
 *
 * Usage patterns:
 *   - Data: Owned vector of bytes (allocated/deallocated by Data)
 *   - DataBuffer: Non-owning mutable view (raw ptr + size + offset)
 *   - DataConstBuffer: Non-owning const view (for read-only operations)
 *
 * Scenario: Frame reception and parsing
 *   - T+0ms: Transport receives 1024 bytes into Data buffer (owns memory)
 *   - T+5ms: Messenger extracts 4-byte frame header:
 *     DataBuffer header(data, 1024, 0) - view of first 4 bytes
 *   - T+10ms: Parse header; determine payload size=500
 *   - T+10ms: Extract payload 500 bytes:
 *     DataBuffer payload(data, 1024, 4) - view of bytes 4-503
 *   - T+15ms: Create DataConstBuffer for protobuf parsing (read-only)
 *   - T+20ms: Protocol deserializes from const view; message constructed
 *   - Data buffer stays valid; multiple views created without copies
 *
 * Offset safety:
 *   - If offset > size: returns null buffer (size=0, data=nullptr)
 *   - Prevents out-of-bounds access; all buffer views validated at construction
 *
 * Thread Safety: Data is movable but not copyable; owns bytes. DataBuffer and
 * DataConstBuffer are lightweight views; caller responsible for buffer lifetime.
 */

#include <boost/algorithm/hex.hpp>
#include <aasdk/Common/Data.hpp>
#include <aasdk/Common/Log.hpp>
#include <string>
#include <sstream>
#include <iomanip>


namespace aasdk {
  namespace common {

    DataBuffer::DataBuffer()
        : data(nullptr), size(0) {

    }

    DataBuffer::DataBuffer(Data::value_type *_data, Data::size_type _size, Data::size_type offset) {
      if (offset > _size || _data == nullptr || _size == 0) {
        data = nullptr;
        size = 0;
      } else if (offset <= _size) {
        data = _data + offset;
        size = _size - offset;
      }
    }

    DataBuffer::DataBuffer(void *_data, Data::size_type _size, Data::size_type offset)
        : DataBuffer(reinterpret_cast<Data::value_type *>(_data), _size, offset) {

    }

    DataBuffer::DataBuffer(Data &_data, Data::size_type offset)
        : DataBuffer(_data.empty() ? nullptr : &_data[0], _data.size(), offset > _data.size() ? 0 : offset) {

    }

    bool DataBuffer::operator==(const std::nullptr_t &) const {
      return data == nullptr || size == 0;
    }

    bool DataBuffer::operator==(const DataBuffer &buffer) const {
      return data == buffer.data && size == buffer.size;
    }

    DataConstBuffer::DataConstBuffer()
        : cdata(nullptr), size(0) {

    }

    DataConstBuffer::DataConstBuffer(const DataBuffer &other)
        : cdata(other.data), size(other.size) {
    }

    DataConstBuffer::DataConstBuffer(const Data::value_type *_data, Data::size_type _size, Data::size_type offset) {
      if (offset > _size || _data == nullptr || _size == 0) {
        cdata = nullptr;
        size = 0;
      } else if (offset <= _size) {
        cdata = _data + offset;
        size = _size - offset;
      }
    }

    DataConstBuffer::DataConstBuffer(const void *_data, Data::size_type _size, Data::size_type offset)
        : DataConstBuffer(reinterpret_cast<const Data::value_type *>(_data), _size, offset) {

    }

    DataConstBuffer::DataConstBuffer(const Data &_data, Data::size_type offset)
        : DataConstBuffer(_data.empty() ? nullptr : &_data[0], _data.size(), offset > _data.size() ? 0 : offset) {

    }

    bool DataConstBuffer::operator==(const std::nullptr_t &) const {
      return cdata == nullptr || size == 0;
    }

    bool DataConstBuffer::operator==(const DataConstBuffer &buffer) const {
      return cdata == buffer.cdata && size == buffer.size;
    }

    common::Data createData(const DataConstBuffer &buffer) {
      common::Data data;
      copy(data, buffer);
      return data;
    }

    std::string dump(const Data &data) {
      return dump(DataConstBuffer(data));
    }


    std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
      std::stringstream ss;

      ss << std::hex << std::setfill('0');

      for (size_t i = 0; i < s; i++) {
        ss << " ";
        ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
      }

      return ss.str();
    }

    std::string dump(const DataConstBuffer &buffer) {
      if (buffer.size == 0) {
        return "[0] null";
      } else {
        std::string hexDump = "[" + uint8_to_hex_string(buffer.cdata, buffer.size) + " ] ";
        //boost::algorithm::hex(bufferBegin(buffer), bufferEnd(buffer), back_inserter(hexDump));
        return hexDump;
      }
    }

  }
}
