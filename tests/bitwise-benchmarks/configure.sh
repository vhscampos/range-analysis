llvmpath=~/workspace/ra/llvm-3.0
testsuite=${llvmpath}/projects/test-suite/SingleSource/Benchmarks
bitwise=$testsuite/Bitwise
rm -rf $bitwise
cp -R Bitwise $bitwise
mkdir $bitwise/Output
