#
# linecnt - a source line counting utility
#
# Copyright (c) 2003-2021, Stone Steps Inc. (www.stonesteps.ca)
#
# See COPYING and Copyright files for additional licensing and copyright information
#

SHELL := /bin/bash

#
# Makefile parameters:
#
#	all targets:
#		BLDDIR=/path/to/build/dir (default ./build)
#
#	linecnt:
# 		AZP_BUILD_NUMBER=auto-incremented-build-number
#	
#	test:
#		TEST_RSLT_DIR=/path/to/test/results/directory (default BLDDIR)
#       TEST_RSLT_FILE=test-results-file-name (default utest.xml)
#

# delete all default suffixes
.SUFFIXES:

.PHONY: clean

# if there is no build directory supplied, use the default
ifeq ($(strip $(BLDDIR)),)
BLDDIR := build
endif

# install target directory
INSTDIR := /usr/local

#
# linecnt variables
#

SRCS := linecnt.cpp cpplexer.cpp
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

LEX_SRC := cpplexer_scanner.l
LEX_INC := $(LEX_SRC:.l=.inc)

LINECNT := linecnt

#
# utest variables
#

TEST_SRCS := test/ut_main.cpp test/ut_tests.cpp

TEST_OBJS := $(TEST_SRCS:.cpp=.o)  \
				cpplexer.o

TEST_DEPS := $(TEST_OBJS:.o=.d)

UTEST := utest

ifeq ($(strip $(TEST_RSLT_DIR)),)
TEST_RSLT_DIR := $(BLDDIR)
endif

ifeq ($(strip $(TEST_RSLT_FILE)),)
TEST_RSLT_FILE := $(UTEST).xml
endif

# C++ compiler flags
CXXFLAGS = -std=c++17 -fexceptions -Werror -pedantic

# if there's no debug information, turn on optimizations
ifeq ($(findstring -g,$(CXXFLAGS)),)
CXXFLAGS += -O3
endif

# if building in Azure DevOps, pass the build number into the source
ifdef AZP_BUILD_NUMBER
CXXFLAGS += -DBUILD_NUMBER=$(AZP_BUILD_NUMBER)
endif

#
# targets
#

# linecnt
$(BLDDIR)/$(LINECNT): $(addprefix $(BLDDIR)/,$(OBJS)) | $(BLDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

$(BLDDIR): 
	@mkdir -p $(BLDDIR)

# utest
$(BLDDIR)/$(UTEST): $(addprefix $(BLDDIR)/,$(TEST_OBJS)) | $(BLDDIR)/test
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++ -lpthread -lgtest

$(BLDDIR)/test:
	mkdir -p $(BLDDIR)/$(TEST)

test: $(BLDDIR)/$(UTEST)
	$(BLDDIR)/$(UTEST) --gtest_output=xml:$(TEST_RSLT_DIR)/$(TEST_RSLT_FILE)

install: $(BLDDIR)/$(LINECNT)
	@cp -f $(BLDDIR)/$(LINECNT) $(INSTDIR)/bin
	@if [[ ! -e $(INSTDIR)/share/doc/linecnt ]]; then mkdir -p $(INSTDIR)/share/doc/linecnt; fi
	@cp -f COPYING Copyright README.md $(INSTDIR)/share/doc/linecnt

uninstall:
	@rm -f $(INSTDIR)/bin/$(LINECNT)
	@rm -f $(INSTDIR)/share/doc/linecnt/{COPYING, Copyright, README.md}
	@rmdir $(INSTDIR)/share/doc/linecnt

clean:
	@rm -f $(BLDDIR)/$(LINECNT)
	@rm -f $(addprefix $(BLDDIR)/, $(OBJS))
	@rm -f $(addprefix $(BLDDIR)/, $(DEPS))
	@rm -f $(LEX_INC)
	@rm -f $(BLDDIR)/$(UTEST)
	@# use TEST_SRCS because TEST_OBJS has some of linecnt files
	@rm -f $(addprefix $(BLDDIR)/, $(TEST_SRCS:.cpp=.o))
	@rm -f $(addprefix $(BLDDIR)/, $(TEST_SRCS:.cpp=.d))
	@rm -f $(TEST_RSLT_DIR)/$(TEST_RSLT_FILE)

#
# Dependency tracking fails for the Lexer-generated include file
# because GCC needs all include files present when .d files are
# being generated. GNU Make has some suggestions to solve this
# on this page:
#
# https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html
#
# BUILT_SOURCES only works for phony targets and using cpplexer_imp.o
# as a target fails because dependencies are being built first and
# fail. Using Lexer-generated files as a prerequisite of C++ source
# including them triggers the Lexer rule. We also need to touch the
# source file to make sure it is recompiled with new Lexer output.
#
cpplexer.cpp : $(LEX_INC)
	touch cpplexer.cpp

#
# pattern rules
#

$(BLDDIR)/%.d : %.cpp
	if [[ ! -e $(@D) ]]; then mkdir -p $(@D); fi
	set -e; $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $< | \
	sed 's|^[ \t]*$*\.o[ \t]*:|$(BLDDIR)/$*.o $@: |g' > $@;

$(BLDDIR)/%.o : %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $(CXXFLAGS) $<

%.inc : %.l
	$(LEX) --outfile=$@ $<

# include dependencies when the primary target is being built (default or explicit)
ifeq ($(MAKECMDGOALS),)
include $(addprefix $(BLDDIR)/, $(DEPS))
else ifneq ($(filter $(BLDDIR)/$(LINECNT),$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS))
endif

# unit test dependencies
ifneq ($(filter test,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(TEST_DEPS))
else ifneq ($(filter $(BLDDIR)/$(UTEST),$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(TEST_DEPS))
endif
