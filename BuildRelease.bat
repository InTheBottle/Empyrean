@echo off
setlocal
cd /d "%~dp0"

set NAME=PathsOfEmpyrean
set OUT=dist\%NAME%

cmake --preset ALL || (pause & exit /b 1)
cmake --build build --config Release || (pause & exit /b 1)

if exist dist rmdir /s /q dist
xcopy "Data" "%OUT%\" /e /i /y
xcopy "build\Release\%NAME%.dll" "%OUT%\SKSE\Plugins\" /i /y
xcopy "build\Release\%NAME%.pdb" "%OUT%\SKSE\Plugins\" /i /y
xcopy "build\Scripts\*.pex" "%OUT%\Scripts\" /i /y

cmake -E chdir "%OUT%" cmake -E tar cf "..\%NAME%.zip" --format=zip . || (pause & exit /b 1)

if defined SKYRIM_MODS_FOLDER xcopy "%OUT%" "%SKYRIM_MODS_FOLDER%\%NAME%\" /e /i /y

echo.
echo Built dist\%NAME%.zip
pause
