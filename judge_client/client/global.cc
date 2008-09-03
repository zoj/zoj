/*
 * Copyright 2007 Xu, Chuan <xuchuan@gmail.com>
 *
 * This file is part of ZOJ.
 *
 * ZOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ZOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ZOJ. if not, see <http://www.gnu.org/licenses/>.
 */

#include "global.h"

namespace global {
    int terminated;
    int socket_closed;
    CompilerInfo COMPILER_LIST[4] = {
        {1, "gcc", "c"},
        {2, "g++", "cc"},
        {3, "fp", "pas"},
        {4, "javac", "java"}
    };
    const int COMPILER_NUM = sizeof(global::COMPILER_LIST) / sizeof(global::COMPILER_LIST[0]);
}
