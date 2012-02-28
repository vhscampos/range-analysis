#!/bin/bash

#
# Experiments:
# 1: Cousot, Intra, no inline
# 2: Cousot, Intra, inline
# 3: Cousot, Inter, no inline
# 4: Cousot, Inter, inline
#

# Do not forget to put the ending / in every path below
llvmpath=/work/victorsc/llvm-3.0/
testpath=${llvmpath}projects/test-suite/External/SPEC/CINT2006/
outputpath=/work/victorsc/output/
build=Debug+Asserts/

# Experiment 1
cd $llvmpath
cd projects/test-suite
touch TEST.ra.Makefile TEST.ra.report

cd $testpath
make TEST=ra ANALYSIS=-ra-intra-cousot report report.csv

mv report.ra.csv ${outputpath}exp1.csv

# Experiment 2
cd $llvmpath
cd projects/test-suite
touch TEST.ra.Makefile TEST.ra.report

cd $testpath
make TEST=ra ANALYSIS=-ra-intra-cousot INLINE=1 report report.csv

mv report.ra.csv ${outputpath}exp2.csv

# Experiment 3
cd $llvmpath
cd projects/test-suite
touch TEST.ra.Makefile TEST.ra.report

cd $testpath
make TEST=ra ANALYSIS=-ra-inter-cousot report report.csv

mv report.ra.csv ${outputpath}exp3.csv

# Experiment 4
cd $llvmpath
cd projects/test-suite
touch TEST.ra.Makefile TEST.ra.report

cd $testpath
make TEST=ra ANALYSIS=-ra-inter-cousot INLINE=1 report report.csv

mv report.ra.csv ${outputpath}exp4.csv
