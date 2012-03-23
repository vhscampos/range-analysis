##===- lib/Transforms/RAInstrumentation/Makefile -----------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../../..
LIBRARYNAME = RAPrinter
LOADABLE_MODULE = 1
USEDLIBS = 

# If we don't need RTTI or EH, there's no reason to export anything
# from the RAInstrumentation plugin.
ifneq ($(REQUIRES_RTTI), 1)
ifneq ($(REQUIRES_EH), 1)
EXPORTED_SYMBOL_FILE = $(PROJ_SRC_DIR)/RAPrinter.exports
endif
endif

include $(LEVEL)/Makefile.common

