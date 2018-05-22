# Work in progress.

# fgen - function generator for C and C++

## Overview

## Motivation

## Advantages

## Disadvantages

## Project Status

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

```
    $ fgen --help
```

## Bugs and Bug Reports

# What can be improved on in the future?

* Set Accessors could automatically use move semantics if they are working on a copy.
* Accessors: better handling of pointers and references.
* Automatically implement get-accessors for const pointers and const references.
