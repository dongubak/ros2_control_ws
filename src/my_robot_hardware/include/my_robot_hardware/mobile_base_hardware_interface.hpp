#ifndef MOBILE_BASE_HARDWARE_INTERFACE_HPP
#define MOBILE_BASE_HARDWARE_INTERFACE_HPP

#include "hardware_interface/system_interface.hpp"
#include "my_robot_hardware/xl330_driver.hpp"

namespace mobile_base_hardware {

class MobileBaseHardwareInterface : public hardware_interface::SystemInterface 
{
public:
    // lifecycle callbacks
    hardware_interface::CallbackReturn 
        on_configure(const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::CallbackReturn 
        on_activate(const rclcpp_lifecycle::State & previous_state) override;
        
    hardware_interface::CallbackReturn 
        on_deactivate(const rclcpp_lifecycle::State & previous_state) override;   
        
    // SystemInterface overrides
    hardware_interface::CallbackReturn 
        on_init(const hardware_interface::HardwareInfo & info) override;
    hardware_interface::return_type
        read(const rclcpp::Time & time, const rclcpp::Duration & period) override;
    hardware_interface::return_type
        write(const rclcpp::Time & time, const rclcpp::Duration & period) override;

    std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
    std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

private:
    std::shared_ptr<XL330Driver> driver_;
    int left_motor_id;
    int right_motor_id;
    std::string port_;

    double left_pos_ = 0.0, left_vel_ = 0.0;
    double right_pos_ = 0.0, right_vel_ = 0.0;
    double left_cmd_vel_ = 0.0, right_cmd_vel_ = 0.0;

}; // class MobileBaseHardwareInterface

} // namespace mobile_base_hardware

#endif