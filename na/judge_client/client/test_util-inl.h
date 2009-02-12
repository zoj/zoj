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
#ifndef __TEST_UTIL_INL_H
#define __TEST_UTIL_INL_H

#include "common_io.h"
#include "util.h"

static inline int TryReadUint32(int fd) {
    uint32_t t;
    if (ReadUint32(fd, &t) < 0) {
        return -1;
    }
    return (int)t;
}

static inline int ReadUint32(int fd) {
    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd, &t));
    return (int)t;
}

static inline int ReadLastUint32(int fd) {
    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd, &t));
    char ch;
    ASSERT_EQUAL(0, read(fd, &ch, 1));
    return (int)t;
}

extern int ReadUntilNotRunning(int fd) {
    for (;;) {
        int reply = TryReadUint32(fd);
        if (reply != RUNNING) {
            return reply;
        }
        int time = ReadUint32(fd);
        int memory = ReadUint32(fd);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

extern int ReadUntilNotJudging(int fd) {
    for (;;) {
        int reply = ReadUint32(fd);
        if (reply != JUDGING) {
            return reply;
        }
    }
}

extern inline int Eof(int fd) {
    char ch;
    return !read(fd, &ch, 1);
}

#endif
