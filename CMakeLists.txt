cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0148 NEW)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/twin4/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(CMAKE_PREFIX_PATH "C:/Users/twin4/vcpkg/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})


project(NoSleepDrive)

set(CMAKE_CXX_STANDARD 17)

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Python3 + NumPy (CMake 3.12 이상부터 지원, CMake 3.28 호환됨)
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
include_directories(${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS})

# NumPy 경로 메시지 출력 (선택 사항)
message(STATUS "Found Python3 include path: ${Python3_INCLUDE_DIRS}")
message(STATUS "Found NumPy include path: ${Python3_NumPy_INCLUDE_DIRS}")

# nlohmann json header
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(cpr CONFIG REQUIRED)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})


# OpenSSL
find_package(OpenSSL REQUIRED)

# 소스 파일 수집
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

# 실행 파일 생성
add_executable(nosleep_drive ${SOURCE_FILES})

# NumPy 매크로 정의
target_compile_definitions(nosleep_drive PRIVATE 
    NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION
)

# 외부 라이브러리 링크
target_link_libraries(nosleep_drive 
    ${OpenCV_LIBS}
    ${Python3_LIBRARIES}
    cpr::cpr
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

# 컴파일 옵션
if (MSVC)
    target_compile_options(nosleep_drive PRIVATE /W4)
else()
    target_compile_options(nosleep_drive PRIVATE -Wall -Wextra -Werror)
endif() 