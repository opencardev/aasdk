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
 * @file Error.cpp
 * @brief AASDK error code and exception handling.
 *
 * Error encapsulates all failure conditions in AASDK library: USB device
 * errors, protocol violations, network failures, and internal failures.
 * Each error combines an ErrorCode enum (AASDK-specific) with a nativeCode
 * (OS/library-specific like libusb errno).
 *
 * Error categories:
 *   - USB errors (USB_DEVICE_NOT_FOUND, USB_CLAIM_INTERFACE, etc.)
 *   - Protocol errors (INVALID_CHANNEL, AUTHENTICATION_FAILURE, etc.)
 *   - Transport errors (TCP_TRANSFER, OPERATION_ABORTED)
 *   - Resource errors (OUT_OF_MEMORY)
 *
 * Usage: Promises reject with Error; callers check code or compare to
 * specific ErrorCode values:
 *   - promise->fail([](const Error& e) { if (e == ErrorCode::USB_DEVICE_NOT_FOUND) ... })
 *   - if (!error) { /* Success - code is NONE */ }
 *   - e.getNativeCode() retrieves OS error (libusb_error, errno, etc.)
 *
 * Scenario: Device disconnect handling
 *   - Messenger calls transport->receive()
 *   - USB cable unplugged; USB driver reports LIBUSB_ERROR_NO_DEVICE
 *   - receiveHandler called with error
 *   - Error created: code=USB_TRANSFER, nativeCode=LIBUSB_ERROR_NO_DEVICE
 *   - receivePromise->reject(error)
 *   - Navigation service promise rejected: "USB device disconnected"
 *   - UI displays error message; user can reconnect device
 *
 * Thread Safety: Error is immutable after construction; safe to copy and pass
 * across threads via promise chains.
 */

#include <aasdk/Error/Error.hpp>
#include <utility>


namespace aasdk {
  namespace error {

    Error::Error()
        : code_(ErrorCode::NONE), nativeCode_(0) {

    }

    Error::Error(ErrorCode code, uint32_t nativeCode, std::string information)
        : code_(code), nativeCode_(nativeCode), information_(std::move(information)) {
      message_ = "AASDK Error: " + std::to_string(static_cast<uint32_t>(code_))
                 + ", Native Code: " + std::to_string(nativeCode_)
                 + ", Additional Information: " + information_;
    }

    ErrorCode Error::getCode() const {
      return code_;
    }

    uint32_t Error::getNativeCode() const {
      return nativeCode_;
    }

    const char *Error::what() const noexcept {
      return message_.c_str();
    }

    bool Error::operator!() const {
      return code_ == ErrorCode::NONE;
    }

    bool Error::operator==(const Error &other) const {
      return code_ == other.code_ && nativeCode_ == other.nativeCode_;
    }

    bool Error::operator==(const ErrorCode &code) const {
      return code_ == code;
    }

    bool Error::operator!=(const ErrorCode &code) const {
      return !operator==(code);
    }

  }
}
