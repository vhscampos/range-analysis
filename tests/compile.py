#! /usr/bin/env python

import sys
import os

passPath = "~/workspace/ra/llvm-3.0/Debug/lib/"
analysisTypes = ["-ra-inter-crop","-ra-inter-cousot","-ra-intra-crop","-ra-intra-cousot"]
testTypes = ["-ra-test-range"]

def checkArgs(args):
    if len(args) != 3: 
        return False
    elif checkIfIsAnalysis(args) or checkIfIsTest(args):
        return True
    return False

def checkIfIsAnalysis(args):
    if args[1] in analysisTypes:
        return True
    return False

def checkIfIsTest(args):
    if args[1] in testTypes:
        return True
    return False

def printHelp():
    print "./compile.py <OPTION> <FILE.c>"
    print "\tOPTION:"
    print "\t-ra-inter-crop\t\t-\tInter-procedural analysis with CropDFS meet operator"
    print "\t-ra-inter-cousot\t-\tInter-procedural analysis with Cousot meet operator"
    print "\t-ra-intra-crop\t\t-\tIntra-procedural analysis with CropDFS meet operator"
    print "\t-ra-intra-cousot\t-\tIntra-procedural analysis with Cousot meet operator"

if checkArgs(sys.argv):
    fileName, fileExtension = os.path.splitext(sys.argv[2])
    if checkIfIsAnalysis(sys.argv):
        print("Compiling with Clang")
        clangCmd = "clang -c -emit-llvm "+fileName+fileExtension+" -o "+fileName+".bc" 
        print(clangCmd)
        os.system(clangCmd)
        print("Generating eSSA")
        eSSACmd = "opt -instnamer -mem2reg -break-crit-edges -load "+passPath+"vSSA.so -vssa "+fileName+".bc -o "+fileName+".essa.bc"
        print(eSSACmd)
        os.system(eSSACmd)
        print("Running the Range Analysis Pass")
        raCmd = "opt -load "+passPath+"RangeAnalysis.so "+sys.argv[1]+" "+fileName+".essa.bc -stats"
        print(raCmd)
        os.system(raCmd)
    elif checkIfIsTest(sys.argv):
        print("Running the Range Analysis Test Pass")
        raCmd = "opt -load "+passPath+"RangeAnalysis.so -load "+passPath+"RangeAnalysisTest.so "+sys.argv[1]+" "+fileName+".essa.bc"
        print(raCmd)
        os.system(raCmd)
    else:
        printHelp()
else:
    printHelp()



