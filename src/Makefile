all:
	cd ~/workspace/ra/llvm-3.0/lib/Transforms && rm -rf RangeAnalysis vSSA Matching
	cp -R RangeAnalysis ~/workspace/ra/llvm-3.0/lib/Transforms/RangeAnalysis
	cp -R vSSA ~/workspace/ra/llvm-3.0/lib/Transforms/vSSA
	cp -R Matching ~/workspace/ra/llvm-3.0/lib/Transforms/Matching
	# Compile vSSA
	cd ~/workspace/ra/llvm-3.0/lib/Transforms/vSSA && make
	# Compile RangeAnalysis
	cd ~/workspace/ra/llvm-3.0/lib/Transforms/RangeAnalysis && make
	# Compile Matching
	cd ~/workspace/ra/llvm-3.0/lib/Transforms/Matching && make