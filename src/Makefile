llvmpath=~/workspace/ra/llvm-3.0

all:
	cd ${llvmpath}/lib/Transforms && rm -rf RangeAnalysis vSSA RAInstrumentation  RAPrinter OverflowDetect #RangeAnalysisTest
	cp -R RangeAnalysis ${llvmpath}/lib/Transforms/RangeAnalysis
	cp -R vSSA ${llvmpath}/lib/Transforms/vSSA
	cp -R RAInstrumentation ${llvmpath}/lib/Transforms/RAInstrumentation
	cp -R RAPrinter ${llvmpath}/lib/Transforms/RAPrinter
	cp -R OverflowDetect ${llvmpath}/lib/Transforms/OverflowDetect	
#	cp -R RangeAnalysisTest ${llvmpath}/lib/Transforms/RangeAnalysisTest
	# Compile vSSA
	cd ${llvmpath}/lib/Transforms/vSSA && make # && make -j2 ENABLE_PROFILING=1
	# Compile RangeAnalysis
	cd ${llvmpath}/lib/Transforms/RangeAnalysis && make # && make -j2 ENABLE_PROFILING=1
	# Compile RangeAnalysis Instrumentation
	cd ${llvmpath}/lib/Transforms/RAInstrumentation && make # && make -j2 ENABLE_PROFILING=1
	# Compile RangeAnalysis Printer
	cd ${llvmpath}/lib/Transforms/RAPrinter && make # && make -j2
	# Compile OverflowDetect
	cd ${llvmpath}/lib/Transforms/OverflowDetect && make # && make -j2	
ENABLE_PROFILING=1	
	# Compile RangeAnalysisTests
#cd ${llvmpath}/lib/Transforms/RangeAnalysisTest && make
