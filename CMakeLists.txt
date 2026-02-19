cmake_minimum_required(VERSION 3.18)

# 컴파일러 설정 (프로젝트 선언 전 필수)
set(CMAKE_C_COMPILER /usr/bin/gcc-12)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)
set(CMAKE_CUDA_HOST_COMPILER /usr/bin/g++-12)

project(physical_ai LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 빌드 타입 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 필수 라이브러리 찾기
set(CMAKE_PREFIX_PATH "$ENV{LIBTORCH_PATH}")
find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)

# ========================================
# 공통 유틸리티 라이브러리 생성
# ========================================
set(UTILS_SOURCES
    src/utils/logger.cpp
)

add_library(utils_lib STATIC ${UTILS_SOURCES})
target_include_directories(utils_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(utils_lib PUBLIC ${TORCH_LIBRARIES} ${OpenCV_LIBS})

# ========================================
# Vision 라이브러리 (웹캠 모듈)
# ========================================
add_library(vision_lib STATIC
    src/vision/webcam.cpp
)
target_include_directories(vision_lib PUBLIC 
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(vision_lib PUBLIC 
    utils_lib
    ${OpenCV_LIBS}
    pthread
)

# ========================================
# 실행 파일 1: 모델 학습
# ========================================
add_executable(main src/main.cpp)

# 헤더 파일 경로 설정
target_include_directories(main PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}            # OpenCV 헤더 경로
)

# 라이브러리 연결
target_link_libraries(main PRIVATE 
    utils_lib
    vision_lib
    ${TORCH_LIBRARIES}
    ${OpenCV_LIBS}
    pthread
)

# 실행 시 라이브러리 경로 설정
set_target_properties(main PROPERTIES
    BUILD_RPATH "${TORCH_INSTALL_PREFIX}/lib"
)

# 빌드 정보 출력 (디버깅용)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "LibTorch version: ${TORCH_VERSION}")

# ${CMAKE_SOURCE_DIR}        # CMakeLists.txt가 있는 최상위 폴더 경로
# ${CMAKE_BINARY_DIR}        # build 폴더 경로
# ${CMAKE_BUILD_TYPE}        # Debug, Release 등 빌드 타입
# ${CMAKE_CXX_COMPILER}      # 사용 중인 C++ 컴파일러 경로
# ${PROJECT_NAME}            # project()에서 지정한 프로젝트 이름
# ${PROJECT_SOURCE_DIR}     # 프로젝트 최상위 폴더 경로
# ${PROJECT_BINARY_DIR}     # 프로젝트 빌드 폴더 경로
# ${CMAKE_CURRENT_SOURCE_DIR} # 현재 CMakeLists.txt가 있는 폴더 경로
# ${CMAKE_CURRENT_BINARY_DIR} # 현재 CMakeLists.txt에 해당하는 빌드 폴더 경로
# ${CMAKE_CXX_STANDARD}     # 설정된 C++ 표준 버전
# ${CMAKE_CUDA_STANDARD}    # 설정된 CUDA 표준 버전
# ${CMAKE_CUDA_COMPILER}   # 사용 중인 CUDA 컴파일러 경로
# ${OpenCV_VERSION}        # OpenCV 버전 정보
# ${OpenCV_LIBS}           # 링크할 OpenCV 라이브러리 목록
# ${OpenCV_INCLUDE_DIRS}   # OpenCV 헤더 파일 경로
# ${TORCH_VERSION}         # LibTorch 버전 정보
# ${TORCH_INSTALL_PREFIX}  # LibTorch 설치 경로
# ${TORCH_LIBRARIES}       # 링크할 LibTorch 라이브러리 목록

