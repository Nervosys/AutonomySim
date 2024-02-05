#include "common/common_utils/StrictMode.hpp"
STRICT_MODE_OFF // todo what does this do?
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "rpc/rpc_error.h"
    STRICT_MODE_ON

#include "AutonomySim_settings_parser.h"
#include "common/AutonomySimSettings.hpp"
#include "common/common_utils/FileSystem.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensors/imu/ImuBase.hpp"
#include "sensors/lidar/LidarSimpleParams.hpp"
#include "vehicles/car/api/CarRpcLibClient.hpp"
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "yaml-cpp/yaml.h"
#include <AutonomySim_interfaces/msg/altimeter.hpp> //hector_uav_msgs defunct?
#include <AutonomySim_interfaces/msg/car_controls.hpp>
#include <AutonomySim_interfaces/msg/car_state.hpp>
#include <AutonomySim_interfaces/msg/environment.hpp>
#include <AutonomySim_interfaces/msg/gimbal_angle_euler_cmd.hpp>
#include <AutonomySim_interfaces/msg/gimbal_angle_quat_cmd.hpp>
#include <AutonomySim_interfaces/msg/gps_yaw.hpp>
#include <AutonomySim_interfaces/msg/vel_cmd.hpp>
#include <AutonomySim_interfaces/msg/vel_cmd_group.hpp>
#include <AutonomySim_interfaces/srv/land.hpp>
#include <AutonomySim_interfaces/srv/land_group.hpp>
#include <AutonomySim_interfaces/srv/reset.hpp>
#include <AutonomySim_interfaces/srv/takeoff.hpp>
#include <AutonomySim_interfaces/srv/takeoff_group.hpp>
#include <chrono>
#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <image_transport/image_transport.hpp>
#include <iostream>
#include <math.h>
#include <math_common.h>
#include <mavros_msgs/msg/state.hpp>
#include <memory>
#include <nav_msgs/msg/odometry.hpp>
#include <opencv2/opencv.hpp>
#include <rosgraph_msgs/msg/clock.hpp>
#include <sensor_msgs/distortion_models.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/range.hpp>
#include <std_srvs/srv/empty.hpp>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/convert.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <unordered_map>

    struct SimpleMatrix {
    int rows;
    int cols;
    double *data;

    SimpleMatrix(int rows, int cols, double *data) : rows(rows), cols(cols), data(data) {}
};

struct VelCmd {
    double x;
    double y;
    double z;
    nervosys::autonomylib::DrivetrainType drivetrain;
    nervosys::autonomylib::YawMode yaw_mode;
    std::string vehicle_name;
};

struct GimbalCmd {
    std::string vehicle_name;
    std::string camera_name;
    nervosys::autonomylib::Quaternionr target_quat;
};

template <typename T> struct SensorPublisher {
    nervosys::autonomylib::SensorBase::SensorType sensor_type;
    std::string sensor_name;
    typename rclcpp::Publisher<T>::SharedPtr publisher;
};

class AutonomySimROSWrapper {
    using AutonomySimSettings = nervosys::autonomylib::AutonomySimSettings;
    using SensorBase = nervosys::autonomylib::SensorBase;
    using CameraSetting = nervosys::autonomylib::AutonomySimSettings::CameraSetting;
    using CaptureSetting = nervosys::autonomylib::AutonomySimSettings::CaptureSetting;
    using LidarSetting = nervosys::autonomylib::AutonomySimSettings::LidarSetting;
    using VehicleSetting = nervosys::autonomylib::AutonomySimSettings::VehicleSetting;
    using ImageRequest = nervosys::autonomylib::ImageCaptureBase::ImageRequest;
    using ImageResponse = nervosys::autonomylib::ImageCaptureBase::ImageResponse;
    using ImageType = nervosys::autonomylib::ImageCaptureBase::ImageType;

  public:
    enum class AutonomySim_MODE : unsigned { DRONE, CAR };

    AutonomySimROSWrapper(const std::shared_ptr<rclcpp::Node> nh, const std::shared_ptr<rclcpp::Node> nh_img,
                          const std::shared_ptr<rclcpp::Node> nh_lidar, const std::string &host_ip);
    ~AutonomySimROSWrapper(){};

    void initialize_AutonomySim();
    void initialize_ros();

    bool is_used_lidar_timer_cb_queue_;
    bool is_used_img_timer_cb_queue_;

  private:
    // utility struct for a SINGLE robot
    class VehicleROS {
      public:
        virtual ~VehicleROS() {}
        std::string vehicle_name_;

        /// All things ROS
        rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_local_pub_;
        rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr global_gps_pub_;
        rclcpp::Publisher<AutonomySim_interfaces::msg::Environment>::SharedPtr env_pub_;
        AutonomySim_interfaces::msg::Environment env_msg_;

        std::vector<SensorPublisher<AutonomySim_interfaces::msg::Altimeter>> barometer_pubs_;
        std::vector<SensorPublisher<sensor_msgs::msg::Imu>> imu_pubs_;
        std::vector<SensorPublisher<sensor_msgs::msg::NavSatFix>> gps_pubs_;
        std::vector<SensorPublisher<sensor_msgs::msg::MagneticField>> magnetometer_pubs_;
        std::vector<SensorPublisher<sensor_msgs::msg::Range>> distance_pubs_;
        std::vector<SensorPublisher<sensor_msgs::msg::PointCloud2>> lidar_pubs_;

        // handle lidar seperately for max performance as data is collected on its own thread/callback

        nav_msgs::msg::Odometry curr_odom_;
        sensor_msgs::msg::NavSatFix gps_sensor_msg_;

        std::vector<geometry_msgs::msg::TransformStamped> static_tf_msg_vec_;

        rclcpp::Time stamp_;

        std::string odom_frame_id_;
    };

    class CarROS : public VehicleROS {
      public:
        nervosys::autonomylib::CarApiBase::CarState curr_car_state_;

        rclcpp::Subscription<AutonomySim_interfaces::msg::CarControls>::SharedPtr car_cmd_sub_;
        rclcpp::Publisher<AutonomySim_interfaces::msg::CarState>::SharedPtr car_state_pub_;
        AutonomySim_interfaces::msg::CarState car_state_msg_;

        bool has_car_cmd_;
        nervosys::autonomylib::CarApiBase::CarControls car_cmd_;
    };

    class MultirotorROS : public VehicleROS {
      public:
        /// State
        nervosys::autonomylib::MultirotorState curr_drone_state_;

        rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmd>::SharedPtr vel_cmd_body_frame_sub_;
        rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmd>::SharedPtr vel_cmd_world_frame_sub_;

        rclcpp::Service<AutonomySim_interfaces::srv::Takeoff>::SharedPtr takeoff_srvr_;
        rclcpp::Service<AutonomySim_interfaces::srv::Land>::SharedPtr land_srvr_;

        bool has_vel_cmd_;
        VelCmd vel_cmd_;
    };

    /// ROS timer callbacks
    void img_response_timer_cb(); // update images from AutonomySim_client_ every nth sec
    void drone_state_timer_cb();  // update drone state from AutonomySim_client_ every nth sec
    void lidar_timer_cb();

    /// ROS subscriber callbacks
    void vel_cmd_world_frame_cb(const AutonomySim_interfaces::msg::VelCmd::SharedPtr msg,
                                const std::string &vehicle_name);
    void vel_cmd_body_frame_cb(const AutonomySim_interfaces::msg::VelCmd::SharedPtr msg,
                               const std::string &vehicle_name);

    void vel_cmd_group_body_frame_cb(const AutonomySim_interfaces::msg::VelCmdGroup::SharedPtr msg);
    void vel_cmd_group_world_frame_cb(const AutonomySim_interfaces::msg::VelCmdGroup::SharedPtr msg);

    void vel_cmd_all_world_frame_cb(const AutonomySim_interfaces::msg::VelCmd::SharedPtr msg);
    void vel_cmd_all_body_frame_cb(const AutonomySim_interfaces::msg::VelCmd::SharedPtr msg);

    // void vel_cmd_body_frame_cb(const AutonomySim_interfaces::msg::VelCmd& msg, const std::string& vehicle_name);
    void gimbal_angle_quat_cmd_cb(
        const AutonomySim_interfaces::msg::GimbalAngleQuatCmd::SharedPtr gimbal_angle_quat_cmd_msg);
    void gimbal_angle_euler_cmd_cb(
        const AutonomySim_interfaces::msg::GimbalAngleEulerCmd::SharedPtr gimbal_angle_euler_cmd_msg);

    // commands
    void car_cmd_cb(const AutonomySim_interfaces::msg::CarControls::SharedPtr msg, const std::string &vehicle_name);
    void update_commands();

    // state, returns the simulation timestamp best guess based on drone state timestamp, AutonomySim needs to return
    // timestap for environment
    rclcpp::Time update_state();
    void update_and_publish_static_transforms(VehicleROS *vehicle_ros);
    void publish_vehicle_state();

    /// ROS service callbacks
    bool takeoff_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::Takeoff::Request> request,
                        const std::shared_ptr<AutonomySim_interfaces::srv::Takeoff::Response> response,
                        const std::string &vehicle_name);
    bool takeoff_group_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::TakeoffGroup::Request> request,
                              const std::shared_ptr<AutonomySim_interfaces::srv::TakeoffGroup::Response> response);
    bool takeoff_all_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::Takeoff::Request> request,
                            const std::shared_ptr<AutonomySim_interfaces::srv::Takeoff::Response> response);
    bool land_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::Land::Request> request,
                     const std::shared_ptr<AutonomySim_interfaces::srv::Land::Response> response,
                     const std::string &vehicle_name);
    bool land_group_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::LandGroup::Request> request,
                           const std::shared_ptr<AutonomySim_interfaces::srv::LandGroup::Response> response);
    bool land_all_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::Land::Request> request,
                         const std::shared_ptr<AutonomySim_interfaces::srv::Land::Response> response);
    bool reset_srv_cb(const std::shared_ptr<AutonomySim_interfaces::srv::Reset::Request> request,
                      const std::shared_ptr<AutonomySim_interfaces::srv::Reset::Response> response);

    /// ROS tf broadcasters
    void publish_camera_tf(const ImageResponse &img_response, const rclcpp::Time &ros_time, const std::string &frame_id,
                           const std::string &child_frame_id);
    void publish_odom_tf(const nav_msgs::msg::Odometry &odom_msg);

    /// camera helper methods
    sensor_msgs::msg::CameraInfo generate_cam_info(const std::string &camera_name, const CameraSetting &camera_setting,
                                                   const CaptureSetting &capture_setting) const;

    std::shared_ptr<sensor_msgs::msg::Image> get_img_msg_from_response(const ImageResponse &img_response,
                                                                       const rclcpp::Time curr_ros_time,
                                                                       const std::string frame_id);
    std::shared_ptr<sensor_msgs::msg::Image> get_depth_img_msg_from_response(const ImageResponse &img_response,
                                                                             const rclcpp::Time curr_ros_time,
                                                                             const std::string frame_id);

    void process_and_publish_img_response(const std::vector<ImageResponse> &img_response_vec,
                                          const int img_response_idx, const std::string &vehicle_name);

    // methods which parse setting json ang generate ros pubsubsrv
    void create_ros_pubs_from_settings_json();
    void convert_tf_msg_to_enu(geometry_msgs::msg::TransformStamped &tf_msg);
    void append_static_camera_tf(VehicleROS *vehicle_ros, const std::string &camera_name,
                                 const CameraSetting &camera_setting);
    void append_static_lidar_tf(VehicleROS *vehicle_ros, const std::string &lidar_name,
                                const nervosys::autonomylib::LidarSimpleParams &lidar_setting);
    void append_static_vehicle_tf(VehicleROS *vehicle_ros, const VehicleSetting &vehicle_setting);
    void set_nans_to_zeros_in_pose(VehicleSetting &vehicle_setting) const;
    void set_nans_to_zeros_in_pose(const VehicleSetting &vehicle_setting, CameraSetting &camera_setting) const;
    void set_nans_to_zeros_in_pose(const VehicleSetting &vehicle_setting, LidarSetting &lidar_setting) const;
    geometry_msgs::msg::Transform
    get_camera_optical_tf_from_body_tf(const geometry_msgs::msg::Transform &body_tf) const;

    /// utils. todo parse into an Airlib<->ROS conversion class
    tf2::Quaternion get_tf2_quat(const nervosys::autonomylib::Quaternionr &airlib_quat) const;
    nervosys::autonomylib::Quaternionr get_airlib_quat(const geometry_msgs::msg::Quaternion &geometry_msgs_quat) const;
    nervosys::autonomylib::Quaternionr get_airlib_quat(const tf2::Quaternion &tf2_quat) const;
    nav_msgs::msg::Odometry
    get_odom_msg_from_kinematic_state(const nervosys::autonomylib::Kinematics::State &kinematics_estimated) const;
    nav_msgs::msg::Odometry get_odom_msg_from_multirotor_state(const nervosys::autonomylib::MultirotorState &drone_state) const;
    nav_msgs::msg::Odometry get_odom_msg_from_car_state(const nervosys::autonomylib::CarApiBase::CarState &car_state) const;
    AutonomySim_interfaces::msg::CarState
    get_roscarstate_msg_from_car_state(const nervosys::autonomylib::CarApiBase::CarState &car_state) const;
    nervosys::autonomylib::Pose get_airlib_pose(const float &x, const float &y, const float &z,
                                      const nervosys::autonomylib::Quaternionr &airlib_quat) const;
    AutonomySim_interfaces::msg::GPSYaw
    get_gps_msg_from_AutonomySim_geo_point(const nervosys::autonomylib::GeoPoint &geo_point) const;
    sensor_msgs::msg::NavSatFix
    get_gps_sensor_msg_from_AutonomySim_geo_point(const nervosys::autonomylib::GeoPoint &geo_point) const;
    sensor_msgs::msg::Imu get_imu_msg_from_AutonomySim(const nervosys::autonomylib::ImuBase::Output &imu_data) const;
    AutonomySim_interfaces::msg::Altimeter
    get_altimeter_msg_from_AutonomySim(const nervosys::autonomylib::BarometerBase::Output &alt_data) const;
    sensor_msgs::msg::Range get_range_from_AutonomySim(const nervosys::autonomylib::DistanceSensorData &dist_data) const;
    sensor_msgs::msg::PointCloud2 get_lidar_msg_from_AutonomySim(const nervosys::autonomylib::LidarData &lidar_data,
                                                                 const std::string &vehicle_name,
                                                                 const std::string &sensor_name) const;
    sensor_msgs::msg::NavSatFix get_gps_msg_from_AutonomySim(const nervosys::autonomylib::GpsBase::Output &gps_data) const;
    sensor_msgs::msg::MagneticField
    get_mag_msg_from_AutonomySim(const nervosys::autonomylib::MagnetometerBase::Output &mag_data) const;
    AutonomySim_interfaces::msg::Environment
    get_environment_msg_from_AutonomySim(const nervosys::autonomylib::Environment::State &env_data) const;
    nervosys::autonomylib::GeoPoint get_origin_geo_point() const;
    VelCmd get_airlib_world_vel_cmd(const AutonomySim_interfaces::msg::VelCmd &msg) const;
    VelCmd get_airlib_body_vel_cmd(const AutonomySim_interfaces::msg::VelCmd &msg,
                                   const nervosys::autonomylib::Quaternionr &airlib_quat) const;
    geometry_msgs::msg::Transform
    get_transform_msg_from_AutonomySim(const nervosys::autonomylib::Vector3r &position,
                                       const nervosys::autonomylib::AutonomySimSettings::Rotation &rotation);
    geometry_msgs::msg::Transform get_transform_msg_from_AutonomySim(const nervosys::autonomylib::Vector3r &position,
                                                                     const nervosys::autonomylib::Quaternionr &quaternion);

    // not used anymore, but can be useful in future with an unreal camera calibration environment
    void read_params_from_yaml_and_fill_cam_info_msg(const std::string &file_name,
                                                     sensor_msgs::msg::CameraInfo &cam_info) const;
    void convert_yaml_to_simple_mat(const YAML::Node &node, SimpleMatrix &m) const; // todo ugly

    template <typename T>
    const SensorPublisher<T> create_sensor_publisher(const std::string &sensor_type_name,
                                                     const std::string &sensor_name, SensorBase::SensorType sensor_type,
                                                     const std::string &topic_name, int QoS);

  private:
    // subscriber / services for ALL robots
    rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmd>::SharedPtr vel_cmd_all_body_frame_sub_;
    rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmd>::SharedPtr vel_cmd_all_world_frame_sub_;
    rclcpp::Service<AutonomySim_interfaces::srv::Takeoff>::SharedPtr takeoff_all_srvr_;
    rclcpp::Service<AutonomySim_interfaces::srv::Land>::SharedPtr land_all_srvr_;

    // todo - subscriber / services for a GROUP of robots, which is defined by a list of `vehicle_name`s passed in the
    // ros msg / srv request
    rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmdGroup>::SharedPtr vel_cmd_group_body_frame_sub_;
    rclcpp::Subscription<AutonomySim_interfaces::msg::VelCmdGroup>::SharedPtr vel_cmd_group_world_frame_sub_;
    rclcpp::Service<AutonomySim_interfaces::srv::TakeoffGroup>::SharedPtr takeoff_group_srvr_;
    rclcpp::Service<AutonomySim_interfaces::srv::LandGroup>::SharedPtr land_group_srvr_;

    AutonomySim_MODE AutonomySim_mode_ = AutonomySim_MODE::DRONE;

    rclcpp::Service<AutonomySim_interfaces::srv::Reset>::SharedPtr reset_srvr_;
    rclcpp::Publisher<AutonomySim_interfaces::msg::GPSYaw>::SharedPtr origin_geo_point_pub_; // home geo coord of drones
    nervosys::autonomylib::GeoPoint origin_geo_point_;                   // gps coord of unreal origin
    AutonomySim_interfaces::msg::GPSYaw origin_geo_point_msg_; // todo duplicate

    AutonomySimSettingsParser AutonomySim_settings_parser_;
    std::unordered_map<std::string, std::unique_ptr<VehicleROS>> vehicle_name_ptr_map_;
    static const std::unordered_map<int, std::string> image_type_int_to_string_map_;

    bool is_vulkan_; // rosparam obtained from launch file. If vulkan is being used, we BGR encoding instead of RGB

    std::string host_ip_;
    std::unique_ptr<nervosys::autonomylib::RpcLibClientBase> AutonomySim_client_;
    // seperate busy connections to AutonomySim, update in their own thread
    nervosys::autonomylib::RpcLibClientBase AutonomySim_client_images_;
    nervosys::autonomylib::RpcLibClientBase AutonomySim_client_lidar_;

    std::shared_ptr<rclcpp::Node> nh_;
    std::shared_ptr<rclcpp::Node> nh_img_;
    std::shared_ptr<rclcpp::Node> nh_lidar_;

    // todo not sure if async spinners shuold be inside this class, or should be instantiated in AutonomySim_node.cpp,
    // and cb queues should be public todo for multiple drones with multiple sensors, this won't scale. make it a part
    // of VehicleROS?

    std::mutex control_mutex_;

    // gimbal control
    bool has_gimbal_cmd_;
    GimbalCmd gimbal_cmd_;

    /// ROS tf
    const std::string AutonomySim_FRAME_ID = "world_ned";
    std::string world_frame_id_ = AutonomySim_FRAME_ID;
    const std::string AutonomySim_ODOM_FRAME_ID = "odom_local_ned";
    const std::string ENU_ODOM_FRAME_ID = "odom_local_enu";
    std::string odom_frame_id_ = AutonomySim_ODOM_FRAME_ID;
    std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    std::shared_ptr<tf2_ros::StaticTransformBroadcaster> static_tf_pub_;

    bool isENU_;
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    /// ROS params
    double vel_cmd_duration_;

    /// ROS Timers.
    rclcpp::TimerBase::SharedPtr AutonomySim_img_response_timer_;
    rclcpp::TimerBase::SharedPtr AutonomySim_control_update_timer_;
    rclcpp::TimerBase::SharedPtr AutonomySim_lidar_update_timer_;

    /// Callback groups
    std::vector<rclcpp::CallbackGroup::SharedPtr> AutonomySim_img_callback_groups_;
    rclcpp::CallbackGroup::SharedPtr AutonomySim_control_callback_group_;
    std::vector<rclcpp::CallbackGroup::SharedPtr> AutonomySim_lidar_callback_groups_;

    typedef std::pair<std::vector<ImageRequest>, std::string> AutonomySim_img_request_vehicle_name_pair;
    std::vector<AutonomySim_img_request_vehicle_name_pair> AutonomySim_img_request_vehicle_name_pair_vec_;
    std::vector<image_transport::Publisher> image_pub_vec_;
    std::vector<rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr> cam_info_pub_vec_;

    std::vector<sensor_msgs::msg::CameraInfo> camera_info_msg_vec_;

    /// ROS other publishers
    rclcpp::Publisher<rosgraph_msgs::msg::Clock>::SharedPtr clock_pub_;
    rosgraph_msgs::msg::Clock ros_clock_;
    bool publish_clock_;

    rclcpp::Subscription<AutonomySim_interfaces::msg::GimbalAngleQuatCmd>::SharedPtr gimbal_angle_quat_cmd_sub_;
    rclcpp::Subscription<AutonomySim_interfaces::msg::GimbalAngleEulerCmd>::SharedPtr gimbal_angle_euler_cmd_sub_;

    static constexpr char CAM_YML_NAME[] = "camera_name";
    static constexpr char WIDTH_YML_NAME[] = "image_width";
    static constexpr char HEIGHT_YML_NAME[] = "image_height";
    static constexpr char K_YML_NAME[] = "camera_matrix";
    static constexpr char D_YML_NAME[] = "distortion_coefficients";
    static constexpr char R_YML_NAME[] = "rectification_matrix";
    static constexpr char P_YML_NAME[] = "projection_matrix";
    static constexpr char DMODEL_YML_NAME[] = "distortion_model";
};
