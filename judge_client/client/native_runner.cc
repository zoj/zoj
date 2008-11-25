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

#include "native_runner.h"

#include <string>

#include <sys/wait.h>
#include <unistd.h>

#include "common_io.h"
#include "logging.h"
#include "protocol.h"
#include "trace.h"
#include "util.h"

int NativeRunner::Monitor(int sock, pid_t pid, int time_limit, int memory_limit, TraceCallback* callback) {
    int result = -1;
    int time_consumption = 0;
    int memory_consumption = 0;
    time_limit *= 1000;
    while (result < 0 && !callback->HasExited()) {
        struct timespec request, remain;
        request.tv_sec = 1;
        request.tv_nsec = 0;
        while (result < 0 && !callback->HasExited() && nanosleep(&request, &remain) < 0) {
            if (errno != EINTR) {
                LOG(SYSCALL_ERROR)<<"Fail to sleep";
                result = INTERNAL_ERROR;
            }
            request = remain;
        }
        int ts;
        int ms;
        if (result < 0 && !callback->HasExited()) {
            ts = ReadTimeConsumption(pid);
            ms = ReadMemoryConsumption(pid);
            if (ts > time_consumption) {
                time_consumption = ts;
            }
            if (ms > memory_consumption) {
                memory_consumption = ms;
            }
            if (time_consumption > time_limit) {
                result = TIME_LIMIT_EXCEEDED;
            }
            if (result == TIME_LIMIT_EXCEEDED) {
                time_consumption = time_limit + 1;
            }
            if (memory_consumption > memory_limit) {
                result = MEMORY_LIMIT_EXCEEDED;
            }
            if (result == MEMORY_LIMIT_EXCEEDED) {
                memory_consumption = memory_limit + 1;
            }
            if (SendRunningMessage(sock,
                                   time_consumption,
                                   memory_consumption) == -1) {
                result = INTERNAL_ERROR;
            }
        }
    }
    if (result >= 0) {
    } else {
        int status;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                LOG(SYSCALL_ERROR);
                return INTERNAL_ERROR;
            }
        }
        if (callback->GetResult() == 0) {
            time_consumption = callback->GetTimeConsumption();
            memory_consumption = callback->GetMemoryConsumption();
        }
        callback->ProcessResult(status);
        result = callback->GetResult();
        if (memory_consumption > memory_limit) {
            result = MEMORY_LIMIT_EXCEEDED;
        }
        if (result == TIME_LIMIT_EXCEEDED) {
            time_consumption = time_limit + 1;
        }
        if (result == MEMORY_LIMIT_EXCEEDED) {
            memory_consumption = memory_limit + 1;
        }
        if (SendRunningMessage(sock, time_consumption, memory_consumption) == -1) {
            result = INTERNAL_ERROR;
        }
    }
    return result;
}

int NativeRunner::Run(int sock, int time_limit, int memory_limit, int output_limit, int uid, int gid) {
    LOG(INFO)<<"Running";
    const char* commands[] = {"p", "p", NULL};
    StartupInfo info;
    info.stdin_filename = "input";
    info.stdout_filename = "p.out";
    info.uid = uid;
    info.gid = gid;
    info.time_limit = time_limit;
    info.memory_limit = memory_limit;
    info.output_limit = output_limit;
    info.stack_limit = 8192; // Always set stack limit to 8M
    info.proc_limit = 1;
    info.file_limit = 5;
    info.trace = 1;
    TraceCallback callback;
    pid_t pid = CreateProcess(commands, info);
    if (pid == -1) {
        LOG(ERROR)<<"Fail to execute the program";
        WriteUint32(sock, INTERNAL_ERROR);
        return -1;
    }
    int result = Monitor(sock, pid, time_limit, memory_limit, &callback);
    if (result) {
        LogResult(result);
        WriteUint32(sock, result);
        if (result == INTERNAL_ERROR) {
            return -1;
        } else {
            return 1;
        }
    }
    return 0;
}
