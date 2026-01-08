cmake_minimum_required(VERSION 3.5)
project(hospital_description)

find_package(ament_cmake REQUIRED)

# Install meshes and urdf so that "package://hospital_description/..." works
install(
  DIRECTORY meshes urdf launch config maps rviz worlds 
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
