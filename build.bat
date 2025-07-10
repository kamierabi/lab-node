@echo off
setlocal

:: Настройка переменных
set CXX=cl
set CXXFLAGS=/std:c++20 /EHsc /Iinclude
set SOURCES=Logger\Logger.cpp Server\Server.cpp ThreadPool\ThreadPool.cpp Loader\Loader.cpp main.cpp
set TARGET=lab-iserver.exe

:: Очистка предыдущих сборок
if exist %TARGET% del %TARGET%
del *.obj 2>nul

:: Компиляция
%CXX% %CXXFLAGS% %SOURCES% /Fe:%TARGET%

:: Завершение
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
) else (
    echo Build succeeded.
    del *.obj 2>nul

)