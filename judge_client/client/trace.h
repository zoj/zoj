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

#ifndef __TRACE_H
#define __TRACE_H

#include <string>
#include <vector>

#include "judge_result.h"

class TraceCallback {
    public:
        TraceCallback():
            result_(-1),
            timeConsumption_(0),
            memoryConsumption_(0) {
            TraceCallback::instance_ = this;
        }

        virtual ~TraceCallback() {
            TraceCallback::instance_ = NULL;
        }

        virtual int onClone() {
            return 0;
        }

        virtual int onExecve();

        virtual void onMemoryLimitExceeded();

        virtual void onExit(pid_t pid);

        virtual void onSIGCHLD(pid_t pid);

        virtual void onError();

        int getResult() const {
            return result_;
        }

        void setResult(int result) {
            result_ = result;
        }

        double getTimeConsumption() const {
            return timeConsumption_;
        }

        int getMemoryConsumption() const {
            return memoryConsumption_;
        }

        int hasExited() const {
            return result_ >= 0 && result_ != RUNNING;
        }

        static TraceCallback* getInstance() {
            return TraceCallback::instance_;
        }

    protected:
        int result_;
        double timeConsumption_;
        int memoryConsumption_;

    private:
        static TraceCallback* instance_;
};

class ExecutiveCallback: public TraceCallback {
    public:
        virtual void onExit(pid_t pid);

};

void installHandlers();

#endif
