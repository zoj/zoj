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
#include "compiler.h"

#include <sys/socket.h>
#include <sys/stat.h>

#include "common_io.h"
#include "environment.h"
#include "logging.h"
#include "protocol.h"

class CompilerTest: public TestFixture {
  protected:
    void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, mkdir("script", 0700));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/../../script/compile.sh").c_str(), "script/compile.sh"));
        ASSERT_EQUAL(0, symlink((CURRENT_WORKING_DIR + "/CustomJavaCompiler.class").c_str(),
                                "CustomJavaCompiler.class"));
        ASSERT_EQUAL(0, symlink((CURRENT_WORKING_DIR + "/CustomJavaCompiler$CustomJavaFileManager.class").c_str(),
                                "CustomJavaCompiler$CustomJavaFileManager.class"));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        Environment::GetInstance();
        Environment::instance_->root_ = root_;
        compiler_id_ = 2;
        compiler_name_ = "g++";
        source_filename_ = "p.cc";
    }

    void TearDown() {
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        if (system(("rm -rf " + root_).c_str())) {
        }
    }

    int Run() {
        Compiler compiler(compiler_id_, compiler_name_, source_filename_);
        int ret = compiler.Compile(fd_[1]);
        ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
        return ret;
    }

    int fd_[2];
    char buf_[1024 * 16];
    string root_;
    int compiler_id_;
    string compiler_name_;
    string source_filename_;
    Compiler* compiler_;
};

TEST_F(CompilerTest, Success) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ac.cc").c_str(), "p.cc"));

    ASSERT_EQUAL(0, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, (int) read(fd_[0], buf_, 1));
}

TEST_F(CompilerTest, Failure) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce.cc").c_str(), "p.cc"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT((int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
}

TEST_F(CompilerTest, TooLongErrorMessage) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce_long_error.cc").c_str(), "p.cc"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(4096, (int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
}

class CCompilerTest : public CompilerTest {
  protected:
    virtual void SetUp() {
        CompilerTest::SetUp();
        compiler_id_ = 1;
        compiler_name_ = "gcc";
        source_filename_ = "p.c";
    }
};

TEST_F(CCompilerTest, HugeOutput) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce_huge_output.c").c_str(), "p.cc"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT((int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
}

TEST_F(CCompilerTest, LibMath) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/math.c").c_str(), "p.c"));

    ASSERT_EQUAL(0, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, (int) read(fd_[0], buf_, 1));
}

class JavaCompilerTest : public CompilerTest {
  protected:
    virtual void SetUp() {
        CompilerTest::SetUp();
        compiler_id_ = 4;
        compiler_name_ = "javac";
        source_filename_ = "Main.java";
    }
};

TEST_F(JavaCompilerTest, Success) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ac.java").c_str(), "Main.java"));

    ASSERT_EQUAL(0, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, (int) read(fd_[0], buf_, 1));
}

TEST_F(JavaCompilerTest, Failure) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce.java").c_str(), "Main.java"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT((int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
    buf_[t] = 0;
    LOG(INFO)<<buf_;
}

TEST_F(JavaCompilerTest, FailureInvalidClassName) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce_invalid_class_name.java").c_str(), "Main.java"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT((int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
    buf_[t] = 0;
    LOG(INFO)<<buf_;
}

TEST_F(JavaCompilerTest, FailureTooManyClasses) {
    ASSERT_EQUAL(0, symlink((TESTDIR + "/ce_too_many_classes.java").c_str(), "Main.java"));

    ASSERT_EQUAL(1, Run());

    uint32_t t;
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILING, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT_EQUAL(COMPILATION_ERROR, (int)t);
    ASSERT_EQUAL(0, ReadUint32(fd_[0], &t));
    ASSERT((int)t);
    ASSERT_EQUAL((ssize_t)t, read(fd_[0], buf_, t + 1));
    buf_[t] = 0;
    LOG(INFO)<<buf_;
}

// TODO add a unittest for invalid chars
