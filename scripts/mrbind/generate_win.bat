@echo off

rem This script generates the bindings by running `make -f scripts/mrbind/generate.mk -B` in MSYS2 shell.
rem Must run this inside of the Visual Studio developer command prompt!
rem Any additional arguments are forwarded to that makefile.

set MSYS2_DIR=C:\msys64_meshlib_mrbind

set MRBIND_DIR=%MSYS2_DIR%\home\%USERNAME%\_mrbind

if not exist %MSYS2_DIR% (
    echo MSYS2 was NOT found at `%MSYS2_DIR%`. Run `install_msys2_tools.bat` to build it.
) else (
    echo Fount MSYS2 at `%MSYS2_DIR%`.

    rem Here we save all additional arguments to a variable, and then apply string replacement to it to escape `"` as `""`.
    set args=%*

    call %MSYS2_DIR%\msys2_shell -no-start -defterm -full-path -here -clang64 -c "time make -f '%~dp0generate.mk' -B %args:"=""% "
)