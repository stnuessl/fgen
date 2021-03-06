# fgen - function generator for C and C++

## Overview
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
        * [General](README.md#general)
        * [Examples](README.md#examples)
            * [Option "-fstubs"](README.md#option--fstubs)
            * [Option "-faccessors"](README.md#option--faccessors)
            * [Option "-fconversions"](README.md#option--fconversions)
            * [Option "-fmove"](README.md#option--fmove)
            * [Option "-fnamespace-definitions"](README.md#option--fnamespace-definitions)
            * [Option "-fcontains"](README.md#option--fcontains)
        * [Formatted Output](README.md#formatted-output)
    * [Troubleshooting](README.md#troubleshooting)
    * [Bugs and Bug Reports](README.md#bugs-and-bug-reports)

## Motivation

TBD

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
* Add some sort of configuration file to enable the user to set some default
flags / arguments.
* Add an option to automatically include the passed header files.

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

This section describes how to use __fgen__. We first start with some general
information before going into details with a few explicit examples. Note that
for the sake of brevity not __all__ options are discussed here. 

### General

As of now __fgen__'s main focus is to be run on header (.h / .hpp) files.
So a typical __fgen__ invocation will look like this

```
$ fgen [<file> ...]
```

Although __fgen__ can process multiple files per invocation (you can specify a
list of files on the command line), it won't usually make sense to do so.

The generated output of __fgen__ will be written to stdout. To start with your
implementation pipe the produced output to a file.

```
$ fgen [<file> ...] > file.cpp
```

The output of __fgen__ can be controlled with command line arguments 
which are used to enable or disable __fgen__'s options.
To get an overview and some short documentation over all available 
options you can run

```
$ fgen -help
```

Most of these options will be discussed in the [Examples](README.md#examples)
section down below. If you are already familiar with most of the options you 
can just double press the tabulator key to invoke the bash-completion for 
suggestions.

As of now __fgen__ is run with some options enabled by default. This means
that the following two commands are equal:

```
$ fgen [<file> ...]
$ fgen -faccessors -fconversions -fmove -fnamespaces -fstubs [<file> ...]
```

To turn an option off, the arguments have to be passed like this:

```
$ fgen -faccessors=false -fconversions=false [...] [<file> ...]
```

The strings __"true", "TRUE", "True", "1", "false", "FALSE", "False", "0"__
are supported for boolean arguments.


### Examples

For the following examples we will consider the following header file 
_example.hpp_:

```
#ifndef FGEN_EXAMPLE_HPP_
#define FGEN_EXAMPLE_HPP_

#include <vector>

namespace ns {

class example {
public:
    example() = default;
    virtual ~example();
    
    void set_valid(bool valid);
    bool valid() const;
    
    void set_vec(std::vector<int> vec);
    void set_vec(std::vector<int> &vec);
    
    std::vector<int> &vec();
    const std::vector<int> &vec() const;
    
    operator bool() const;
private:
    unsigned char valid_ : 1;
    
    std::vector<int> vec_;
};

example create();

}

bool operator==(const ns::example &lhs, const ns::example &rhs);


#endif /* FGEN_EXAMPLE_HPP_ */
```

By default most of the useful __fgen__ options are already enabled. For the 
sake of simplicity, we will turn those options off manually and then reenable
them one after the other to understand what they do. The only exception is
"-ftrim", which will be used consistently to get a terse output. For the same
reason, we will not pipe the output to 
[clang-format](https://clang.llvm.org/docs/ClangFormat.html).
So the first command we run is

```
$ fgen -ftrim -fstubs=false -faccessors=false -fconversions=false -fmove=false example.hpp
```
and the result of this specific __fgen__ invocation is:
```
namespace ns {
example::~example() {}
void example::set_valid(bool valid) {}
bool example::valid() const {}
void example::set_vec(std::vector<int> vec) {}
void example::set_vec(std::vector<int> &vec) {}
std::vector<int> &example::vec() {}
const std::vector<int> &example::vec() const {}
example::operator bool() const {}
ns::example create() {}
}
bool operator==(const ns::example &lhs, const ns::example &rhs) {}
```

#### Option "-fstubs"

For the next command we will turn on the "-fstubs" option, which tries to
implement a simple function body to make the result instantly compileable.
```
$ fgen -ftrim -fstubs -faccessors=false -fconversions=false -fmove=false example.hpp
```
Generated output:
```
namespace ns {
example::~example() {}
void example::set_valid(bool valid) {}
bool example::valid() const { return false; }
void example::set_vec(std::vector<int> vec) {}
void example::set_vec(std::vector<int> &vec) {}
std::vector<int> &example::vec() { static std::vector<int> stub_dummy_; return stub_dummy_; }
const std::vector<int> &example::vec() const { static const std::vector<int> stub_dummy_; return stub_dummy_; }
example::operator bool() const { return false; }
ns::example create() { return ns::example(); }
}
bool operator==(const ns::example &lhs, const ns::example &rhs) { return false; }
```

This looks like it should compile, however, the implemented stubs are probably 
not what we want. So can __fgen__ do better? Let's see what happens when
we turn "-faccessors" on.

#### Option "-faccessors"

The __fgen__ invocation with "-faccessors" turned on reads
```
$ fgen -ftrim -fstubs -faccessors -fconversions=false -fmove=false example.hpp
```
and the generated output is:
```
namespace ns {
example::~example() {}
void example::set_valid(bool valid) { valid_ = valid; }
bool example::valid() const { return valid_; }
void example::set_vec(std::vector<int> vec) { vec_ = vec; }
void example::set_vec(std::vector<int> &vec) {}
std::vector<int> &example::vec() { return vec_; }
const std::vector<int> &example::vec() const { return vec_; }
example::operator bool() const { return false; }
ns::example create() { return ns::example(); }
}
bool operator==(const ns::example &lhs, const ns::example &rhs) { return false; }
```
__fgen__ automatically detects functions which appeared to be simple
accessor functions and implemented them. This is probably a better starting
point for our own implementation than the examples before. 

In the next step, we turn on the "-fconversions" option.

#### Option "-fconversions"

```
$ fgen -ftrim -fstubs -faccessors -fconversions -fmove=false example.hpp
```
Because there is so little change in the generated output, 
only the interesting part is shown here.
```
...
example::operator bool() const { return valid_; }
...
```
Ok, so __fgen__ automatically implemented the conversion operator for this 
class. Again, this implementation seems to be more useful than the previous one.

#### Option "-fmove"
```
$ fgen -ftrim -fstubs -faccessors -fconversions -fmove example.hpp
```
Note that the next __fgen__ invocations is equivalent to the previous one, 
because the left out options are enabled by default.
```
fgen -ftrim example.hpp
```
Generated output, again trimmed to the relevant sections:
```
#include <utility>

namespace ns {
...
void example::set_vec(std::vector<int> vec) { vec_ = std::move(vec); }
...
```

If the option "-fmove" is enabled, __fgen__ detects if the move assignment
operation can and should be used. It also adds the necessary include directive
to the produced code. In most cases this should result in a little performance
improvement of the compiled program and it can't be forgotten by the programmer.

#### Option "-fnamespace-definitions"

If the option "-fnamespace-definitions". is disabled, __fgen__ won't print
any namespace definitions, but instead the namespace declaration name will be 
used in the qualified function name.

```
$ fgen -ftrim -fnamespace-definitions=false example.hpp
```
Generated output:
```
#include <utility>

ns::example::~example() {}
void ns::example::set_valid(bool valid) { valid_ = valid; }
bool ns::example::valid() const { return valid_; }
void ns::example::set_vec(std::vector<int> vec) { vec_ = std::move(vec); }
void ns::example::set_vec(std::vector<int> &vec) {}
std::vector<int> &ns::example::vec() { return vec_; }
const std::vector<int> &ns::example::vec() const { return vec_; }
ns::example::operator bool() const { return valid_; }
ns::example ns::create() { return ns::example(); }
bool operator==(const ns::example &lhs, const ns::example &rhs) { return false; }
```

Finally, there is one last options left, which needs to be discussed.

#### Option "-fcontains"

The "-fcontains" option can be used the access only relevant function 
declarations in the passed header files. Functions will be included in the 
generated output if the additionally passed string is contained somehow in
the fully qualified function name.

```
$ fgen -ftrim -fcontains=set example.hpp
```
Generated output:
```
include <utility>

namespace ns {
void example::set_valid(bool valid) { valid_ = valid; }
void example::set_vec(std::vector<int> vec) { vec_ = std::move(vec); }
void example::set_vec(std::vector<int> &vec) {}

}
```

Additional functions can be accessed if multiple strings are passed to 
"-fcontains".

```
    $ fgen -ftrim -fcontains=set,operator== example.hpp
```
Generated output:
```
#include <utility>

namespace ns {
void example::set_valid(bool valid) { valid_ = valid; }
void example::set_vec(std::vector<int> vec) { vec_ = std::move(vec); }
void example::set_vec(std::vector<int> &vec) {}
}
bool operator==(const ns::example &lhs, const ns::example &rhs) { return false; }
```

### Formatted Output

It is almost impossible for __fgen__ to know how to format the code it generates
according to the user's wishes. Luckily, there are dedicated programs for this
such as [clang-format](https://clang.llvm.org/docs/ClangFormat.html).
The recommended way to get the __fgen__ output formatted is to pipe it
to such a standalone tool, e.g.:

```
$ fgen [<file> ...] | clang-format 
```

Using the command for "example.hpp" from above as an example we get

```
$ fgen example.hpp | clang-format 
```

Depending on the clang-format configuration, this produces something like
the following output.

```
#include <utility>

namespace ns {

example::~example()
{}

void example::set_valid(bool valid)
{
    valid_ = valid;
}

bool example::valid() const
{
    return valid_;
}

void example::set_vec(std::vector<int> vec)
{
    vec_ = std::move(vec);
}

void example::set_vec(std::vector<int> &vec)
{}

std::vector<int> &example::vec()
{
    return vec_;
}

const std::vector<int> &example::vec() const
{
    return vec_;
}

example::operator bool() const
{
    return valid_;
}

ns::example create()
{
    return ns::example();
}
}

bool operator==(const ns::example &lhs, const ns::example &rhs)
{
    return false;
}

```

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
