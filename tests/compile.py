#! /usr/bin/env python

import sys
import os

passPath = "/home/igor/workspace/llvm-3.0/Debug/lib/"

def checkArgs(args):
    print ("Lenght=",len(args))
    if len(args) != 3:
        return False
    if args[1] in ["-ra-inter-crop","-ra-inter-cousot","-ra-intra-crop","-ra-intra-cousot"]:
        return True
    else:
        return False

if checkArgs(sys.argv):
    fileName, fileExtension = os.path.splitext(sys.argv[2])
    print("Compiling with Clang")
    print("clang -c -emit-llvm "+fileName+fileExtension+" -o "+fileName+".bc")
    os.system("clang -c -emit-llvm "+fileName+fileExtension+" -o "+fileName+".bc")
    print("Generating eSSA")
    print("opt -instnamer -mem2reg "+fileName+".bc -o "+fileName+".bc")
    os.system("opt -instnamer -mem2reg -inline -internalize "+fileName+".bc -o "+fileName+".bc")
    print("opt -load "+passPath+"vSSA.so -vssa "+fileName+".bc -o "+fileName+".essa.bc")
    os.system("opt -load "+passPath+"vSSA.so -vssa "+fileName+".bc -o "+fileName+".essa.bc")
    print("Running the Range Analysis Pass")
    print("opt -load "+passPath+"RangeAnalysis.so "+sys.argv[1]+" "+fileName+".essa.bc")
    os.system("opt -load "+passPath+"RangeAnalysis.so "+sys.argv[1]+" "+fileName+".essa.bc")

#    os.system("opt -load "+passPath+"RangeAnalysis.so -load "+passPath+"Matching.so -matching "+fileName+".essa.bc")    
else:
    print "./compile.py <OPTION> <FILE.c>"
    print "\tOPTION:"
    print "\t-ra-inter-crop\t\t-\tInter-procedural analysis with CropDFS meet operator"
    print "\t-ra-inter-cousot\t-\tInter-procedural analysis with Cousot meet operator"
    print "\t-ra-intra-crop\t\t-\tIntra-procedural analysis with CropDFS meet operator"
    print "\t-ra-intra-cousot\t-\tIntra-procedural analysis with Cousot meet operator"


