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

/**
 * @file BackwardCompatibility.hpp
 * @brief Backward compatibility layer for AASDK API changes
 *
 * This header provides backward compatibility for code written against older
 * versions of AASDK. It includes:
 *
 * 1. Legacy namespace aliases (deprecated)
 * 2. Deprecated API compatibility
 * 3. Migration guidance
 *
 * @deprecated This file will be removed in AASDK v5.0.0
 * Please migrate to the new C++17 nested namespace syntax.
 *
 * Migration Guide:
 * - Old: namespace aasdk { namespace messenger { ... } }
 * - New: namespace aasdk::messenger { ... }
 *
 * - Old: typedef Type Alias;
 * - New: using Alias = Type;
 *
 * - Old: class MyClass : boost::noncopyable { ... }
 * - New: class MyClass { MyClass(const MyClass &) = delete; MyClass &operator=(const MyClass &) = delete; ... }
 *
 * - Old: Inheriting from boost::noncopyable for non-copyable classes
 * - New: Explicitly delete copy constructor and assignment operator
 *
 * Build system changes:
 * - Old AASDK (v4.x) always built protobuf from source and required absl
 * - New AASDK (v5.x) supports --skip-protobuf and --skip-absl options
 * - When using system protobuf v3.21.12+, absl is not required
 * - Migration: Use --skip-protobuf --skip-absl for system protobuf builds
 */

// Deprecation warning macro
#ifdef __GNUC__
#define AASDK_DEPRECATED __attribute__((deprecated("This API is deprecated. Migrate to C++17 nested namespace syntax: namespace aasdk::subnamespace. See BackwardCompatibility.hpp for migration guide.")))
#define AASDK_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#define AASDK_DEPRECATED_NONCOPYABLE __attribute__((deprecated("boost::noncopyable inheritance is deprecated. Use explicit copy operation deletion instead: Class(const Class &) = delete; Class &operator=(const Class &) = delete;")))
#elif defined(_MSC_VER)
#define AASDK_DEPRECATED __declspec(deprecated("This API is deprecated. Migrate to C++17 nested namespace syntax: namespace aasdk::subnamespace. See BackwardCompatibility.hpp for migration guide."))
#define AASDK_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#define AASDK_DEPRECATED_NONCOPYABLE __declspec(deprecated("boost::noncopyable inheritance is deprecated. Use explicit copy operation deletion instead: Class(const Class &) = delete; Class &operator=(const Class &) = delete;"))
#else
#define AASDK_DEPRECATED
#define AASDK_DEPRECATED_MSG(msg)
#define AASDK_DEPRECATED_NONCOPYABLE
#endif

// Include all modern headers to ensure they're available
#include <aasdk/Messenger/IMessenger.hpp>
#include <aasdk/Messenger/Message.hpp>
#include <aasdk/Messenger/Promise.hpp>
#include <aasdk/Common/Data.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/IO/Promise.hpp>

// Legacy namespace definitions for backward compatibility
// These provide the old nested namespace syntax that was used in AASDK v4.x
// DEPRECATED: These namespaces are deprecated. Use aasdk::subnamespace syntax instead.

namespace aasdk {
    namespace channel {  // DEPRECATED
        using namespace aasdk::channel;
    }

    namespace common {  // DEPRECATED
        using namespace aasdk::common;
    }

    namespace error {  // DEPRECATED
        using namespace aasdk::error;
    }

    namespace io {  // DEPRECATED
        using namespace aasdk::io;
    }

    namespace messenger {  // DEPRECATED
        using namespace aasdk::messenger;
    }

    namespace tcp {  // DEPRECATED
        using namespace aasdk::tcp;
    }

    namespace transport {  // DEPRECATED
        using namespace aasdk::transport;
    }

    namespace usb {  // DEPRECATED
        using namespace aasdk::usb;
    }
}

// Build system compatibility notes:
// - Old AASDK (v4.x) always built protobuf from source and required absl
// - New AASDK (v5.x) supports --skip-protobuf and --skip-absl options
// - When using system protobuf v3.21.12+, absl is not required
// - Migration: Use --skip-protobuf --skip-absl for system protobuf builds