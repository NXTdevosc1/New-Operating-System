if not defined LIB ( 
call vcvars64.bat
)
cd lib/cruntime
call ./compile.bat
@REM cd ../eodx
@REM call ./compile.bat
cd ../acpisys
call ./compile.bat
cd ../osusr
call ./compile.bat
cd ../..