@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or
@REM vcvarsall.bat to setup command-line compiler.

@echo OFF
set ProjectName=dchip-8

ctime -begin %ProjectName%.ctm

REM Build tags file
ctags -R

REM Check if build tool is on path
REM >nul, 2>nul will remove the output text from the where command
where cl.exe >nul 2>nul
if %errorlevel%==1 call msvc86.bat

REM Drop compilation files into build folder
IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin

REM EHa- disable exception handling (we don't use)
REM GR- disable c runtime type information (we don't use)

REM MD use dynamic runtime library
REM MT use static runtime library, so build and link it into exe

REM Oi enable intrinsics optimisation, let us use CPU intrinsics if there is one
REM instead of generating a call to external library (i.e. CRT).

REM Zi enables debug data, Z7 combines the debug files into one.

REM W4 warning level 4
REM WX treat warnings as errors
REM wd4100 ignore: unused argument parameters
REM wd4201 ignore: nonstandard extension used: nameless struct/union
REM wd4189 ignore: local variable is initialised but not referenced

set CompileFlags=-EHa- -GR- -Oi -MT -Z7 -W4 -WX -wd4100 -wd4201 -wd4189

REM Include directories
set IncludeFlags=

REM Link libraries
set LinkLibraries=user32.lib gdi32.lib msimg32.lib

REM incrmenetal:no, turn incremental builds off
REM opt:ref, try to remove functions from libs that are referenced at all
set LinkFlags=-incremental:no -opt:ref

cl %CompileFlags% ..\src\%ProjectName%.cpp %IncludeFlags% /link -subsystem:WINDOWS,5.1 %LinkLibraries% %LinkFlags% /nologo /OUT:"%ProjectName%.exe"

popd
ctime -end %ProjectName%.ctm

