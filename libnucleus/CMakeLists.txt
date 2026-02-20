cmake_minimum_required(VERSION 3.24)
project(
    libnucleus
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION "C++ library used to interact with the Nortek Nucleus"
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)

find_package(ament_cmake REQUIRED)
find_package(Eigen3 REQUIRED)

add_library(libnucleus SHARED)

target_sources(
    libnucleus
    PRIVATE
        src/checksum.cpp
        src/client.cpp
        src/login.cpp
        src/packet.cpp
        src/ascii.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/client.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/family_id.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/mode.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/packet.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/report.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/series_id.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/version.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/libnucleus/response.hpp
)
target_compile_features(libnucleus PUBLIC cxx_std_23)
target_link_libraries(libnucleus PUBLIC Eigen3::Eigen)
set_target_properties(libnucleus PROPERTIES PREFIX "")

set(EXAMPLES subscribe_to_reports)
foreach(example IN ITEMS ${EXAMPLES})
    add_executable(${example} examples/${example}.cpp)
    add_dependencies(${example} libnucleus)
    target_link_libraries(${example} PUBLIC libnucleus PRIVATE Eigen3::Eigen)
    set_target_properties(
        ${example}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/examples
    )
endforeach()

install(
    TARGETS libnucleus
    EXPORT libnucleusTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)

ament_export_targets(libnucleusTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(Eigen3)

ament_package()
