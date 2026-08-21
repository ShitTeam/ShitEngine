@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d D:\ShitTeam\ShitEngine\ShitEngine
ninja -C "build\Desktop_Qt_6_8_3_MSVC2022_64bit-Release" Editor/CMakeFiles/Editor.dir/spriteeditordialog.cpp.obj Editor/CMakeFiles/Editor.dir/spritesheetdock.cpp.obj 2>&1
echo OBJ_EXIT=%ERRORLEVEL%
