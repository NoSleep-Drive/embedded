#!/bin/bash
# 이 파일이 Bash 셸 스크립트임을 나타냄 (리눅스에서 실행 가능하게 함)

# 1. 프로젝트 디렉터리로 이동
cd /home/pi/nosleep_drive
# 사용자의 CMake 프로젝트 루트 디렉터리로 이동함

# 2. build 디렉터리가 없으면 생성
mkdir -p build

# 3. build 디렉터리로 이동
cd build

# 4. CMake로 빌드 구성 설정 (CMakeLists.txt 파일 읽음)
cmake ..
# .. 은 상위 디렉터리 = 소스가 있는 루트 디렉터리를 의미

# 5. make 명령어로 컴파일
make
# cmake가 생성한 Makefile을 기반으로 실제 빌드 수행

# 6. 빌드된 실행 파일 실행
./nosleep_drive
# 실행 파일 이름은 CMakeLists.txt의 add_executable에서 정의한 이름
