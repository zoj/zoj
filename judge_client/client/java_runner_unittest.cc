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


#include "unittest.h"
#include "java_runner.h"

#include "protocol.h"
#include "strutil.h"
#include "test_util-inl.h"
#include "trace.h"

DEFINE_ARG(string, root, "");

class JavaRunnerTest : public TestFixture {
  protected:
    virtual void SetUp() {
        runner_ = new JavaRunner();
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, symlink((CURRENT_WORKING_DIR + "/JavaSandbox.jar").c_str(), "JavaSandbox.jar"));
        ASSERT_EQUAL(0, symlink((CURRENT_WORKING_DIR + "/libsandbox.so").c_str(), "libsandbox.so"));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/1.in").c_str(), "input"));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        time_limit_ = 10;
        memory_limit_ = output_limit_ = 1000;
        ARG_root = root_;
        InstallHandlers();
    }

    virtual void TearDown() {
        if (runner_) {
            delete runner_;
        }
        UninstallHandlers();
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        system(("rm -rf " + root_).c_str());
    }

    int Run() {
        ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
        int ret = runner_->Run(fd_[1], time_limit_, memory_limit_, output_limit_, 0, 0);
        ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
        return ret;
    }

    JavaRunner* runner_;
    string root_;
    int fd_[2];
    int time_limit_;
    int memory_limit_;
    int output_limit_;
};

TEST_F(JavaRunnerTest, Success) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ac.class").c_str(), "P.class"));

    ASSERT_EQUAL(0, Run());

    ASSERT(!system(StringPrintf("diff p.out %s/1.out", TESTDIR.c_str()).c_str()));
    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (reply == -1) {
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, SuccessMultipleClasses) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ac_multiple_classes.class").c_str(), "P.class"));
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ac_multiple_classes$1.class").c_str(), "P$1.class"));

    ASSERT_EQUAL(0, Run());

    ASSERT(!system(StringPrintf("diff p.out %s/1.out", TESTDIR.c_str()).c_str()));
    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (reply == -1) {
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, TimeLimitExceeded) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/tle.class").c_str(), "P.class"));
    time_limit_ = 1;

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(TIME_LIMIT_EXCEEDED, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, TimeLimitExceededStaticInitializer) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/tle_static_initializer.class").c_str(), "P.class"));
    time_limit_ = 1;

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(TIME_LIMIT_EXCEEDED, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, TimeLimitExceededMultipleClassesStaticInitializer) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/tle_multiple_classes_static_initializer.class").c_str(), "P.class"));
    ASSERT_EQUAL(0, symlink((TESTDIR + "/tle_multiple_classes_static_initializer_T.class").c_str(), "T.class"));
    time_limit_ = 1;

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(TIME_LIMIT_EXCEEDED, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, MemoryLimitExceeded) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/mle.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(MEMORY_LIMIT_EXCEEDED, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, OutputLimitExceeded) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ole.class").c_str(), "P.class"));
    output_limit_ = 1;

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(OUTPUT_LIMIT_EXCEEDED, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, RuntimeError) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/rte.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(RUNTIME_ERROR, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, RuntimeErrorInvalidMain) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/rte_invalid_main.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(RUNTIME_ERROR, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, RuntimeErrorNonStaticMain) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/rte_nonstatic_main.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(RUNTIME_ERROR, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, RuntimeErrorHasPackage) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/rte_has_package.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(RUNTIME_ERROR, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

TEST_F(JavaRunnerTest, RuntimeErrorWait) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/rte_wait.class").c_str(), "P.class"));

    ASSERT_EQUAL(1, Run());

    for (;;) {
        int reply = TryReadUint32(fd_[0]);
        int time = TryReadUint32(fd_[0]);
        int memory = TryReadUint32(fd_[0]);
        if (time < 0) {
            ASSERT_EQUAL(RUNTIME_ERROR, reply);
            break;
        }
        ASSERT_EQUAL(RUNNING, reply);
        ASSERT(time >= 0);
        ASSERT(memory >= 0);
    }
}

