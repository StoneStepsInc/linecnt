@echo off

setlocal

rem change to the parent of test to match paths in the model file
pushd
cd %~p0..

rem generate a random file name for the results
set results=test_%RANDOM%.results

rem save new results in a temporary results file
Release\linecnt.exe -v -d test txt > test\%results%

rem compare the model and new results
fc /A test\test.model test\%results%

rem delete the temporary file
del test\%results%

popd

endlocal
