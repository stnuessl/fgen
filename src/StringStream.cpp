/*
 * Copyright (C) 2019  Steffen Nüssle
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

#include <StringStream.hpp>

StringStream::StringStream(bool Unbuffered, size_t BufferSize)
    : llvm::raw_ostream(Unbuffered), Buffer_()
{
    Buffer_.reserve(BufferSize);
}

uint64_t StringStream::current_pos() const
{
    return Buffer_.size();
}

void StringStream::write_impl(const char *Ptr, size_t Size)
{
    Buffer_.append(Ptr, Size);
}

void StringStream::clear()
{
    Buffer_.clear();
}

const std::string &StringStream::str() const
{
    return Buffer_;
}
