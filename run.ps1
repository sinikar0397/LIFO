# LIFO 빌드 + 실행 스크립트 (PowerShell)
# 사용법:
#   .\run.ps1            -> GUI 버전 빌드 후 실행
#   .\run.ps1 console    -> 콘솔 버전 빌드 후 실행
#   .\run.ps1 -NoRun     -> 빌드만 하고 실행은 안 함
param(
    [string]$Target = "gui",
    [switch]$NoRun
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$bash = "C:\msys64\usr\bin\bash.exe"

if (-not (Test-Path $bash)) {
    Write-Error "MSYS2 bash를 찾을 수 없습니다: $bash"
    exit 1
}

# MSYS2 bash로 build.sh 실행
Write-Host "[run] 빌드 시작 ($Target)..." -ForegroundColor Cyan
& $bash -lc "cd '$($root -replace '\\','/')' && ./build.sh $Target"
if ($LASTEXITCODE -ne 0) {
    Write-Error "빌드 실패 (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

if ($NoRun) {
    Write-Host "[run] 빌드만 완료 (실행 생략)" -ForegroundColor Green
    exit 0
}

# 실행할 exe 결정
if ($Target -eq "console") {
    $exe = Join-Path $root "build\console.exe"
} else {
    $exe = Join-Path $root "build\main.exe"
}

Write-Host "[run] 실행: $exe" -ForegroundColor Green
& $exe
