include(ExternalProject)
include(FindPackageHandleStandardArgs)

ExternalProject_Add(yaml-cpp
  GIT_REPOSITORY        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG               yaml-cpp-0.7.0
  CMAKE_ARGS            -DCMAKE_CXX_FLAGS="-w" -DYAML_CPP_BUILD_TESTS=OFF -DYAML_MSVC_SHARED_RT=ON -DYAML_BUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  PREFIX                ${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp
  UPDATE_COMMAND        "" # Skip annoying updated for every build
  LOG_DOWNLOAD          On
)

ExternalProject_Get_Property(yaml-cpp INSTALL_DIR)
set(YAML_CPP_INCLUDE_DIR ${INSTALL_DIR}/include)
set(YAML_CPP_LIB_DIR ${INSTALL_DIR}/lib)
set(YAML_CPP_LIBRARIES "${YAML_CPP_LIB_DIR}/libyaml-cpp.a")

find_package_handle_standard_args(
  yaml-cpp DEFAULT_MSG
  YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARIES)

mark_as_advanced(YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARIES)


