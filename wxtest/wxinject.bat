@echo off

set THIS_DIR=%~dp0

set CFG=Debug

if not "%1" == "" set CFG=%1

set INJECT="%THIS_DIR%\injectdll.exe"

cd /d %THIS_DIR%
cd ..
set DLLFILE="%CD%\vs2015\%CFG%\HKRPC.dll"

%INJECT% "WeChat.exe" %DLLFILE%

cd %THIS_DIR%

