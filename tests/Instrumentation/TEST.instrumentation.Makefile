##===- TEST.instrumentation.Makefile ------------------------------*- Makefile -*-===##
#
# Usage: 
#     make TEST=instrumentation (detailed list with time passes, etc.)
#     make TEST=instrumentation report
#     make TEST=instrumentation report.html
#
# Required arguments:
#     ANALYSIS={-ra-printer-inter-cousot|...}  ==> which analysis the ra-printer should use
#     OUTDIR={Directory} ==> directory to put the text files produced by the analysis. 
#                            The directory should already exist.
# Optional arguments:
#     ESSA=1  ==> use the e-SSA representation
#
# Example:
#     make TEST=instrumentation ESSA=1 ANALYSIS=-ra-printer-inter-cousot OUTDIR=/home/raphael/benchmarkResults/
##===-----------------------------------------------------------------------------===##

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
	$(VERB) $(RM) -f /tmp/RAEstimatedValues*.txt /tmp/RAHashNames*.txt /tmp/RAHashValues*.txt
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
	mv /tmp/RAEstimatedValues*.txt $(OUTDIR)
	mv /tmp/RAHashNames*.txt $(OUTDIR)
	mv /tmp/RAHashValues*.txt $(OUTDIR)
	$(VERB) $(RM) -f /tmp/RAEstimatedValues*.txt /tmp/RAHashNames*.txt /tmp/RAHashValues*.txt

REPORT_DEPENDENCIES := $(LOPT)
