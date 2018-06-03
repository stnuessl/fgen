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

namespace n {
    
class default_constructor {
public:
    default_constructor() = default;
};

}

class deleted_constructor {
public:
    deleted_constructor() = delete;
};

bool &ref_bool();

short &ref_short();

int &ref_int();

long &ref_long();

float &ref_float();

double &ref_double();

n::default_constructor &ref_default_constructor();

deleted_constructor &ref_deleted_constructor();

bool get_bool();

short get_short();

int get_int();

long get_long();

float get_float();

double get_double();

void *get_ptr();

n::default_constructor get_fdefault_constructor();

deleted_constructor get_fdeleted_constructor();
