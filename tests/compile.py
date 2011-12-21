import sys
import os

passPath = "/home/igor/workspace/llvm-3.0/Debug/lib/"

if len(sys.argv) == 2:
    fileName, fileExtension = os.path.splitext(sys.argv[1]) 
    print("Compiling with Clang")
    os.system("clang -c -emit-llvm "+fileName+fileExtension+" -o "+fileName+".bc")
    print("SSA and Register Allocation")
    os.system("opt -instnamer -mem2reg "+fileName+".bc >"+fileName+"1.bc")
    print("vSSA")
    os.system("opt -load "+passPath+"vSSA.so -vssa "+fileName+"1.bc >"+fileName+"2.bc")
    print("RangeAnalysis Pass")
    os.system("opt -load "+passPath+"RangeAnalysis.so -range-analysis "+fileName+"2.bc")
else:
    print len(sys.argv)
