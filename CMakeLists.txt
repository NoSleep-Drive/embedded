# CMake 최소 버전 요구. 3.10 이상이어야 이 설정을 인식할 수 있음.
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름을 설정. 이후 변수 ${PROJECT_NAME}으로도 사용 가능.
project(NoSleepDrive)

# 사용할 C++ 표준 설정 (C++17 사용)
set(CMAKE_CXX_STANDARD 17)

# OpenCV 패키지를 찾음. 시스템에 설치된 OpenCV를 탐색함.
# REQUIRED 옵션은 못 찾으면 오류 발생시킴.
find_package(OpenCV REQUIRED)

# OpenCV 헤더 파일 경로를 포함 경로에 추가
include_directories(${OpenCV_INCLUDE_DIRS})

# dlib 패키지 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(DLIB REQUIRED dlib-1)

# dlib 헤더 파일 경로를 포함 경로에 추가
include_directories(${DLIB_INCLUDE_DIRS})

# Python 라이브러리 찾기 (Python 해더 및 링크 라이브러리) - 가속도 센서용
find_package(PythonLibs REQUIRED)

# Python 헤더 파일 경로를 포함 경로에 추가
include_directories(${PYTHON_INCLUDE_DIRS})

# NumPy 헤더 경로를 Python으로 직접 찾기
execute_process(
    COMMAND python3 -c "import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE NUMPY_INCLUDE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NUMPY_INCLUDE_PATH)
    include_directories(${NUMPY_INCLUDE_PATH})
    message(STATUS "Found NumPy include path: ${NUMPY_INCLUDE_PATH}")
else()
    message(FATAL_ERROR "Could not find NumPy include path")
endif()

# nlohmann 헤더 파일 경로를 포함 경로에 추가
include_directories(${PROJECT_SOURCE_DIR}/include) # include 디렉토리에 json.hpp 필요

# CPR 라이브러리 찾기 (수동 설정)
find_library(CPR_LIBRARY
    NAMES cpr libcpr
    PATHS /usr/local/lib /usr/lib
    REQUIRED
)

find_path(CPR_INCLUDE_DIR
    NAMES cpr/cpr.h
    PATHS /usr/local/include /usr/include
    REQUIRED
)

# OpenSSL 패키지 찾기 (CPR에서 필요)
find_package(OpenSSL REQUIRED)

if(CPR_LIBRARY AND CPR_INCLUDE_DIR)
    message(STATUS "Found CPR library: ${CPR_LIBRARY}")
    message(STATUS "Found CPR include: ${CPR_INCLUDE_DIR}")
    include_directories(${CPR_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Could not find CPR library")
endif()

# src 디렉토리의 모든 .cpp 파일을 자동으로 찾아 SOURCE_FILES 변수에 저장
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

# 실행 파일 생성 시 모든 소스 파일 사용
add_executable(nosleep_drive ${SOURCE_FILES})

# 컴파일러 정의 추가 (NumPy API 관련)
target_compile_definitions(nosleep_drive PRIVATE 
    NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION
)

# 실행 파일과 외부 라이브러리를 연결
# OpenCV, dlib, Python, CPR, OpenSSL 관련 라이브러리를 함께 링크함
target_link_libraries(nosleep_drive 
    ${OpenCV_LIBS} 
    ${DLIB_LIBRARIES}
    ${PYTHON_LIBRARIES} 
    ${CPR_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ssl
    crypto
    curl
    pthread
)

# 추가 컴파일 옵션
target_compile_options(nosleep_drive PRIVATE -Wall -Wextra)

# dlib 컴파일러 플래그 추가
target_compile_options(nosleep_drive PRIVATE ${DLIB_CFLAGS_OTHER})