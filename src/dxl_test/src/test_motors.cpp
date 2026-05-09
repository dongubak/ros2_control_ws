#include "rclcpp/rclcpp.hpp"
#include "dxl_test/xl330_driver.hpp"
#include <thread>

using namespace std::chrono_literals;

#define DXL_ID 10

int main()
{
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Starting motor test node.");

    auto driver = XL330Driver("/dev/ttyACM0");


    if(driver.init() != 0) {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to initialize motor driver.");
        return -1;
    }

    std::this_thread::sleep_for(1s);

    
    // driver test
    // driver.activateWithVelocityMode(DXL_ID);
    // driver.setTargetVelocityRadianPerSec(DXL_ID, 3.14159);

    // std::this_thread::sleep_for(5s);

    // double velocity = driver.getVelocityRadianPerSec(DXL_ID);
    // std::cout << "Current velocity: " << velocity << " rad/s" << std::endl;

    driver.activateWithPositionMode(DXL_ID);
    driver.setTargetPositionRadian(DXL_ID, 1.5708); //

    std::this_thread::sleep_for(5s);

    driver.setTargetPositionRadian(DXL_ID, 0.0); //
    
    std::this_thread::sleep_for(5s);


    double position = driver.getPositionRadian(DXL_ID);
    std::cout << "Current position: " << position << " rad" << std::endl;


    driver.deactivate(DXL_ID);

    return 0;
}