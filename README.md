# Work in progress.

# fgen - function generator for C and C++

## Overview
* [Work in progress.](README.md#work-in-progress)
* [fgen - function generator for C and C++](README.md#fgen---function-generator-for-c-and-c)
    * [Overview](README.md#overview)
    * [Motivation](README.md#motivation)
    * [Advantages](README.md#advantages)
    * [Disadvantages](README.md#disadvantages)
    * [Project Status](README.md#project-status)
        * [Features](README.md#features)
        * [What are possible improvements for the future?](README.md#what-are-possible-improvements-for-the-future)
    * [Installation](README.md#installation)
        * [Toolchain](README.md#toolchain)
        * [Dependencies](README.md#dependencies)
            * [Arch Linux](README.md#arch-linux)
        * [Compiling](README.md#compiling)
    * [Usage](README.md#usage)
    * [Troubleshooting](README.md#troubleshooting)
    * [Bugs and Bug Reports](README.md#bugs-and-bug-reports)


## Motivation

## Advantages

Here are some of the advantages I experienced will working on and with __fgen__.
Using __fgen__ ...

* ... is faster than manually writing or copying the function 
declarations from header files to source files.
* ... will reduce mistakes. Ever accidentally forgot to remove the trailing 
";" of the function declaration? Or forgot to remove the "virtual" qualifier
of a function? __fgen__ will not  make such mistakes therefore enabling a 
faster edit - compile - debug cycle.
* ... is not dependant on any graphical user interface. Use __fgen__ with your
favorite text editor.

## Disadvantages

* The lack of a graphical user interface makes __fgen__ less accessible
for users unfamiliar with a shell.
* Depending on the build system it can be quite hard to get a 
compilation database.
* Overkill if used on a file with only a few functions.

## Project Status

__fgen__ should compile and run. The code is not very well tested and still in
development. Use at your own risk and expect some bugs, however __fgen__ treats
all input files as read only, so if you do not overwrite your own files
your code should be save.

### Features

* Create empty function definitions from parsing function declarations.
* Automatically implement accessor functions.
* Automatically fill in return stubs (if possible).

### What are possible improvements for the future?

* Suppress (syntax) errors or at least give an option to do so. 
There are much better tools out there for this anyway, so a user should be 
primarily concerned about the generated output of __fgen__.
* Correctly handle namespaces by putting them before the corresponding 
function definitions (but keep the option to put them in the function name 
qualifiers as an alternative.)
* Add some sort of configuration file to enable the user to set some default
flags / arguments.
* Options to setup the generated output: maybe one line per generated function,
which would enable someone to do the following 
__$ fgen \<file> | grep \<pattern> | clang-format__

## Installation

This section describes a possible installation process for __fgen__. 

### Toolchain

For downloading, compiling and installing __fgen__ you will need

* g++ / clang++ (supporting at least C++11)
* make
* git

Make sure these programs are installed on your system.

### Dependencies

__fgen__ has the following dependencies:

* [llvm](http://llvm.org/)
* [clang](http://clang.llvm.org/)

Sometimes it is hard to get the correct versions, as _llvm_ and _clang_
like to change their APIs and thus breaking the compilation process of this
project. I try to keep the software up-to date with the versions available on 
the official _Arch Linux_ package repository 
([llvm](https://www.archlinux.org/packages/extra/x86_64/llvm/) / 
[clang](https://www.archlinux.org/packages/extra/x86_64/clang/)).
If I release a new version of __fgen__, there will be seperate information
about the required versions.

#### Arch Linux

The above listed programs and dependencies can be installed with the 
following _pacman_ invocation:

```
    # pacman -Syu llvm clang gcc make git
```

### Compiling

First you need to download the source code from this repository and 
change to the project directory. Run:
```
    $ git clone https://github.com/stnuessl/fgen
    $ cd fgen/
```
After that you need to compile the source code. This can be done with:
```
    $ make
```
If you want to compile with _clang++_ change the command to
```
    $ make CXX=clang++
```

The last command installs the __bash-completion__ and the __fgen__ binary on your
system:
```
    $ make install
```

## Usage

An overview over all __fgen__ options is available with:

```
    $ fgen -help
```

As of now __fgen__ is run with some options enabled by default. This means
that the following two command are equal:

```
    $ fgen [<file> ...]
    $ fgen -faccessors -fconversions -fmove -fnamespaces -fstubs [<file> ...]
```

To turn an option off the arguments have to passed like this:

```
    $ fgen -faccessors=false -fconversions=false [...] [<file> ...]
```

The strings __"true", "TRUE", "True", "1", "false", "FALSE", "False", "0"__
are supported for boolean arguments.

## Troubleshooting
    
## Bugs and Bug Reports

You've found a bug in __fgen__? That's actually great (at least for me), 
because this means I can improve my project.
I really appreciate any bug reports as long as they contain the following 
information:

* The __fgen__ invocation which does __not__ produce the desired result.
* A __minimal__ and working program (source code, the shorter the better) 
which the __fgen__ invocation has to be run on.
* A description of the desired / expected result.
* A description of the actual result with emphasis on the differences between
the desired and actual result.
* A trivial and working _compile_commands.json_ or _compile_flags.txt_, like:
```
[{
    "file": "main.cpp",
    "directory": "/path/to/directory/",
    "command": "g++ -std=c++11 -o test /path/to/directory/main.cpp"
}]
```
or
```
-std=c++11
-DMY_PP_MACRO_DEFS
-Imy/include/directories
(...)
```

If you succeed in providing these requirements I will try to fix the 
experienced bug.
