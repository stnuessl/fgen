#
# The MIT License (MIT)
#
# Copyright (c) <2015> Steffen Nüssle
# Copyright (c) <2016> Steffen Nüssle
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

CC		:= /usr/bin/clang
CXX		:= /usr/bin/clang++

#
# Show / suppress compiler invocations. 
# Set 'SUPP :=' to show them.
# Set 'SUPP := @' to suppress compiler invocations.
#
SUPP		:=

#
# Set name of the binary
#
BIN			:= fgen

# Need to install / deinstall the bash completion
BASH_COMPLETION_SRC	:= bash-completion/fgen
BASH_COMPLETION_DIR	:= /usr/share/bash-completion/completions/
BASH_COMPLETION_TARGET	:= /usr/share/bash-completion/completions/fgen

ifndef BIN
$(error No binary name specified)
endif

#
# Specify all source files. The paths should be relative to this file.
#
# SRC 		:= $(shell find ./ -iname "*.c")
SRC 		:= $(shell find src/ -iname "*.cpp")
# SRC 		:= $(shell find ./ -iname "*.c" -o -iname "*.cpp")
HDR			:= $(shell find src/ -iname "*.hpp")

ifndef SRC
$(error No source files specified)
endif


#
# Uncomment if 'VPATH' is needed. 'VPATH' is a list of directories in which
# make searches for source files.
#
EMPTY		:=
SPACE		:= $(EMPTY) $(EMPTY)
# VPATH 	:= $(subst $(SPACE),:,$(sort $(dir $(SRC))))

RELPATHS	:= $(filter ../%, $(SRC))
ifdef RELPATHS

NAMES		:= $(notdir $(RELPATHS))
UNIQUE		:= $(sort $(notdir $(RELPATHS)))

#
# Check for duplicate file names (not regarding directories)
#
ifneq ($(words $(NAMES)),$(words $(UNIQUE)))
DUPS		:= $(shell printf "$(NAMES)" | tr -s " " "\n" | sort | uniq -d)
DIRS		:= $(dir $(filter %$(DUPS), $(SRC)))
$(error [ $(DUPS) ] occur(s) in two or more relative paths [ $(DIRS) ] - not supported)
endif

#
# Only use file name as the source location and add the relative path to 'VPATH'
# This prevents object files to reside in paths like 'build/src/../relative/' or
# even worse 'build/src/../../relative' which would be a path outside of
# the specified build directory
#
SRC			:= $(filter-out ../%, $(SRC)) $(notdir $(RELPATHS))
VPATH		:= $(subst $(SPACE),:, $(dir $(RELPATHS)))
endif


#
# Paths for the build-, objects- and dependency-directories
#
BUILDDIR	:= build
TARGET 		:= $(BUILDDIR)/$(BIN)

#
# Set installation directory used in 'make install'.
# It is essential to place clang related tools in "/usr/bin"
# as they expect their builtin headers relative to this directory.
#
INSTALL_DIR	:= /usr/bin/

#
# Generate all object and dependency files from $(SRC) and get
# a list of all inhabited directories. 'AUX' is used to prevent file paths
# like build/objs/./srcdir/
#
AUX			:= $(patsubst ./%, %, $(SRC))
C_SRC		:= $(filter %.c, $(AUX))
CXX_SRC		:= $(filter %.cpp, $(AUX))
C_OBJS		:= $(addprefix $(BUILDDIR)/, $(patsubst %.c, %.o, $(C_SRC)))
CXX_OBJS	:= $(addprefix $(BUILDDIR)/, $(patsubst %.cpp, %.o, $(CXX_SRC)))
OBJS		:= $(C_OBJS) $(CXX_OBJS)
DIRS		:= $(BUILDDIR) $(sort $(dir $(OBJS)))

#
# Define dependency and JSON compilation database files.
#
DEPS		:= $(patsubst %.o, %.d, $(OBJS))
JSON		:= $(patsubst %.o, %.json, $(OBJS))


#
# Add additional preprocessor definitions
#
DEFS		:= \
#		-D_GNU_SOURCE \
#		-DNDEBUG \

#
# Add additional include paths. The "/usr/lib/clang/x.x.x/include" directory
# makes it easy to use clang-tools on the software build with this Makefile.
# 
#
INCLUDE		:= \
		-I./src

#
# Add used libraries which are configurable with pkg-config
#
PKGCONF		:= \
# 		gstreamer-1.0 \
# 		gstreamer-pbutils-1.0 \
# 		libcurl \
# 		libxml-2.0 \

#
# Set non-pkg-configurable libraries flags 
#
LIBS		:= \
		-Wl,--start-group \
		-lclangAST \
		-lclangBasic \
		-lclangFrontend \
		-lclangFrontendTool \
		-lclangIndex \
		-lclangLex \
		-lclangParse \
		-lclangSerialization \
		-lclangTooling \
		-lclangToolingCore \
		-Wl,--end-group \
		$(shell llvm-config --libs) \
		$(shell llvm-config --system-libs) \
#		-lclangAnalysis \
# 		-lclangASTMatchers \
#		-lclangDriver \
#		-lclangEdit \
# 		-lclangFormat \
# 		-lclangRewrite \
# 		-lclangRewriteFrontend \
#		-lclangSema \
# 		-pthread \
# 		-lm \
# 		-Wl,--start-group \
# 		-Wl,--end-group \

#
# Set linker flags, here: 'rpath' for libraries in non-standard directories
# If '-shared' is specified: '-fpic' or '-fPIC' should be set here 
# as in the CFLAGS / CXXFLAGS
#
LDFLAGS		:= \
		$(shell llvm-config --ldflags)

LDLIBS		:= $(LIBS)


CPPFLAGS	= \
		$(DEFS) \
		$(INCLUDE) \
		-MMD \
		-MF $(patsubst %.o,%.d,$@) \
		-MT $@ \
# 		$(shell llvm-config --cppflags)

# If clang is used, generate a compilation database for each
# translation unit. 
ifeq (clang, $(findstring clang, $(CC) $(CXX)))
CPPFLAGS	+= -MJ $(patsubst %.o, %.json, $@)
endif

#
# Set additional compiler flags
#
CXXFLAGS	:= \
		-fno-rtti \
		-fno-exceptions \
		-std=c++17 \
		-fPIC \
		-fno-plt \
		-fstack-protector-strong \
		-march=x86-64 \
		-mtune=generic \
 		-Wall \
 		-Wextra \
 		-Wpedantic \
# 		-Werror \
# 		-Weffc++ \
# 		-O2 \
# 		-g3 \
# 		-fno-omit-frame-pointer \
#		-fpic \

#
# Check if specified pkg-config libraries are available and abort
# if they are not.
#
ifdef PKGCONF

OK		:= $(shell pkg-config --exists $(PKGCONF) && printf "OK")
ifndef $(OK)
PKGS 		:= $(shell pkg-config --list-all | cut -f1 -d " ")
FOUND		:= $(sort $(filter $(PKGCONF),$(PKGS)))
$(error Missing pkg-config libraries: [ $(filter-out $(FOUND),$(PKGCONF)) ])
endif

CFLAGS		+= $(shell pkg-config --cflags $(PKGCONF))
CXXFLAGS	+= $(shell pkg-config --cflags $(PKGCONF))
LDLIBS		+= $(shell pkg-config --libs $(PKGCONF))

endif


#
# Setting terminal colors. 
#
RED			:= \e[1;31m
GREEN		:= \e[1;32m
YELLOW		:= \e[1;33m
BLUE		:= \e[1;34m
MAGENTA		:= \e[1;35m
CYAN		:= \e[1;36m
RESET		:= \e[0m

#
# Get the MD5 hash value of a file passed as an argument.
#
md5sum		= $$(md5sum $(1) | cut -f1 -d " ")

# release: CPPFLAGS	+= -DNDEBUG
release: CFLAGS		+= -O3 -flto -fdata-sections -ffunction-sections
release: CXXFLAGS	+= -O3 -flto -fdata-sections -ffunction-sections
release: LDFLAGS	+= -O3 -flto -Wl,--gc-sections
release: $(TARGET)

debug: CFLAGS		+= -Og -g2
debug: CXXFLAGS		+= -Og -g2
debug: $(TARGET)

SANITIZERS	:= \
		with-addr-sanitizer \
		with-mem-sanitizer \
		with-thread-sanitizer \
		with-ub-sanitizer

$(SANITIZERS): CXXFLAGS			+= -O1 -g2 -fno-omit-frame-pointer

with-addr-sanitizer: CXXFLAGS 	+= -fsanitize=address
with-addr-sanitizer: LDFLAGS  	+= -fsanitize=address

# This option is only available on clang -- (clang 3.9.1 / gcc 6.3.1)
with-mem-sanitizer: CXX		:= /usr/bin/clang++
with-mem-sanitizer: CXXFLAGS 	+= -fsanitize=memory
with-mem-sanitizer: LDFLAGS  	+= -fsanitize=memory

with-thread-sanitizer: CXXFLAGS += -fsanitize=thread
with-thread-sanitizer: LDFLAGS	+= -fsanitize=thread

with-ub-sanitizer: CXXFLAGS	+= -fsanitize=undefined
with-ub-sanitizer: LDFLAGS 	+= -fsanitize=undefined

$(SANITIZERS): $(TARGET) 

syntax-check: CFLAGS 	+= -fsyntax-only
syntax-check: CXXFLAGS 	+= -fsyntax-only
syntax-check: $(OBJS)


all: $(TARGET)

$(TARGET): $(OBJS)
	@printf "$(YELLOW)Linking [ $@ ]$(RESET)\n"
	$(SUPP)$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@printf "$(GREEN)Built target [ $@ ]: $(call md5sum, $@)$(RESET)\n"

-include $(DEPS)

$(BUILDDIR)/%.o: %.cpp
	@printf "$(BLUE)Building: $@$(RESET)\n"
	$(SUPP)$(CXX) -c -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

$(OBJS): | $(DIRS)

$(DIRS):
	mkdir -p $(DIRS)

compile_commands.json: $(OBJS)
	sed -e '1s/^/[/' -e '$$s/,\s*$$/]/' $(JSON) | json_pp > $@

format:
	clang-format -i $(HDR) $(SRC)

clean:
	rm -rf $(TARGET) $(DIRS) $(COMPDB)

tags: $(HDR) $(SRC)
	ctags -f tags $^

install: $(TARGET)
	cp $(TARGET) $(INSTALL_DIR)
	cp $(BASH_COMPLETION_SRC) $(BASH_COMPLETION_DIR)

uninstall:
	rm -f $(INSTALL_DIR)$(BIN) $(BASH_COMPLETION_TARGET)
	

.PHONY: \
	all \
	clean \
	debug \
	format \
	install \
	release \
	syntax-check \
	uninstall \

.SILENT: \
	clean \
	compile_commands.json \
	format \
	tags \
	$(DIRS)
