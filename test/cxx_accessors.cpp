/*
 * Copyright (C) 2018  Steffen Nüssle
 * fgen - method generator
 *
 * This file is part of fgen.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

class c1 {
public:
    int a() const;
    int b() const;
    int aa() const;
    int ab() const;
    int bb() const;
    
    void set_a(int);
    void set_b(int);
    void set_aa(int);
    void set_ab(int);
    void set_bb(int);
private:
    int a_, b_, aa_, ab_, bb_;
};

class c2 {
public:
    int get_a() const;
    int get_b() const;
    int get_aa() const;
    int get_ab() const;
    int get_bb() const;
    
    void set_a(int val);
    void set_b(int val);
    void set_aa(int val);
    void set_ab(int val);
    void set_bb(int val);
private:
    int a, b, aa, ab, bb;
};

class C3 {
public:
    unsigned long var1() const;
    unsigned long var2() const;
    unsigned long var3() const;
    
    void setVar1(unsigned long Val);
    void setVar2(unsigned long Val);
    void setVar3(unsigned long Val);

private:
    unsigned long Var1, Var2, Var3; 
};

class C4 {
public:
    double varVar1() const;
    double varVar2() const;
    double varVar3() const;
    
    void setVarVar1(double Val);
    void setVarVar2(double Val);
    void setVarVar3(double Val);
    
private:
    double VarVar1, VarVar2, VarVar3; 
};

class c5 {
public:
    /* All these functions should have empty bodys */
    double a() const;
    long b() const;
    int c();
    
    double get_a() const;
    long get_b() const;
    int get_c();
    
    int set_a();
    void set_a(double val);
    void set_a(long val);
    void set_a(int val) const;
    
private:
    int a_, b_, c_;
};

class c6 {
public:
    const c1 *c() const;
    const int &cc() const;
    
    void set_c(c1 *c);
    void set_cc(int &c);
    
private:
    c1 *c_;
    int &cc_;
};
