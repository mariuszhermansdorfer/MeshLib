cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
  set(CMAKE_CXX_STANDARD ${MR_CXX_STANDARD})
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  IF(WIN32)
    # For some reason setting C++20 on Windows appears to be ignored, so that even
    #   the single-argument version of `static_assert` doesn't compile (which is a C++17 feature).
    # Setting C++17 does work though.
    set(CMAKE_CUDA_STANDARD 17)

    # Right now we only seem have the old Cuda 11 in VS 2019 in CI.
    find_package(CUDAToolkit 11 REQUIRED)

    # For our VS2022 CI:
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH")
  ELSE()
    set(CMAKE_CUDA_STANDARD 20)
    find_package(CUDAToolkit 12 REQUIRED)
  ENDIF()
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)

  IF(NOT DEFINED CMAKE_CUDA_COMPILER)
    set(CMAKE_CUDA_COMPILER ${CUDAToolkit_NVCC_EXECUTABLE})
  ENDIF()

  IF(NOT DEFINED CMAKE_CUDA_HOST_COMPILER)
    option(MRCUDA_OVERRIDE_HOST_COMPILER "Override the default nvcc host compiler with the one used for C++ compilation" OFF)
    IF(MRCUDA_OVERRIDE_HOST_COMPILER)
      set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
    ENDIF()
  ENDIF()

  IF(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 52-real 60-real 61-real 70-real 75)
  ENDIF()

  set(CUDART_LIBRARY CUDA::cudart_static)
ELSE()
  # nvcc supports only c++20 and Cmake 3.16 from Ubuntu 20 does not support set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # more info: https://gitlab.kitware.com/cmake/cmake/-/issues/23079
  IF(NOT DEFINED CMAKE_CUDA20_STANDARD_COMPILE_OPTION)
    set(CMAKE_CUDA20_STANDARD_COMPILE_OPTION "-std=c++20")
    set(CMAKE_CUDA20_EXTENSION_COMPILE_OPTION "-std=c++20")
  ENDIF()

  set(CUDA_TOOLKIT_ROOT_DIR "/usr/local/cuda/")
  set(CMAKE_CUDA_PATH /usr/local/cuda/)
  set(CUDA_NVCC_EXECUTABLE ${CUDA_TOOLKIT_ROOT_DIR}/bin/nvcc)
  set(CMAKE_CUDA_COMPILER ${CUDA_TOOLKIT_ROOT_DIR}/bin/nvcc)

  # without it we get "nvcc warning : The -std=c++20 flag is not supported with the configured host compiler. Flag will be ignored."
  # https://stackoverflow.com/q/77170793/7325599
  set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
  message("CMAKE_CXX_COMPILER=CMAKE_CUDA_HOST_COMPILER=${CMAKE_CUDA_HOST_COMPILER}")

  find_package(CUDA 12 REQUIRED)

  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} \
    --std c++20 \
    -use_fast_math \
    -arch=sm_52 \
    -gencode=arch=compute_52,code=sm_52 \
    -gencode=arch=compute_60,code=sm_60 \
    -gencode=arch=compute_61,code=sm_61 \
    -gencode=arch=compute_70,code=sm_70 \
    -gencode=arch=compute_75,code=sm_75 \
    -gencode=arch=compute_75,code=compute_75"
  )
  set(CUDA_VERBOSE_BUILD ON)

  set(CUDART_LIBRARY ${CUDA_LIBRARIES})
ENDIF()

project(MRCuda CXX CUDA)

IF(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -use_fast_math")
ELSEIF(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -ffast-math")
  # see: https://github.com/llvm/llvm-project/issues/58491
  set(CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG} -Xarch_device -g0")
ENDIF()

option(MRCUDA_NO_VOXELS "Disable voxels support" OFF)

IF(NOT MESHLIB_BUILD_VOXELS)
  set(MRCUDA_NO_VOXELS ON)
ENDIF()

file(GLOB SOURCES "*.cpp" "*.cu")
file(GLOB HEADERS "*.h" "*.hpp" "*.cuh")

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_cmake.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config_cmake.h)

set(MRCUDA_OPTIONAL_DEPENDENCIES "")

IF(NOT MRCUDA_NO_VOXELS)
  list(APPEND MRCUDA_OPTIONAL_DEPENDENCIES MRVoxels)
ENDIF()

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CUDA_INCLUDE_DIRS}
)


IF(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    MRMesh
    spdlog::spdlog
    fmt::fmt
    TBB::tbb
    ${CUDART_LIBRARY}
    ${MRCUDA_OPTIONAL_DEPENDENCIES}
  )
ELSE()
  target_link_libraries(${PROJECT_NAME} PRIVATE
    MRMesh
    spdlog
    fmt
    tbb
    ${CUDART_LIBRARY}
    ${MRCUDA_OPTIONAL_DEPENDENCIES}
  )
ENDIF()

set(CMAKE_INSTALL_PREFIX "/Library/Frameworks")
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION "${MR_MAIN_LIB_DIR}"
  ARCHIVE DESTINATION "${MR_MAIN_LIB_DIR}"
  RUNTIME DESTINATION "${MR_BIN_DIR}"
)

install(
  FILES ${HEADERS}
  DESTINATION "${MR_INCLUDE_DIR}/${PROJECT_NAME}"
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake
  DESTINATION ${MR_CONFIG_DIR}
)

install(
  EXPORT ${PROJECT_NAME}
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE MeshLib::
  DESTINATION ${MR_CONFIG_DIR}
)

IF(MR_PCH)
  IF(CMAKE_CXX_STANDARD EQUAL MR_CXX_STANDARD)
    TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} REUSE_FROM MRPch)
    message( "MRCuda: using PCH" )
  ELSE()
    message( "MRCuda: not using PCH" )
  ENDIF()
ENDIF()
