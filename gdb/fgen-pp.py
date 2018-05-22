#!/usr/bin/env python

#
# The MIT License (MIT)
#
# Copyright (c) <2018> Steffen Nüssle
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

#
# If "gdb" is up and running you can load this script manually with:
# (gdb) source <script.py> 
#

import gdb.printing

class LLVMStringRefPrinter:
    def __init__(self, val):
        self.val = val

    # There is 'array', 'map' and 'string'
    #def display_hint(self):
        #return 'map'
        
    def to_string(self):
        return 'llvm::StringRef'
    
    def children(self):
        size = self.val['Length']
        data = gdb.Value(self.val['Data'].string()[:size])

        return [
            ('.Data', data), 
            ('.Length', size)
        ]

def init():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('fgen-pp')
    pp.add_printer('llvm::StringRef', '^llvm::StringRef$', LLVMStringRefPrinter)
    
    return pp

if __name__ == '__main__':
    gdb.printing.register_pretty_printer(gdb.current_objfile(), init())
