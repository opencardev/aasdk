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

#pragma once

#include <limits>
#include <boost/circular_buffer.hpp>
#include <aasdk/Common/Data.hpp>


namespace aasdk::transport {

    class DataSink {
    public:
      DataSink();

      /// Allocate a cChunkSize slot at the tail of the buffer for the next USB
      /// receive transfer to write into.  If a previous fill() was never
      /// followed by commit() (e.g. the USB transfer returned an error), the
      /// dangling uncommitted slot is automatically rolled back first so that
      /// stale zero-bytes never appear in the protocol stream.
      common::DataBuffer fill();

      /// Finalise the most recent fill(): trim the unused tail of the slot to
      /// the actual number of bytes transferred and clear the pending flag.
      void commit(common::Data::size_type size);

      /// Discard the uncommitted slot from the most recent fill() that was
      /// never followed by a successful commit().  Safe to call even when no
      /// fill is pending (no-op in that case).
      void rollback();

      common::Data::size_type getAvailableSize();

      common::Data consume(common::Data::size_type size);

    private:
      boost::circular_buffer<common::Data::value_type> data_;
      /// True between a fill() call and its matching commit() or rollback().
      bool pendingFill_{false};
      static constexpr common::Data::size_type cChunkSize = 16384;
    };

}
