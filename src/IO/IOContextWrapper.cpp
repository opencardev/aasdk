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
 * @file IOContextWrapper.cpp
 * @brief Context dispatch wrapper for promise creation on strands or services.
 *
 * IOContextWrapper abstracts either an io_service or strand for deferred
 * promise creation. Allows code to accept either execution context and create
 * promises that run callbacks in that context (thread-safe execution).
 *
 * Usage: Promise::defer(context) dispatches promise callback to appropriate
 * executor (io_service or strand) when promise resolves/rejects.
 *
 * Scenario: Promise resolution on specific thread
 *   - Messenger created with main io_service
 *   - receiveStrand_ wraps receive queue operations
 *   - enqueueReceive(channel, promise) dispatched to receiveStrand_
 *   - receiveStrand_ promises created with:
 *     IOContextWrapper(receiveStrand_) - ensures resolve/reject runs on strand
 *   - Promise resolves on receiveStrand_ thread; thread-safe queue access
 *   - Multiple threads can call enqueueReceive safely; serialised on strand
 *
 * Design:
 *   - Empty constructor: null context (for immediate resolution)
 *   - io_service constructor: promises execute on main thread pool
 *   - strand constructor: promises execute on strand (serialised)
 *   - isActive(): check if context is set (null = no async dispatch needed)
 *
 * Thread Safety: Wrapper is immutable once constructed. Can be copied and
 * passed between threads. Strand/io_service guarantee execution safety.
 */

#include <aasdk/IO/IOContextWrapper.hpp>


namespace aasdk {
  namespace io {

    IOContextWrapper::IOContextWrapper()
        : ioService_(nullptr), strand_(nullptr) {

    }

    IOContextWrapper::IOContextWrapper(boost::asio::io_service &ioService)
        : ioService_(&ioService), strand_(nullptr) {

    }

    IOContextWrapper::IOContextWrapper(boost::asio::io_service::strand &strand)
        : ioService_(nullptr), strand_(&strand) {

    }

    void IOContextWrapper::reset() {
      ioService_ = nullptr;
      strand_ = nullptr;
    }

    bool IOContextWrapper::isActive() const {
      return ioService_ != nullptr || strand_ != nullptr;
    }

  }
}
