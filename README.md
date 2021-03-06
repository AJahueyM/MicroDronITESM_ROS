# MicroDronITESM_ROS
Clone with:
```
git clone --recurse-submodules https://github.com/microdron-itesm/MicroDronITESM_ROS.git
```

Follow the instructions as specified in the ROS Wiki http://wiki.ros.org/noetic/Installation/Ubuntu

Make sure to install the needed dependencies
On Ubuntu: 
```
sudo apt-get install ros-noetic-octomap libgoogle-glog-dev libsfml-dev 
```


This is used to test the drone control code used in the MicroDron.

To compile the project use:
```
cd MicroDronITESM_ROS
```
```
catkin_make
```
```
source devel/setup.bash
```
To run the simulation use:
```
roslaunch rotors_gazebo mav.launch mav_name:=ardrone world_name:=basic paused:=false
```

To run the control node open another terminal, source devel/setup.bash and use:
```
rosrun drone_control drone_control_node
```
