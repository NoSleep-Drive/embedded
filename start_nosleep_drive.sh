#!/bin/bash
set -euo pipefail

# Determine project root as the script's directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

# 1. 프로젝트 디렉터리로 이동
cd "${SCRIPT_DIR}" || exit 1

# 2. build 디렉터리가 없으면 생성
mkdir -p build

# 3. build 디렉터리로 이동
cd build || exit 1

# 4. CMake로 빌드 구성 설정 (CMakeLists.txt 파일 읽음)
cmake ..

# 5. make 명령어로 컴파일
make

# 6. 빌드된 실행 파일 실행
./nosleep_drive