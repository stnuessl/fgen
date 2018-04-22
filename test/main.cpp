/*
 * Copyright (C) 2017  Steffen Nüssle
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

#include <memory>
#include <cstddef>

// template <typename T, typename... Args>
// std::unique_ptr<T> make_unique(Args &&... args);

namespace ns {

template <typename T, std::size_t size = 16>
class my_array {
public:
    typedef std::size_t size_type;
    
    my_array();
    my_array(const my_array &other);
    my_array(my_array &&other);
    ~my_array() = default;
    
    my_array &operator=(const my_array &other);
    my_array &operator=(my_array &&other);

    template <typename U>
    U &getAs(size_type i);
    
    template <typename U>
    const U &getAs(size_type i) const;
    
    bool empty() const;
private:
    template <typename V> 
    class my_array_allocator {
        typedef std::size_t size_type;
        
        my_array_allocator() = default;
        
        template <typename W>
        my_array_allocator(const my_array_allocator<W> &other);
        
        V *allocate(size_type n);
        void deallocate(V *ptr, size_type n);
    };
};

}

int main(int argc, char **argv)
{
    class ns::my_array<int> array;
    
    return 0;
}
