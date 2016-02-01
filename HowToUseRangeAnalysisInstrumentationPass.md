# Introduction #

We have a profiler that finds, for each program variable, the lowest and highest values that the variable has stored during the program execution. This profiler has been very useful to us. We use it to verify how precise is our range analysis.

# Requirements #

Our profiler instruments the LLVM IR to record the values assigned to each variable. It receives, as input, a LLVM bitcode file. You can readily produce this file using `clang`. If your program is composed of more than one module, you can use the llvm-link tool to link all the bitcode files into one file. This file must contain the main function, otherwise the pass will not instrument the program.

# Tutorial #

We will illustrate our pass using the program below as an example.

```
#include <stdio.h>

/*
 * The basic case: the inter and intra-procedural analyses should return the
 * same results.
 */
int foo() {
  int k = 0;
  while (k < 100) {
    int i = 0;
    int j = k;
    while (i < j) {
      i = i + 1;
      j = j - 1;
    }
    k = k + 1;
  }
  return k;
}

int main(int argc, char** argv) {
  printf("%d\n", foo());
}
```


## Compiling and Running the Program ##

  * First, we can translate a c file into a bitcode file using clang:
```
clang -c -emit-llvm test.c -o test.bc
```
  * Next thing: we must convert the bitcode file to e-SSA form. We do it using the `vssa` pass. This step is not mandatory, but if you want to compare the results of the profiling with our range analysis, you'd better do it, otherwise the range analysis will be simply too imprecise.
```
opt -instnamer -mem2reg -break-crit-edges test.bc -o test.bc
opt -load LLVM_SRC_PATH/Debug/lib/vSSA.so -vssa test.bc -o test.essa.bc
```
> Notice that we use a number of other passes too, to improve the quality of the code that we are producing:
    * `instnamer` just assigns strings to each variable. This will make the dot files that we produce to look nicer. We only use this pass for aesthetic reasons.
    * `mem2reg` maps variables allocated in the stack to virtual registers. Without this pass everything is mapped into memory, and then our range analysis will not be able to find any meaningful ranges to the variables.
    * `break-crit-edges` removes the critical edges from the control flow graph of the input program. This will increase the precision of our range analysis (just a tiny bit though), because the e-SSA transformation will be able to insert more sigma-functions into the code.

  * Now, we can run our instrumentation pass. We can do this with the code below:
```
opt -load LLVM_SRC_PATH/Debug/lib/RAInstrumentation.so -ra-instrumentation test.essa.bc -f -o test.inst.bc
```
This step produces a file named RAHashNames.txt, containing all variables that we found int he program text, plus an integer code for each one. We will need this integer code to recover the values bound to each variable.

  * Our instrumentation needs to call a few external functions to manipulate the values inside the program. These functions are in the file `RAInstrumentationHash.c`. We have to get it in a bitcode file and then link it with the instrumented bitcode file.  We can do this with the code below:
```
clang -c -emit-llvm RAInstrumentationHash.c -o RAInstrumentationHash.bc
```

  * Now, we can link all the bitcode files. We can do this with the code below:
```
llvm-link -f -o=test.linked.bc test.inst.bc RAInstrumentationHash.bc 
```

  * To execute the instrumented file, we can use `lli`.
```
lli test.linked.bc
```
> Notice that in the end of the execution a file named `RAHashValues.txt` was created. This file contains the minimum and the maximum values that each variable has received at any moment of the program execution. Important: this is a profiler. Thus, its results might, and possibly will, depend on the program input.

  * Last thing: we need to join the `RAHashNames.txt` and `RAHashValues.txt` files. I have prepared a `awk` command to do it. It is just so amazing the wonders that one can do with `awk`!
```
awk 'ARGIND == 1 {vars[$2] = $1} ARGIND == 2 { print vars[$1], $2, $3}' RAHashNames.txt RAHashValues.txt
```

> The `awk` command produces this result, containing the name of each variable followed by its lowest and highest values assigned during the execution:
```
t1.essa.bc.foo.vSSA_sigma2 0 49
t1.essa.bc.foo.add 1 50
t1.essa.bc.foo.sub 0 98
t1.essa.bc.foo.add4 1 100
t1.essa.bc.foo.vSSA_sigma1 100 100
t1.essa.bc.main.call 100 100
t1.essa.bc.main.call1 4 4
t1.essa.bc.foo.j.0 0 99
t1.essa.bc.foo.i.0 0 50
t1.essa.bc.foo.vSSA_sigma3 1 99
t1.essa.bc.foo.vSSA_sigma 0 99
t1.essa.bc.foo.k.0 0 100
```