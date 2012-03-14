llvmpath=~/workspace/ra/llvm-3.0

all:
	cd ${llvmpath}/lib/Transforms && rm -rf RangeAnalysis vSSA #RangeAnalysisTest
	cp -R RangeAnalysis ${llvmpath}/lib/Transforms/RangeAnalysis
	cp -R vSSA ${llvmpath}/lib/Transforms/vSSA
#	cp -R RangeAnalysisTest ${llvmpath}/lib/Transforms/RangeAnalysisTest
	# Compile vSSA
	cd ${llvmpath}/lib/Transforms/vSSA && make # && make -j2 ENABLE_PROFILING=1
	# Compile RangeAnalysis
	cd ${llvmpath}/lib/Transforms/RangeAnalysis && make # && make -j2 ENABLE_PROFILING=1
	# Compile RangeAnalysisTests
#cd ${llvmpath}/lib/Transforms/RangeAnalysisTest && make
