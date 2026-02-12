cmake_minimum_required(VERSION 3.18)
project(physical_ai_test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "$ENV{LIBTORCH_PATH}")

find_package(Torch REQUIRED)

add_executable(test_app src/main.cpp)
target_link_libraries(test_app "${TORCH_LIBRARIES}")

# 실행 시 라이브러리 찾기 위한 RPATH 설정
set_target_properties(test_app PROPERTIES
    BUILD_RPATH "${TORCH_INSTALL_PREFIX}/lib"
)
