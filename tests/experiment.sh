#!/bin/bash

#
# Experiments:
# 1: simple SSA, Cousot, Intra, no inline
# 2: e-SSA, Cousot, Intra, no inline
# 3: simple SSA, Cousot, Intra, inline
# 4: e-SSA, Cousot, Intra, inline
# 5: simple SSA, Cousot, Inter, no inline
# 6: e-SSA, Cousot, Inter, no inline
# 7: simple SSA, Cousot, Inter, inline
# 8: e-SSA, Cousot, Inter, inline
#

if [ $# -eq 2 ]; then
	# Do not forget to put the ending / in every path below
	llvmpath=/home/igor/workspace/ra/llvm-3.0/
	testpath=${llvmpath}projects/test-suite/$1 #External/SPEC/CINT2006/
	outputpath=$2 #/home/igor/workspace/ra/experiments/
	build=Debug/
	mkdir -p $outputpath

	# Experiment 1
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ANALYSIS=-ra-intra-cousot report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp1.csv

	# Experiment 2
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ESSA=1 ANALYSIS=-ra-intra-cousot report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp2.csv

	# Experiment 3
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ANALYSIS=-ra-intra-cousot INLINE=1 report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp3.csv

	# Experiment 4
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ESSA=1 ANALYSIS=-ra-intra-cousot INLINE=1 report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp4.csv

	# Experiment 5
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ANALYSIS=-ra-inter-cousot report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp5.csv

	# Experiment 6
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ESSA=1 ANALYSIS=-ra-inter-cousot report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp6.csv

	# Experiment 7
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ANALYSIS=-ra-inter-cousot INLINE=1 report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp7.csv

	# Experiment 8
	cd $llvmpath
	cd projects/test-suite
	touch TEST.ra.Makefile TEST.ra.report

	cd $testpath
	make TEST=ra ESSA=1 ANALYSIS=-ra-inter-cousot INLINE=1 report report.csv

	mv ${testpath}report.ra.csv ${outputpath}exp8.csv
else
	echo "How to use this script:"
	echo "./experiment.sh <SUITE_PATH> <OUTPUT_PATH>"
	echo "    <SUITE_PATH>: path where the test suite is located"
	echo "    <OUTPUT_PATH>: path where the output results of the tests will be saved"
fi
