// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef autonomylib_vehicles_MultirotorRpcLibAdaptors_hpp
#define autonomylib_vehicles_MultirotorRpcLibAdaptors_hpp

#include "api/RpcLibAdaptorsBase.hpp"
#include "common/Common.hpp"
#include "common/CommonStructs.hpp"
#include "common/ImageCaptureBase.hpp"
#include "safety/SafetyEval.hpp"
#include "vehicles/multirotor/api/MultirotorApiBase.hpp"
#include "vehicles/multirotor/api/MultirotorCommon.hpp"

#include "common/utils/WindowsApisCommonPost.hpp"
#include "common/utils/WindowsApisCommonPre.hpp"
#include "rpc/msgpack.hpp"

namespace nervosys {
namespace autonomylib_rpclib {

class MultirotorRpcLibAdaptors : public RpcLibAdaptorsBase {
  public:
    struct YawMode {
        bool is_rate = true;
        float yaw_or_rate = 0;
        MSGPACK_DEFINE_MAP(is_rate, yaw_or_rate);

        YawMode() {}

        YawMode(const nervosys::autonomylib::YawMode &s) {
            is_rate = s.is_rate;
            yaw_or_rate = s.yaw_or_rate;
        }
        nervosys::autonomylib::YawMode to() const { return nervosys::autonomylib::YawMode(is_rate, yaw_or_rate); }
    };

    struct RotorParameters {
        nervosys::autonomylib::real_T thrust;
        nervosys::autonomylib::real_T torque_scaler;
        nervosys::autonomylib::real_T speed;

        MSGPACK_DEFINE_MAP(thrust, torque_scaler, speed);

        RotorParameters() {}

        RotorParameters(const nervosys::autonomylib::RotorParameters &s) {
            thrust = s.thrust;
            torque_scaler = s.torque_scaler;
            speed = s.speed;
        }

        nervosys::autonomylib::RotorParameters to() const {
            return nervosys::autonomylib::RotorParameters(thrust, torque_scaler, speed);
        }
    };

    struct RotorStates {
        std::vector<RotorParameters> rotors;
        uint64_t timestamp;

        MSGPACK_DEFINE_MAP(rotors, timestamp);

        RotorStates() {}

        RotorStates(const nervosys::autonomylib::RotorStates &s) {
            for (const auto &r : s.rotors) {
                rotors.push_back(RotorParameters(r));
            }
            timestamp = s.timestamp;
        }

        nervosys::autonomylib::RotorStates to() const {
            std::vector<nervosys::autonomylib::RotorParameters> d;
            for (const auto &r : rotors) {
                d.push_back(r.to());
            }
            return nervosys::autonomylib::RotorStates(d, timestamp);
        }
    };

    struct MultirotorState {
        CollisionInfo collision;
        KinematicsState kinematics_estimated;
        KinematicsState kinematics_true;
        GeoPoint gps_location;
        uint64_t timestamp;
        nervosys::autonomylib::LandedState landed_state;
        RCData rc_data;
        bool ready;
        std::string ready_message;
        std::vector<std::string> controller_messages;
        bool can_arm;

        MSGPACK_DEFINE_MAP(collision, kinematics_estimated, gps_location, timestamp, landed_state, rc_data);

        MultirotorState() {}

        MultirotorState(const nervosys::autonomylib::MultirotorState &s) {
            collision = s.collision;
            kinematics_estimated = s.kinematics_estimated;
            gps_location = s.gps_location;
            timestamp = s.timestamp;
            landed_state = s.landed_state;
            rc_data = RCData(s.rc_data);
            ready = s.ready;
            ready_message = s.ready_message;
            can_arm = s.can_arm;
        }

        nervosys::autonomylib::MultirotorState to() const {
            return nervosys::autonomylib::MultirotorState(collision.to(), kinematics_estimated.to(), gps_location.to(),
                                                          timestamp, landed_state, rc_data.to(), ready, ready_message,
                                                          can_arm);
        }
    };
};

} // namespace autonomylib_rpclib
} // namespace nervosys

MSGPACK_ADD_ENUM(nervosys::autonomylib::DrivetrainType);
MSGPACK_ADD_ENUM(nervosys::autonomylib::LandedState);

#endif
