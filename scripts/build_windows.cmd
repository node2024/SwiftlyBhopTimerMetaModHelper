@echo off
setlocal
chcp 65001 > nul
set "PYTHONIOENCODING=utf-8"

set "ROOT=%~dp0.."
set "PYTHON=%CODEX_PYTHON%"
if "%PYTHON%"=="" set "PYTHON=C:\Users\node\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
set "AMBUILD=C:\Users\node\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\Scripts\ambuild.exe"
set "AMBUILD_SOURCE=C:\tmp\ambuild"

set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "MMSOURCE=C:\tmp\mmsource-2.0"
set "HL2SDK_ROOT=C:\tmp\hl2sdk-root"
set "HL2SDK_MANIFESTS=C:\tmp\hl2sdk-manifests"

if not exist "%VS_VCVARS%" (
  echo Visual Studio vcvars64.bat not found: %VS_VCVARS%
  exit /b 1
)

call "%VS_VCVARS%"
if errorlevel 1 exit /b 1

set "MSVC_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "WINSDK_BIN=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
set "PATH=%MSVC_BIN%;%WINSDK_BIN%;%PATH%"
set "Path=%PATH%"

if "%~1"=="--probe" (
  where cl
  "%PYTHON%" -c "import os, shutil; print('PATH_KEYS=', [k for k in os.environ if k.lower()=='path']); paths=os.environ.get('PATH', os.environ.get('Path','')).split(';'); print('cl=', shutil.which('cl.exe')); print('link=', shutil.which('link.exe')); print('lib=', shutil.which('lib.exe')); print('rc=', shutil.which('rc.exe')); print('VS_PATHS=', [p for p in paths if 'Visual Studio' in p or 'Windows Kits' in p]); print('INCLUDE_HEAD=', os.environ.get('INCLUDE','')[:500]); print('LIB_HEAD=', os.environ.get('LIB','')[:500])"
  exit /b 0
)

if not exist "%ROOT%\generated" mkdir "%ROOT%\generated"
"%HL2SDK_ROOT%\hl2sdk-cs2\devtools\bin\protoc.exe" --proto_path="%HL2SDK_ROOT%\hl2sdk-cs2\common" -I="%HL2SDK_ROOT%\hl2sdk-cs2\thirdparty\protobuf-3.21.8\src" --cpp_out="%ROOT%\generated" "%HL2SDK_ROOT%\hl2sdk-cs2\common\network_connection.proto"
if errorlevel 1 exit /b 1
"%HL2SDK_ROOT%\hl2sdk-cs2\devtools\bin\protoc.exe" --proto_path="%HL2SDK_ROOT%\hl2sdk-cs2\common" -I="%HL2SDK_ROOT%\hl2sdk-cs2\thirdparty\protobuf-3.21.8\src" --cpp_out="%ROOT%\generated" "%HL2SDK_ROOT%\hl2sdk-cs2\common\networkbasetypes.proto"
if errorlevel 1 exit /b 1

if not exist "%ROOT%\build_windows_headers" mkdir "%ROOT%\build_windows_headers"
pushd "%ROOT%\build_windows_headers"
if not exist "%AMBUILD%" if exist "%AMBUILD_SOURCE%\ambuild2\run.py" set "PYTHONPATH=%AMBUILD_SOURCE%;%PYTHONPATH%"
"%PYTHON%" ..\configure.py --sdks cs2 --targets x86_64 --mms_path "%MMSOURCE%" --hl2sdk-root "%HL2SDK_ROOT%" --hl2sdk-manifests "%HL2SDK_MANIFESTS%"
if errorlevel 1 exit /b 1
if exist "%AMBUILD%" (
  "%AMBUILD%"
) else (
  "%PYTHON%" -c "from ambuild2.run import cli_run; cli_run()"
)
if errorlevel 1 exit /b 1
popd

endlocal
