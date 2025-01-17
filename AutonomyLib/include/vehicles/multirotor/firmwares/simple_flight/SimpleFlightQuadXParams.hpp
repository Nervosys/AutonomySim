// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef autonomylib_vehicles_SimpleFlightQuadXParams_hpp
#define autonomylib_vehicles_SimpleFlightQuadXParams_hpp

#include "common/AutonomySimSettings.hpp"
#include "sensors/SensorFactory.hpp"
#include "vehicles/multirotor/MultirotorParams.hpp"
#include "vehicles/multirotor/firmwares/simple_flight/SimpleFlightApi.hpp"

namespace nervosys {
namespace autonomylib {

class SimpleFlightQuadXParams : public MultirotorParams {

  private:
    const AutonomySimSettings::VehicleSetting *vehicle_setting_; // store as pointer because of derived classes
    std::shared_ptr<const SensorFactory> sensor_factory_;

  protected:
    virtual void setupParams() override {
        auto &params = getParams();
        // Use connection_info_.model for the model name, see Px4MultirotorParams for example
        // Only Generic for now
        setupFrameGenericQuad(params);
    }

    virtual const SensorFactory *getSensorFactory() const override { return sensor_factory_.get(); }

  public:
    SimpleFlightQuadXParams(const AutonomySimSettings::VehicleSetting *vehicle_setting,
                            std::shared_ptr<const SensorFactory> sensor_factory)
        : vehicle_setting_(vehicle_setting), sensor_factory_(sensor_factory) {}

    virtual ~SimpleFlightQuadXParams() = default;

    virtual std::unique_ptr<MultirotorApiBase> createMultirotorApi() override {
        return std::unique_ptr<MultirotorApiBase>(new SimpleFlightApi(this, vehicle_setting_));
    }
};

} // namespace autonomylib
} // namespace nervosys

#endif
