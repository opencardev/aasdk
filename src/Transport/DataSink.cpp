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

#include <cstring>
#include <aasdk/Transport/DataSink.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk {
  namespace transport {

    DataSink::DataSink()
        : data_(common::cStaticDataSize), pendingFill_(false) {
    }

    common::DataBuffer DataSink::fill() {
      if (pendingFill_) {
        // Safety net: a previous fill() never received a matching commit() — most
        // likely a USB transfer retry path where the transfer returned an error
        // before writing any data (e.g. EINTR, LIBUSB_ERROR_NO_DEVICE).  Roll back
        // that dangling allocation now so it can never be mistaken for real protocol
        // data by distributeReceivedData().  Callers on the retry path should also
        // call rollback() explicitly before fill() to document intent, but this
        // auto-rollback acts as a hard safety net for any path that misses that.
        if (data_.size() >= cChunkSize) {
          data_.erase_end(cChunkSize);
        }
      }
      pendingFill_ = true;

      const auto offset = data_.size();
      data_.resize(data_.size() + cChunkSize);

      auto ptr = data_.is_linearized() ? &data_[offset] : data_.linearize() + offset;
      return common::DataBuffer(ptr, cChunkSize);
    }

    void DataSink::commit(common::Data::size_type size) {
      if (size > cChunkSize) {
        throw error::Error(error::ErrorCode::DATA_SINK_COMMIT_OVERFLOW);
      }

      pendingFill_ = false;
      data_.erase_end((cChunkSize - size));
    }

    void DataSink::rollback() {
      if (!pendingFill_) {
        return;
      }
      // Only erase if there's actually a pending chunk to roll back.
      // Safety check: circular_buffer::erase_end(n) requires size() >= n.
      if (data_.size() >= cChunkSize) {
        data_.erase_end(cChunkSize);
      }
      pendingFill_ = false;
    }

    common::Data::size_type DataSink::getAvailableSize() {
      return data_.size();
    }

    common::Data DataSink::consume(common::Data::size_type size) {
      if (size > data_.size()) {
        throw error::Error(error::ErrorCode::DATA_SINK_CONSUME_UNDERFLOW);
      }

      common::Data data(size, 0);
      std::copy(data_.begin(), data_.begin() + size, data.begin());
      data_.erase_begin(size);

      return data;
    }

  }
}
