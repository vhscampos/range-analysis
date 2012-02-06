all:
	cd ~/workspace/ra/llvm-3.0/lib/Transforms && rm -rf RangeAnalysis vSSA
	cp -R RangeAnalysis ~/workspace/ra/llvm-3.0/lib/Transforms/RangeAnalysis
	cp -R vSSA ~/workspace/ra/llvm-3.0/lib/Transforms/vSSA
	# Compile vSSA
	cd ~/workspace/ra/llvm-3.0/lib/Transforms/vSSA && make
	# Compile RangeAnalysis
	cd ~/workspace/ra/llvm-3.0/lib/Transforms/RangeAnalysis && make
