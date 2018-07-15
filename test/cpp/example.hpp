/*
 * Copyright (C) 2018  Steffen Nüssle
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
