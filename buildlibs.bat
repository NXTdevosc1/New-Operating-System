if not defined LIB ( 
call vcvars64.bat
)
cd lib/cruntime
call ./compile.bat
cd ../..