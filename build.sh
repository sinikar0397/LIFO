#!/usr/bin/env bash
# LIFO 빌드 스크립트 (MSYS2 UCRT64 bash 에서 실행)
# 사용법:
#   ./build.sh         -> GUI 버전 빌드 (build/main.exe)
#   ./build.sh console -> 콘솔 로그인 테스트 빌드 (build/console.exe)
set -e

# cc1.exe 의존 DLL 경로를 PATH에 추가 (없으면 gcc가 에러 없이 조용히 실패함)
export PATH="/c/msys64/ucrt64/bin:$PATH"

# 스크립트 위치로 이동 (어디서 호출해도 동작하도록)
cd "$(dirname "$0")"

TARGET="${1:-gui}"

if [ "$TARGET" = "console" ]; then
    echo "[build] 콘솔 버전 빌드 중..."
    gcc main.c src/cJSON.c src/people/login.c src/people/people.c \
        -I src -o build/console.exe
    echo "[build] 완료 -> build/console.exe"
else
    echo "[build] GUI 버전 빌드 중..."
    gcc main_ui.c src/cJSON.c src/people/login.c src/people/people.c \
        src/gui/ui.c src/gui/display.c \
        $(pkg-config --cflags sdl2 SDL2_image SDL2_ttf) \
        $(pkg-config --libs sdl2 SDL2_image SDL2_ttf) \
        -I src -o build/main.exe
    echo "[build] 완료 -> build/main.exe"
fi
