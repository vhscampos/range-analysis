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

LLVM_DIR = "/work/victorsc"
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
	@if [ -n "$(INLINE)" ]; then \
		@$(LOPT) -mem2reg -internalize -inline -instcount -stats -time-passes -disable-output $< 2>>$@
	else \
		@$(LOPT) -mem2reg -instcount -stats -time-passes -disable-output $< 2>>$@
	fi

REPORT_DEPENDENCIES := $(LOPT)
