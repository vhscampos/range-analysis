# Introduction #

LLVM provides a mechanism called Test-Suite that allows testing the compiler and compiler passes. The LLVM Test-Suite is a very useful tool based in makefiles that is capable of doing everything a makefile can do (that is almost everything, once the Makefile language is Turing complete). The Test-Suite infrastructure is already prepared to execute tests using the SPEC benchmarks.
However, the Test-Suite can be very complicated when we need to write tests that gather runtime information of the benchmark programs. To get access to runtime information, we need to compile the benchmark, run the compiled program and get the results that the compiled program will produce in a text file or somewhere else. It is absolutely possible to do that only using the Test-Suite infrastructure, but it is hard.
We have made a set of scripts that have the main purpose of running tests on the SPEC benchmarks, collecting the results and building a table of processed results. Our scripts use the Test-Suite infrastructure but let the programmer define what will happen in the test using shell scripts.

# Requirements #

LLVM Test-Suite must be already configured to recognize the SPEC benchmarks. The version of the script that is available on our repository is prepared to run tests on the SPEC2006. Other versions of the SPEC benchmarks are supported as well, but our script will have to be modified to include the arguments of the new programs to be tested.

# How to run tests with the scripts #

In our repository in the folder [svn/trunk/tests/spec\_custom](http://code.google.com/p/range-analysis/source/browse/#svn%2Ftrunk%2Ftests%2Fspec_custom) there are the scripts that we will use to run our custom tests:
  * The files TEST.CompileAll.Makefile and TEST.CompileAll.report should stay in the folder llvm\_root/projects/test-suite
  * The rest of the scripts should stay in the folder llvm\_root/projects/test-suite/External/SPEC

After you put the files in the correct folders, you just need to open a terminal, go to the scripts folder and run the script:

```
$ cd llvm_root/projects/test-suite/External/SPEC
$ bash PerformanceTests.sh
```


# What are these scripts actually doing? #

The file TEST.CompileAll.Makefile is responsible for using the test-suite infrastructure to compile and link the source files, generating a
benchmark\_name.linked.rbc for each of the programs. One can alter the makefile if it is needed to add or remove something (The current version of the makefile already compiles the files with -g option, to insert debug info).

The script PerformanceTest.sh does the magic. It does the following:
  * Enter on each of the folders below SPEC where there is a file named benchmark\_name.linked.rbc
  * Generate an executable without optimization (I call it the "original executable")
  * Run the original executable with the correct arguments getting the running time and keeping the output in a temporary file
  * Run your optimization passes in the file benchmark\_name.linked.rbc
  * Generate an optimized executable
  * Run the optimized executable with the same arguments getting the running time
  * Compare the output of the two executables (original and optimized), to check if both are giving the same output
  * Generate an output file, in HTML

The script PerformanceTests.sh is useful if you need to run the test N times.
It calls PerformanceTest.sh varying the output filename in each iteration.


# What do I have to change in these scripts? #

All the changes below apply to the file PerformanceTest.sh


  * I want to change the optimization or change the informations that appear in the HTML report.
Between the lines 288 and 463 you can change the optimizations and add
or remove informations of the output file.
If you add or remove columns in the report, you should modify the
header of the HTML table. It is defined in the line 480

  * I want to run the SPEC benchmarks with a different set of arguments.
Modify the lines between 88 and 213.

  * I want to include other versions of SPEC to my tests.
Insert the new programs following the pattern that exists between the lines 88 and 213.


# Example #

The script PerformanceTest.CountOVF.sh is an example of a modified
version of the script PerformanceTest.sh that extracts other informations.