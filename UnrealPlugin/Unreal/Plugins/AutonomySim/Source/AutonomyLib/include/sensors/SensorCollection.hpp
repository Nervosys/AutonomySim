// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef autonomylib_sensors_SensorCollection_hpp
#define autonomylib_sensors_SensorCollection_hpp

#include "common/Common.hpp"
#include "common/UpdatableContainer.hpp"
#include "sensors/SensorBase.hpp"
#include <unordered_map>

namespace nervosys {
namespace autonomylib {

class SensorCollection : public UpdatableObject {
  public: // types
    typedef SensorBase *SensorBasePtr;

  public:
    void insert(SensorBasePtr sensor, SensorBase::SensorType type) {
        auto type_int = static_cast<uint>(type);
        const auto &it = sensors_.find(type_int);
        if (it == sensors_.end()) {
            const auto &pair = sensors_.emplace(type_int, unique_ptr<SensorBaseContainer>(new SensorBaseContainer()));
            pair.first->second->insert(sensor);
        } else {
            it->second->insert(sensor);
        }
    }

    const SensorBase *getByType(SensorBase::SensorType type, uint index = 0) const {
        auto type_int = static_cast<uint>(type);
        const auto &it = sensors_.find(type_int);
        if (it == sensors_.end()) {
            return nullptr;
        } else {
            return it->second->at(index);
        }
    }

    uint size(SensorBase::SensorType type) const {
        auto type_int = static_cast<uint>(type);
        const auto &it = sensors_.find(type_int);
        if (it == sensors_.end()) {
            return 0;
        } else {
            return it->second->size();
        }
    }

    void initialize(const Kinematics::State *kinematics, const Environment *environment) {
        for (auto &pair : sensors_) {
            for (auto &sensor : *pair.second) {
                sensor->initialize(kinematics, environment);
            }
        }
    }

    void clear() { sensors_.clear(); }

    //*** Start: UpdatableState implementation ***//
    virtual void resetImplementation() override {
        for (auto &pair : sensors_) {
            pair.second->reset();
        }
    }

    virtual void update() override {
        UpdatableObject::update();

        for (auto &pair : sensors_) {
            pair.second->update();
        }
    }

    virtual void reportState(StateReporter &reporter) override {
        for (auto &pair : sensors_) {
            pair.second->reportState(reporter);
        }
    }
    //*** End: UpdatableState implementation ***//

  private:
    typedef UpdatableContainer<SensorBasePtr> SensorBaseContainer;
    unordered_map<uint, unique_ptr<SensorBaseContainer>> sensors_;
};

} // namespace autonomylib
} // namespace nervosys

#endif
