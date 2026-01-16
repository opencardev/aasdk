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

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/IO/IOContextWrapper.hpp>


namespace aasdk::io {

// Generic promise that resolves with a value and rejects with an error.
template<typename ResolveArgumentType, typename ErrorArgumentType = error::Error>
class Promise {
public:
  Promise(const Promise &) = delete;
  Promise &operator=(const Promise &) = delete;

  using ValueType = ResolveArgumentType;
  using ErrorType = ErrorArgumentType;
  using ResolveHandler = std::function<void(ResolveArgumentType)>;
  using RejectHandler = std::function<void(ErrorArgumentType)>;
  using Pointer = std::shared_ptr<Promise>;

  static Pointer defer(boost::asio::io_service &ioService) {
    return std::make_shared<Promise>(ioService);
  }

  static Pointer defer(boost::asio::io_service::strand &strand) {
    return std::make_shared<Promise>(strand);
  }

  explicit Promise(boost::asio::io_service &ioService)
      : ioContextWrapper_(ioService) {}

  explicit Promise(boost::asio::io_service::strand &strand)
      : ioContextWrapper_(strand) {}

  void then(ResolveHandler resolveHandler, RejectHandler rejectHandler = RejectHandler()) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    resolveHandler_ = std::move(resolveHandler);
    rejectHandler_ = std::move(rejectHandler);
  }

  void resolve(ResolveArgumentType argument) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (resolveHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post(
          [argument = std::move(argument), resolveHandler = std::move(resolveHandler_)]() mutable {
            resolveHandler(std::move(argument));
          });
    }

    ioContextWrapper_.reset();
    rejectHandler_ = RejectHandler();
  }

  void reject(ErrorArgumentType error) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (rejectHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([error = std::move(error), rejectHandler = std::move(rejectHandler_)]() mutable {
        rejectHandler(std::move(error));
      });
    }

    ioContextWrapper_.reset();
    resolveHandler_ = ResolveHandler();
  }

private:
  bool isPending() const {
    return ioContextWrapper_.isActive();
  }

  ResolveHandler resolveHandler_;
  RejectHandler rejectHandler_;
  IOContextWrapper ioContextWrapper_;
  std::mutex mutex_;
};

// Promise that resolves with void and rejects with an error.
template<typename ErrorArgumentType>
class Promise<void, ErrorArgumentType> {
public:
  Promise(const Promise &) = delete;
  Promise &operator=(const Promise &) = delete;

  using ErrorType = ErrorArgumentType;
  using ResolveHandler = std::function<void()>;
  using RejectHandler = std::function<void(ErrorArgumentType)>;
  using Pointer = std::shared_ptr<Promise>;

  static Pointer defer(boost::asio::io_service &ioService) {
    return std::make_shared<Promise>(ioService);
  }

  static Pointer defer(boost::asio::io_service::strand &strand) {
    return std::make_shared<Promise>(strand);
  }

  explicit Promise(boost::asio::io_service &ioService)
      : ioContextWrapper_(ioService) {}

  explicit Promise(boost::asio::io_service::strand &strand)
      : ioContextWrapper_(strand) {}

  void then(ResolveHandler resolveHandler, RejectHandler rejectHandler = RejectHandler()) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    resolveHandler_ = std::move(resolveHandler);
    rejectHandler_ = std::move(rejectHandler);
  }

  void resolve() {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (resolveHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([resolveHandler = std::move(resolveHandler_)]() mutable {
        resolveHandler();
      });
    }

    ioContextWrapper_.reset();
    rejectHandler_ = RejectHandler();
  }

  void reject(ErrorArgumentType error) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (rejectHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([error = std::move(error), rejectHandler = std::move(rejectHandler_)]() mutable {
        rejectHandler(std::move(error));
      });
    }

    ioContextWrapper_.reset();
    resolveHandler_ = ResolveHandler();
  }

private:
  bool isPending() const {
    return ioContextWrapper_.isActive();
  }

  ResolveHandler resolveHandler_;
  RejectHandler rejectHandler_;
  IOContextWrapper ioContextWrapper_;
  std::mutex mutex_;
};

// Promise that resolves and rejects with void.
template<>
class Promise<void, void> {
public:
  Promise(const Promise &) = delete;
  Promise &operator=(const Promise &) = delete;

  using ResolveHandler = std::function<void()>;
  using RejectHandler = std::function<void()>;
  using Pointer = std::shared_ptr<Promise>;

  static Pointer defer(boost::asio::io_service &ioService) {
    return std::make_shared<Promise>(ioService);
  }

  static Pointer defer(boost::asio::io_service::strand &strand) {
    return std::make_shared<Promise>(strand);
  }

  explicit Promise(boost::asio::io_service &ioService)
      : ioContextWrapper_(ioService) {}

  explicit Promise(boost::asio::io_service::strand &strand)
      : ioContextWrapper_(strand) {}

  void then(ResolveHandler resolveHandler, RejectHandler rejectHandler = RejectHandler()) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    resolveHandler_ = std::move(resolveHandler);
    rejectHandler_ = std::move(rejectHandler);
  }

  void resolve() {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (resolveHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([resolveHandler = std::move(resolveHandler_)]() mutable {
        resolveHandler();
      });
    }

    ioContextWrapper_.reset();
    rejectHandler_ = RejectHandler();
  }

  void reject() {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (rejectHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([rejectHandler = std::move(rejectHandler_)]() mutable {
        rejectHandler();
      });
    }

    ioContextWrapper_.reset();
    resolveHandler_ = ResolveHandler();
  }

private:
  bool isPending() const {
    return ioContextWrapper_.isActive();
  }

  ResolveHandler resolveHandler_;
  RejectHandler rejectHandler_;
  IOContextWrapper ioContextWrapper_;
  std::mutex mutex_;
};

// Promise that resolves with a value and rejects with void.
template<typename ResolveArgumentType>
class Promise<ResolveArgumentType, void> {
public:
  Promise(const Promise &) = delete;
  Promise &operator=(const Promise &) = delete;

  using ValueType = ResolveArgumentType;
  using ResolveHandler = std::function<void(ResolveArgumentType)>;
  using RejectHandler = std::function<void()>;
  using Pointer = std::shared_ptr<Promise>;

  static Pointer defer(boost::asio::io_service &ioService) {
    return std::make_shared<Promise>(ioService);
  }

  static Pointer defer(boost::asio::io_service::strand &strand) {
    return std::make_shared<Promise>(strand);
  }

  explicit Promise(boost::asio::io_service &ioService)
      : ioContextWrapper_(ioService) {}

  explicit Promise(boost::asio::io_service::strand &strand)
      : ioContextWrapper_(strand) {}

  void then(ResolveHandler resolveHandler, RejectHandler rejectHandler = RejectHandler()) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    resolveHandler_ = std::move(resolveHandler);
    rejectHandler_ = std::move(rejectHandler);
  }

  void resolve(ResolveArgumentType argument) {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (resolveHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post(
          [argument = std::move(argument), resolveHandler = std::move(resolveHandler_)]() mutable {
            resolveHandler(std::move(argument));
          });
    }

    ioContextWrapper_.reset();
    rejectHandler_ = RejectHandler();
  }

  void reject() {
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if (rejectHandler_ != nullptr && isPending()) {
      ioContextWrapper_.post([rejectHandler = std::move(rejectHandler_)]() mutable {
        rejectHandler();
      });
    }

    ioContextWrapper_.reset();
    resolveHandler_ = ResolveHandler();
  }

private:
  bool isPending() const {
    return ioContextWrapper_.isActive();
  }

  ResolveHandler resolveHandler_;
  RejectHandler rejectHandler_;
  IOContextWrapper ioContextWrapper_;
  std::mutex mutex_;
};

}
