cmake_minimum_required(VERSION 3.13)
project(MqttEcalBridge)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(yaml-cpp REQUIRED)
find_package(Mosquitto REQUIRED)

find_package(eCAL REQUIRED)

add_executable(${PROJECT_NAME}
  src/Bridge.cpp
  src/MqttEcalBridge.cpp
  src/Bridge.h
  src/stringutils.h
  src/Broker.h
  src/Broker.cpp
  src/MqttTopic.h
  src/MqttTopic.cpp
  src/EcalTopic.h
  src/EcalTopic.cpp
  src/utils.h
)

add_dependencies(${PROJECT_NAME} yaml-cpp)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  src
  SYSTEM
  PRIVATE
  ${MOSQUITTO_INCLUDE_DIR}
  ${YAML_CPP_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  eCAL::core
  eCAL::pb
  ${MOSQUITTO_LIBRARIES}
  ${YAML_CPP_LIBRARIES}
)