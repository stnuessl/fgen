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

#include <FGenConfiguration.hpp>

void FGenConfiguration::setImplementAccessors(bool Value)
{
    ImplementAccessors_ = Value;
}

bool FGenConfiguration::implementAccessors() const
{
    return ImplementAccessors_;
}

void FGenConfiguration::setImplementReturnValues(bool Value)
{
    ImplementReturnValues_ = Value;
}

bool FGenConfiguration::implementReturnValues() const
{
    return ImplementReturnValues_;
}

void FGenConfiguration::setOutputFile(std::string File)
{
    OutputFile_ = std::move(File);
}

const std::string &FGenConfiguration::outputFile() const
{
    return OutputFile_;
}

std::unordered_set<std::string> &FGenConfiguration::targets()
{
    return Targets_;
}

const std::unordered_set<std::string> &FGenConfiguration::targets() const
{
    return Targets_;
}
