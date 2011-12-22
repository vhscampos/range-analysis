#! /usr/bin/env python

import sys
import os

passPath = "/home/igor/workspace/llvm-3.0/Debug/lib/"

if len(sys.argv) == 2:
    fileName, fileExtension = os.path.splitext(sys.argv[1]) 
    print("Compiling with Clang")
    os.system("clang -c -emit-llvm "+fileName+fileExtension+" -o "+fileName+".bc")
    print("eSSA")
    os.system("opt -instnamer -mem2reg -load "+passPath+"vSSA.so -vssa "+fileName+".bc -o "+fileName+".essa.bc")
    print("RangeAnalysis Pass")
    os.system("opt -load "+passPath+"RangeAnalysis.so -range-analysis "+fileName+".essa.bc")
else:
    print len(sys.argv)
