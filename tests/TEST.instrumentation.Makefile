##===- TEST.ra.Makefile ------------------------------*- Makefile -*-===##
#
# Usage: 
#     make TEST=ra (detailed list with time passes, etc.)
#     make TEST=ra report
#     make TEST=ra report.html
#
##===----------------------------------------------------------------------===##

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(PROJ_SRC_ROOT)
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

LLVM_DIR = "/home/raphael/workspace/ra"
LLVM_BUILD = "Debug"


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@cat $<

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt):  \
Output/%.$(TEST).report.txt: Output/%.linked.rbc $(LOPT) \
	$(PROJ_SRC_ROOT)/TEST.ra.Makefile 
	$(VERB) $(RM) -f $@
	@echo "---------------------------------------------------------------" >> $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------------" >> $@
	@-$(LOPT) -mem2reg -stats -time-passes $< > $<.m2r.bc 2>>$@
	@if [ -n "$(INLINE)" ]; then \
		$(LOPT) -internalize -inline -break-crit-edges -instnamer -stats -time-passes $<.m2r.bc > $<.more.bc 2>>$@; \
	else \
		$(LOPT) -break-crit-edges -instnamer -stats -time-passes $<.m2r.bc > $<.more.bc 2>>$@; \
	fi
	clang $(LLVM_DIR)/llvm-3.0/lib/Transforms/RAInstrumentation/RAInstrumentationHash.c -c -emit-llvm -o $(LLVM_DIR)/llvm-3.0/lib/Transforms/RAInstrumentation/RAInstrumentationHash.bc
	@if [ -n "$(ESSA)" ]; then \
		$(LOPT) -load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/vSSA.so -vssa \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RangeAnalysis.so \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RAInstrumentation.so \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RAPrinter.so \
		$(ANALYSIS) -ra-instrumentation -instcount -stats -time-passes $<.more.bc > $<.instr.bc 2>>$@; \
	else \
		$(LOPT) -load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RangeAnalysis.so \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RAInstrumentation.so \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RAPrinter.so \
		$(ANALYSIS) -ra-instrumentation -instcount -stats -time-passes $<.more.bc > $<.instr.bc 2>>$@; \
	fi
	llvm-link -o=$<.linked.bc $<.instr.bc $(LLVM_DIR)/llvm-3.0/lib/Transforms/RAInstrumentation/RAInstrumentationHash.bc
	lli $<.linked.bc

REPORT_DEPENDENCIES := $(LOPT)
