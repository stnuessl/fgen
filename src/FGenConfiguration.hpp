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

#ifndef FGEN_FGENCONFIGURATION_HPP_
#define FGEN_FGENCONFIGURATION_HPP_

#include <string>
#include <unordered_set>

class FGenConfiguration {
public:
    FGenConfiguration() = default;

    void setAllowMove(bool Value);
    bool allowMove() const;

    void setImplementAccessors(bool Value);
    bool implementAccessors() const;

    void setImplementStubs(bool Value);
    bool implementStubs() const;

    void setSkipNamespaces(bool Value);
    bool skipNamespaces() const;

    void setOutputFile(std::string File);
    const std::string &outputFile() const;

    std::unordered_set<std::string> &targets();
    const std::unordered_set<std::string> &targets() const;

private:
    bool AllowMove_;
    bool ImplementAccessors_;
    bool ImplementStubs_;
    bool SkipNamespaces_;

    std::string OutputFile_;
    std::unordered_set<std::string> Targets_;
};

#endif /* FGEN_FGENCONFIGURATION_HPP_ */
