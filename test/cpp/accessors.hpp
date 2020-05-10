/*
 * Copyright (C) 2019  Steffen Nüssle
 * fgen - function generator
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

template <typename T>
class sptr {
public:
    void set_ptr(T *ptr);

    T *ptr() const;
private:
    T *ptr_;
};

template <typename T>
class a {
public:
    void set(T &&x);

    T get() const; 
private:
    T val_;
};

int main()
{
    a<int> a;

    int i = a.get();
}
