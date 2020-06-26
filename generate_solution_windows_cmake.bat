@ECHO OFF

where java > NUL
if %ERRORLEVEL% neq 0 (
	echo.
	echo.
	echo ERROR: java.exe that is required to generate solutions files was not found
	echo.
	echo.Please download it from:
	echo   https://jdk.java.net/13/ 
	echo or here:
	echo   https://openjdk.java.net/install/index.html
	echo.
	echo.
	pause
	exit /b 0
)

SET DIR=%cd%

java -jar %DIR%\build.jar -generator=cmake

PAUSE