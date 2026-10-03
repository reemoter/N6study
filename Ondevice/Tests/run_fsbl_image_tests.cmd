@echo off
setlocal
cd /d "%~dp0.."
if not exist Build\tests\native mkdir Build\tests\native
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /W4 /I FSBL\Core\Inc Tests\fsbl_image_test.c /FoBuild\tests\native\fsbl_image_test.obj /FeBuild\tests\native\fsbl_image_test.exe /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
Build\tests\native\fsbl_image_test.exe Firmware\baseline\Secure-trusted.bin Firmware\baseline\NonSecure-trusted.bin
if errorlevel 1 exit /b 1
cl /nologo /W4 /wd4312 /wd4302 /wd4311 /I Tests\stubs /I FSBL\Core\Inc Tests\fsbl_loader_test.c FSBL\Core\Src\fsbl_app.c /FoBuild\tests\native\ /FeBuild\tests\native\fsbl_loader_test.exe /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
Build\tests\native\fsbl_loader_test.exe Firmware\baseline\Secure-trusted.bin Firmware\baseline\NonSecure-trusted.bin
exit /b %errorlevel%
