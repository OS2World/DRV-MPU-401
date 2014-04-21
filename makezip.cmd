@echo off
setlocal
wmake clean
if exist \mpu401.zip del \mpu401.zip
cd ..\..\..\vdev\mme\vmpu401
make clean
cd \
zip -9oX mpu401b base\src\dev\mme\mpu401b\* base\src\vdev\mme\vmpu401\*
endlocal
