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

# Python 라이브러리 찾기 (Python 해더 및 링크 라이브러리)
find_package(PythonLibs REQUIRED)

# Python 헤더 파일 경로를 포함 경로에 추가
include_directories(${PYTHON_INCLUDE_DIRS})

# src 디렉토리의 모든 .cpp 파일을 자동으로 찾아 SOURCE_FILES 변수에 저장
file(GLOB_RECURSE SOURCE_FILES src/*.cpp test/*.cpp)

# 실행 파일 생성 시 모든 소스 파일 사용
add_executable(nosleep_drive ${SOURCE_FILES})

# 실행 파일과 외부 라이브러리를 연결
# OpenCV와 Python 관련 라이브러리를 함께 링크함
target_link_libraries(nosleep_drive ${OpenCV_LIBS} ${PYTHON_LIBRARIES})
