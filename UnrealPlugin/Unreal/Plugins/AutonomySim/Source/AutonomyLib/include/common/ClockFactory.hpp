// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef autonomylib_common_ClockFactory_hpp
#define autonomylib_common_ClockFactory_hpp

#include "ScalableClock.hpp"
#include <memory>

namespace nervosys {
namespace autonomylib {

class ClockFactory {
  public:
    // output of this function should not be stored as pointer might change
    static ClockBase *get(std::shared_ptr<ClockBase> val = nullptr) {
        static std::shared_ptr<ClockBase> clock;

        if (val != nullptr)
            clock = val;

        if (clock == nullptr)
            clock = std::make_shared<ScalableClock>();

        return clock.get();
    }

    // don't allow multiple instances of this class
    ClockFactory(ClockFactory const &) = delete;
    void operator=(ClockFactory const &) = delete;

  private:
    // disallow instance creation
    ClockFactory() {}
};

} // namespace autonomylib
} // namespace nervosys

#endif
