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

LLVM_DIR = "/home/vhscampos/IC"
LLVM_BUILD = "Debug+Asserts"


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
	@if [ -n "$(ESSA)" ]; then \
		$(LOPT) -load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/vSSA.so -vssa \
		-load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RangeAnalysis.so \
		$(ANALYSIS) -instcount -stats -time-passes -disable-output $<.more.bc 2>>$@; \
	else \
		$(LOPT) -load $(LLVM_DIR)/llvm-3.0/$(LLVM_BUILD)/lib/RangeAnalysis.so \
		$(ANALYSIS) -instcount -stats -time-passes -disable-output $<.more.bc 2>>$@; \
	fi

REPORT_DEPENDENCIES := $(LOPT)
