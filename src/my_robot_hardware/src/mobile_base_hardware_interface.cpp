#include "my_robot_hardware/mobile_base_hardware_interface.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"

namespace HW_IF = hardware_interface;

namespace mobile_base_hardware 
{
HW_IF::CallbackReturn 
    MobileBaseHardwareInterface::on_init(const HW_IF::HardwareInfo & info) {
        if(
            HW_IF::SystemInterface::on_init(info) != HW_IF::CallbackReturn::SUCCESS
        ) {
            return HW_IF::CallbackReturn::ERROR;
        }

        info_ = info;

        left_motor_id = 10;
        right_motor_id = 20;
        port_ = "/dev/ttyACM0";

        driver_ = std::make_shared<XL330Driver>(port_);
        
        return HW_IF::CallbackReturn::SUCCESS;
}

HW_IF::CallbackReturn 
    MobileBaseHardwareInterface::on_configure(const rclcpp_lifecycle::State & previous_state) {
        (void)previous_state;

        if(driver_->init() != 0) {
            return HW_IF::CallbackReturn::ERROR;
        }

        return HW_IF::CallbackReturn::SUCCESS;
}

HW_IF::CallbackReturn 
    MobileBaseHardwareInterface::on_activate(const rclcpp_lifecycle::State & previous_state) {
        (void)previous_state;
        
        left_cmd_vel_ = 0.0;
        right_cmd_vel_ = 0.0;
        left_vel_ = 0.0;
        right_vel_ = 0.0;
        left_pos_ = 0.0;
        right_pos_ = 0.0;

        driver_ -> activateWithVelocityMode(left_motor_id);
        driver_ -> activateWithVelocityMode(right_motor_id);
        return HW_IF::CallbackReturn::SUCCESS;
}


HW_IF::CallbackReturn 
    MobileBaseHardwareInterface::on_deactivate(const rclcpp_lifecycle::State & previous_state) {
        (void)previous_state;
        driver_ -> deactivate(left_motor_id);
        driver_ -> deactivate(right_motor_id);
        return HW_IF::CallbackReturn::SUCCESS;
}   
        

std::vector<HW_IF::StateInterface>
    MobileBaseHardwareInterface::export_state_interfaces() {
        std::vector<HW_IF::StateInterface> interfaces;
        interfaces.emplace_back("base_left_wheel_joint",  HW_IF::HW_IF_POSITION, &left_pos_);
        interfaces.emplace_back("base_left_wheel_joint",  HW_IF::HW_IF_VELOCITY, &left_vel_);
        interfaces.emplace_back("base_right_wheel_joint", HW_IF::HW_IF_POSITION, &right_pos_);
        interfaces.emplace_back("base_right_wheel_joint", HW_IF::HW_IF_VELOCITY, &right_vel_);
        return interfaces;
}

std::vector<HW_IF::CommandInterface>
    MobileBaseHardwareInterface::export_command_interfaces() {
        std::vector<HW_IF::CommandInterface> interfaces;
        interfaces.emplace_back("base_left_wheel_joint",  HW_IF::HW_IF_VELOCITY, &left_cmd_vel_);
        interfaces.emplace_back("base_right_wheel_joint", HW_IF::HW_IF_VELOCITY, &right_cmd_vel_);
        return interfaces;
}

HW_IF::return_type
    MobileBaseHardwareInterface::read(const rclcpp::Time & time, const rclcpp::Duration & period) {
        (void) time;
        left_vel_  = driver_->getVelocityRadianPerSec(left_motor_id);
        right_vel_ = driver_->getVelocityRadianPerSec(right_motor_id);

        left_pos_  += left_vel_  * period.seconds();
        right_pos_ += right_vel_ * period.seconds();

        return HW_IF::return_type::OK;
}
HW_IF::return_type
    MobileBaseHardwareInterface::write(const rclcpp::Time & time, const rclcpp::Duration & period) {
        (void) time;
        (void) period;

        driver_->setTargetVelocityRadianPerSec(left_motor_id, left_cmd_vel_);
        driver_->setTargetVelocityRadianPerSec(right_motor_id, right_cmd_vel_);

        return HW_IF::return_type::OK;
}
    
} // namespace mobile_base_hardware