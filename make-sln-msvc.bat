@echo off
if not exist "%~dp0build" rd /s /q "%~dp0build" 2>nul
cmake -B "%~dp0build" -S "%~dp0Plugin" --preset=build-debug-msvc-msvc
