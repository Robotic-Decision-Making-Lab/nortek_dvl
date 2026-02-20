# Nortek Nucleus API

nortek_dvl is a C++ library and ROS 2 driver designed to interface with the
[Nortek Nucleus](https://www.nortekgroup.com/products/nucleus1000). Get started
with nortek_dvl by installing the project, exploring the implemented library
[examples](https://github.com/Robotic-Decision-Making-Lab/nortek_dvl/tree/main/libnucleus/examples),
or by launching the ROS 2 driver.

> :warning: This project is not affiliated with or maintained by Nortek.
> Please refer to the Nortek [GitHub Account](https://github.com/NortekSupport/)
> for all official software.

## Installation

To install nortek_dvl, first clone the repository to the `src/` directory
of your ROS 2 workspace

```bash
git clone git@github.com:Robotic-Decision-Making-Lab/nortek_dvl.git
```

Then install the project dependencies using vcstool and rosdep

```bash
vcs import src < src/nortek_dvl/ros2.repos && \
rosdep install --from paths src -y --ignore-src --skip-keys nlohmann_json
```

Finally, build the workspace using colcon

```bash
colcon build && source install/setup.bash
```

## Usage

Prior to using nortek_dvl, first ensure that you can successfully connect
to your respective device. For additional information, please refer to the
Nortek [operation documentation](https://github.com/Robotic-Decision-Making-Lab/nortek_dvl/tree/main/docs).
After verifying the network connection, the DVL ROS 2 driver can be launched
with the following command:

```bash
ros2 launch nucleus_driver dvl.launch.py
```

A sample configuration file can be found in `nucleus_driver/config/dvl.yaml`,
and the full list of parameters can be found in `nucleus_driver/src/nucleus_driver_parameters.yaml`.

## Getting help

If you have questions regarding usage of nortek_dvl or regarding contributing
to this project, please ask a question on our [Discussions](https://github.com/Robotic-Decision-Making-Lab/nortek_dvl/discussions)
board.

## License

nortek_dvl is released under the MIT license.
