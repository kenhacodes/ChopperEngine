@echo off
echo =========================================
echo   Cleaning project:  Chopper Engine UwU
echo =========================================

del /s /q *.sln
del /s /q *.vcxproj
del /s /q *.vcxproj.filters
del /s /q *.vcxproj.user
del /s /q *.DotSettings.user

rmdir /s /q ".idea"
rmdir /s /q ".vs"
rmdir /s /q "binaries"

echo.
pause
