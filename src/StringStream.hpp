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

#ifndef STRING_STREAM_HPP_
#define STRING_STREAM_HPP_

#include <llvm/Support/raw_ostream.h>

class StringStream : public llvm::raw_ostream {
public:
    explicit StringStream(bool Unbuffered = false, size_t BufferSize = BUFSIZ);

    virtual uint64_t current_pos() const override;
    virtual void write_impl(const char *Ptr, size_t Size) override;

    void clear();

    const std::string &str() const;

private:
    std::string Buffer_;
};

#endif /* STRING_STREAM_HPP_ */
