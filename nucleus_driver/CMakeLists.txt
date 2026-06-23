cmake_minimum_required(VERSION 3.24)
project(nucleus_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)

find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(marine_acoustic_msgs REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(libnucleus REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(generate_parameter_library REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

generate_parameter_library(nucleus_driver_parameters
    src/nucleus_driver_parameters.yaml
)

add_library(nucleus_driver_library SHARED)
target_sources(
    nucleus_driver_library
    PRIVATE src/nucleus_driver.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/include/nucleus_driver/nucleus_driver.hpp
)
target_compile_features(nucleus_driver_library PUBLIC cxx_std_23)
target_link_libraries(
    nucleus_driver_library
    PRIVATE nucleus_driver_parameters
    PUBLIC
        rclcpp::rclcpp
        rclcpp_lifecycle::rclcpp_lifecycle
        rclcpp_components::component
        rclcpp_components::component_manager
        libnucleus::libnucleus
        tf2_geometry_msgs::tf2_geometry_msgs
        ${geometry_msgs_TARGETS}
        ${marine_acoustic_msgs_TARGETS}
        ${nav_msgs_TARGETS}
)

rclcpp_components_register_node(nucleus_driver_library
    PLUGIN "nucleus::ros::NucleusDriver"
    EXECUTABLE nucleus_driver
)

install(
    TARGETS nucleus_driver_library nucleus_driver_parameters
    EXPORT export_nucleus_driver_library
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)

install(DIRECTORY config launch DESTINATION share/nucleus_driver)

ament_export_targets(export_nucleus_driver_library HAS_LIBRARY_TARGET)
ament_export_dependencies(
    "rclcpp"
    "rclcpp_lifecycle"
    "geometry_msgs"
    "marine_acoustic_msgs"
    "rclcpp_components"
    "libnucleus"
    "nav_msgs"
    "tf2_geometry_msgs"
)

ament_package()
