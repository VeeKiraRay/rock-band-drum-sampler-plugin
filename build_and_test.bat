@echo off
powershell -ExecutionPolicy Bypass -File "%~dp0scripts\build_and_test.ps1" %*
exit /b %ERRORLEVEL%
