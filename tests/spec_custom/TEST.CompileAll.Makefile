##===- TEST.CompileAll.Makefile -------------------------*- Makefile -*-===##
#
# This recursively traverses the programs, and runs the range-analysis pass
# on each *.linked.rbc bytecode file with -stats set so that it is possible to
# determine which CompileAll are being analysed in which programs.
# 
# Usage: 
#     make TEST=CompileAll summary (short summary)
#     make TEST=CompileAll (detailed list with time passes, etc.)
#     make TEST=CompileAll report
#     make TEST=CompileAll report.html
#
##===----------------------------------------------------------------------===##

TEST_TARGET_FLAGS = -g -O2

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(PROJ_SRC_ROOT)
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))



$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@cat $<

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt):  \
Output/%.$(TEST).report.txt: Output/%.linked.rbc $(LOPT) \
	$(PROJ_SRC_ROOT)/TEST.CompileAll.Makefile 
	$(VERB) $(RM) -f $@
	@echo "---------------------------------------------------------" >> $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------" >> $@

	@-$(LOPT) -mem2reg -instnamer -break-crit-edges -inline -stats -time-passes $< > $<.m2r.rbc 2>>$@

summary:
	@$(MAKE) TEST=CompileAll

.PHONY: summary
REPORT_DEPENDENCIES := $(LOPT)

