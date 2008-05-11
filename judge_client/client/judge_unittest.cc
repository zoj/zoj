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

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "trace.h"
#include "util.h"

DECLARE_ARG(string, compiler);

bool IsSupportedCompiler(const string& sourceFileType);

class IsSupportedCompilerTest : public TestFixture {
};

TEST_F(IsSupportedCompilerTest, NoneSupported) {
    ARG_compiler = "";
    ASSERT(!IsSupportedCompiler("g++"));
}

TEST_F(IsSupportedCompilerTest, OneSupported) {
    ARG_compiler = "g++";
    ASSERT(IsSupportedCompiler("g++"));
    ASSERT(!IsSupportedCompiler("gcc"));
}

TEST_F(IsSupportedCompilerTest, TwoSupported) {
    ARG_compiler = "g++,gcc";
    ASSERT(IsSupportedCompiler("g++"));
    ASSERT(IsSupportedCompiler("gcc"));
    ASSERT(!IsSupportedCompiler("fp"));
}

TEST_F(IsSupportedCompilerTest, ThreeSupported) {
    ARG_compiler = "g++,gcc,fp";
    ASSERT(IsSupportedCompiler("g++"));
    ASSERT(IsSupportedCompiler("gcc"));
    ASSERT(IsSupportedCompiler("fp"));
}



int ExecJudgeCommand(int sock,
                     const string& root,
                     int* problem_id,
                     int* revision);

class ExecJudgeCommandTest: public TestFixture {
  protected:
    virtual void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        problem_id_ = 100;
        revision_ = 101;
        checksum_ = CheckSum(CMD_JUDGE) +
                    CheckSum(problem_id_) +
                    CheckSum(revision_);
    }

    virtual void TearDown() {
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        system(("rm -rf " + root_).c_str());
    }

    void SendCommand() {
        problem_id_ = htonl(problem_id_);
        revision_ = htonl(revision_);
        checksum_ = htons(checksum_);
        Writen(fd_[0], &problem_id_, sizeof(problem_id_)); 
        Writen(fd_[0], &revision_, sizeof(revision_)); 
        Writen(fd_[0], &checksum_, sizeof(checksum_));
    }

    int fd_[2];
    char buf_[32];
    string root_;
    uint32_t problem_id_;
    uint32_t revision_;
    uint16_t checksum_;
};

TEST_F(ExecJudgeCommandTest, ReadCommandFailure) {
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int problem_id;
    int revision;
    ASSERT_EQUAL(-1, ExecJudgeCommand(fd_[1], root_, &problem_id, &revision));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(ExecJudgeCommandTest, InvalidCheckSum) {
    checksum_ = 0;
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int problem_id;
    int revision;
    ASSERT_EQUAL(-1, ExecJudgeCommand(fd_[1], root_, &problem_id, &revision));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecJudgeCommandTest, NoSuchProblem) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int problem_id;
    int revision;
    ASSERT_EQUAL(1, ExecJudgeCommand(fd_[1], root_, &problem_id, &revision));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
}

TEST_F(ExecJudgeCommandTest, Success) {
    ASSERT_EQUAL(0, mkdir("prob", 0700));
    ASSERT_EQUAL(0, mkdir("prob/100", 0700));
    ASSERT_EQUAL(0, mkdir("prob/100/101", 0700));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int problem_id;
    int revision;
    ASSERT_EQUAL(0, ExecJudgeCommand(fd_[1], root_, &problem_id, &revision));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(100, problem_id);
    ASSERT_EQUAL(101, revision);
}



int ExecCompileCommand(int sock,
                       const string& root,
                       const string& working_root,
                       int* compiler);

class ExecCompileCommandTest: public TestFixture {
  protected:
    virtual void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, mkdir((root_ + "/script").c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/../../script/compile.sh").c_str(),
                                (root_ + "/script/compile.sh").c_str()));
        fd_[0] = fd_[1] = temp_fd_ = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        submission_id_ = 1234;
        compiler_id_ = COMPILER_GPP;
        source_filename_ = TESTDIR + "/ac.cc";
        struct stat stat; 
        lstat(source_filename_.c_str(), &stat);
        source_file_size_ = stat.st_size;
        checksum_ = CheckSum(CMD_COMPILE) +
                    CheckSum(submission_id_) +
                    CheckSum(compiler_id_) +
                    CheckSum(source_file_size_);
        ARG_compiler = "g++";
    }

    virtual void TearDown() {
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        if (temp_fd_ >= 0) {
            close(temp_fd_);
        }
        system(("rm -rf " + root_).c_str());
    }

    void SendCommand() {
        submission_id_ = htonl(submission_id_);
        source_file_size_ = htonl(source_file_size_);
        checksum_ = htons(checksum_);
        Writen(fd_[0], &submission_id_, sizeof(submission_id_)); 
        Writen(fd_[0], &compiler_id_, sizeof(compiler_id_)); 
        Writen(fd_[0], &source_file_size_, sizeof(source_file_size_));
        Writen(fd_[0], &checksum_, sizeof(checksum_));
        if (!source_filename_.empty()) {
            temp_fd_ = open(source_filename_.c_str(), O_RDONLY);
            ASSERT(temp_fd_ != -1);
            int size = Readn(temp_fd_, buf_, sizeof(buf_));
            Writen(fd_[0], buf_, size);
        }
    }

    int fd_[2];
    int temp_fd_;
    char buf_[1024 * 16];
    string root_;
    uint32_t submission_id_;
    uint8_t compiler_id_;
    uint32_t source_file_size_;
    uint16_t checksum_;
    string source_filename_;
};

TEST_F(ExecCompileCommandTest, ReadCommandFailure) {
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(ExecCompileCommandTest, InvalidCheckSum) {
    checksum_ = 0;
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecCompileCommandTest, InvalidCompiler) {
    compiler_id_ = 255;
    checksum_ = CheckSum(CMD_COMPILE) +
                CheckSum(submission_id_) +
                CheckSum(compiler_id_) +
                CheckSum(source_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecCompileCommandTest, UnsupportedCompiler) {
    ARG_compiler = "";
    checksum_ = CheckSum(CMD_COMPILE) +
                CheckSum(submission_id_) +
                CheckSum(compiler_id_) +
                CheckSum(source_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecCompileCommandTest, SaveFailure) {
    source_filename_ = "";
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(2, Readn(fd_[0], buf_, 3));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[1]);
}

TEST_F(ExecCompileCommandTest, Success) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(0, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(COMPILER_GPP, compiler);
    ASSERT_EQUAL(3, Readn(fd_[0], buf_, 4));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(READY, (int)buf_[2]);
    ASSERT_EQUAL(0, access("p", F_OK));
}

TEST_F(ExecCompileCommandTest, CompilationError) {
    source_filename_ = TESTDIR + "/ce.cc";
    struct stat stat;
    lstat(source_filename_.c_str(), &stat);
    source_file_size_ = stat.st_size;
    checksum_ = CheckSum(CMD_COMPILE) +
                CheckSum(submission_id_) +
                CheckSum(compiler_id_) +
                CheckSum(source_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    int compiler;
    ASSERT_EQUAL(-1, ExecCompileCommand(fd_[1], root_, root_, &compiler));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(5, Readn(fd_[0], buf_, 5));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(COMPILATION_ERROR, (int)buf_[2]);
    int len = ntohs(*(uint16_t*)(buf_ + 3));
    ASSERT(len);
    ASSERT_EQUAL(len, Readn(fd_[0], buf_, len + 1));
}



class ExecTestCaseCommandTest: public TestFixture {
  protected:
    virtual void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, mkdir("prob", 0700));
        ASSERT_EQUAL(0, mkdir("prob/0", 0700));
        ASSERT_EQUAL(0, mkdir("prob/0/0", 0700));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/1.in").c_str(), "prob/0/0/1.in"));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/1.out").c_str(), "prob/0/0/1.out"));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/ac").c_str(), "p"));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        testcase_ = 1;
        time_limit_ = 10;
        memory_limit_ = 1000;
        output_limit_ = 1001;
        checksum_ = CheckSum(CMD_TESTCASE) +
                    CheckSum(testcase_) +
                    CheckSum(time_limit_) +
                    CheckSum(memory_limit_) +
                    CheckSum(output_limit_);
        InstallHandlers();
    }

    virtual void TearDown() {
        UninstallHandlers();
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        system(("rm -rf " + root_).c_str());
    }

    void SendCommand() {
        time_limit_ = htons(time_limit_);
        memory_limit_ = htonl(memory_limit_);
        output_limit_ = htons(output_limit_);
        checksum_ = htons(checksum_);
        Writen(fd_[0], &testcase_, sizeof(testcase_)); 
        Writen(fd_[0], &time_limit_, sizeof(time_limit_)); 
        Writen(fd_[0], &memory_limit_, sizeof(memory_limit_)); 
        Writen(fd_[0], &output_limit_, sizeof(output_limit_)); 
        Writen(fd_[0], &checksum_, sizeof(checksum_));
    }

    int fd_[2];
    char buf_[32];
    string root_;
    uint8_t testcase_;
    uint16_t time_limit_;
    uint32_t memory_limit_;
    uint16_t output_limit_;
    uint16_t checksum_;
};

int ExecTestCaseCommand(int sock,
                        const string& root,
                        int problem_id,
                        int revision,
                        int compiler,
                        int uid,
                        int gid);

TEST_F(ExecTestCaseCommandTest, ReadCommandFailure) {
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(ExecTestCaseCommandTest, InvalidCheckSum) {
    checksum_ = 0;
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecTestCaseCommandTest, InvalidProblemId) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecTestCaseCommand(fd_[1], root_, 1, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecTestCaseCommandTest, InvalidRevision) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecTestCaseCommand(fd_[1], root_, 0, 1, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecTestCaseCommandTest, InvalidTestCase) {
    testcase_ = 2;
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecTestCaseCommandTest, RunFailure) {
    ASSERT_EQUAL(0, unlink("p"));
    ASSERT_EQUAL(0, symlink((TESTDIR + "/fpe").c_str(), "p"));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(0, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(10, Readn(fd_[0], buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(FLOATING_POINT_ERROR, (int)buf_[9]);
}

TEST_F(ExecTestCaseCommandTest, CheckFailure) {
    ASSERT_EQUAL(0, unlink("p"));
    ASSERT_EQUAL(0, symlink((TESTDIR + "/wa").c_str(), "p"));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(0, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(11, Readn(fd_[0], buf_, 12));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(WRONG_ANSWER, (int)buf_[10]);
}

TEST_F(ExecTestCaseCommandTest, Success) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(0, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(11, Readn(fd_[0], buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
}

TEST_F(ExecTestCaseCommandTest, ExistingSymlinkInputOutput) {
    ASSERT_EQUAL(0, symlink(".", "input"));
    ASSERT_EQUAL(0, symlink(".", "output"));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(0, ExecTestCaseCommand(fd_[1], root_, 0, 0, COMPILER_GPP, 0, 0));
    ASSERT_EQUAL(11, Readn(fd_[0], buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
}



int CheckData(int sock, const string& root, const string& data_dir);

class CheckDataTest: public TestFixture {
  protected:
    virtual void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, mkdir((root_ + "/script").c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/../../script/compile.sh").c_str(),
                                (root_ + "/script/compile.sh").c_str()));
        ASSERT_EQUAL(0, mkdir("data", 0700));
        ASSERT_EQUAL(0, close(open("data/1.in", O_RDWR | O_CREAT)));
        ASSERT_EQUAL(0, close(open("data/2.in", O_RDWR | O_CREAT)));
        ASSERT_EQUAL(0, close(open("data/3.in", O_RDWR | O_CREAT)));
        ASSERT_EQUAL(0, close(open("data/1.out", O_RDWR | O_CREAT)));
        ASSERT_EQUAL(0, close(open("data/2.out", O_RDWR | O_CREAT)));
        ASSERT_EQUAL(0, close(open("data/3.out", O_RDWR | O_CREAT)));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, pipe(fd_));
        ARG_compiler = "g++";
    }

    virtual void TearDown() {
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        system(("rm -rf " + root_).c_str());
    }

    int fd_[2];
    char buf_[1024 * 16];
    string root_;
};

TEST_F(CheckDataTest, NonExistingDir) {
    ASSERT_EQUAL(0, system("rm -rf data"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[0]);
}

TEST_F(CheckDataTest, EmptyDir) {
    ASSERT_EQUAL(0, system("rm data/*"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, NonRegularFile) {
    ASSERT_EQUAL(0, mkdir("data/4.in", 0700));
    ASSERT_EQUAL(0, close(open("data/4.out", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, InvalidName1) {
    ASSERT_EQUAL(0, close(open("data/test", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, InvalidName2) {
    ASSERT_EQUAL(0, close(open("data/a.in", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(0, close(open("data/a.out", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, InvalidName3) {
    ASSERT_EQUAL(0, close(open("data/judge", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, InvalidName4) {
    ASSERT_EQUAL(0, close(open("data/.in", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(0, close(open("data/.out", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, UnsupportedJudgeSourceFileType) {
    ARG_compiler = "";
    ASSERT_EQUAL(0, link((TESTDIR + "/judge.cc").c_str(), "data/judge.cc"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, JudgeOnly) {
    ASSERT_EQUAL(0, system("rm data/*"));
    ASSERT_EQUAL(0, link((TESTDIR + "/judge.cc").c_str(), "data/judge.cc"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, UnmatchedTestcase1) {
    ASSERT_EQUAL(0, close(open("data/4.in", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, UnmatchedTestcase2) {
    ASSERT_EQUAL(0, close(open("data/4.out", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, UnmatchedTestcase3) {
    ASSERT_EQUAL(0, unlink("data/1.out"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(CheckDataTest, JudgeCompilationError) {
    ASSERT_EQUAL(0, link((TESTDIR + "/ce.cc").c_str(), "data/judge.cc"));
    ASSERT_EQUAL(-1, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(4, Readn(fd_[0], buf_, 4));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(COMPILATION_ERROR, (int)buf_[1]);
    int len = ntohs(*(uint16_t*)(buf_ + 2));
    ASSERT(len);
    ASSERT_EQUAL(len, Readn(fd_[0], buf_, len + 1));
}

TEST_F(CheckDataTest, SuccessNoJudge) {
    ASSERT_EQUAL(0, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(CheckDataTest, SuccessHasDataZip) {
    ASSERT_EQUAL(0, close(open("data/data.zip", O_RDWR | O_CREAT)));
    ASSERT_EQUAL(0, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(CheckDataTest, SuccessHasJudge) {
    ASSERT_EQUAL(0, link((TESTDIR + "/judge.cc").c_str(), "data/judge.cc"));
    ASSERT_EQUAL(0, CheckData(fd_[1], root_, root_ + "/data"));
    close(fd_[1]);
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
}


int ExecDataCommand(int sock, const string& root, unsigned int problem_id, unsigned int revision);

class ExecDataCommandTest: public TestFixture {
  protected:
    virtual void SetUp() {
        root_ = tmpnam(NULL);
        ASSERT_EQUAL(0, mkdir(root_.c_str(), 0700));
        ASSERT_EQUAL(0, chdir(root_.c_str()));
        ASSERT_EQUAL(0, mkdir("script", 0700));
        ASSERT_EQUAL(0, mkdir("prob", 0700));
        ASSERT_EQUAL(0, symlink((TESTDIR + "/../../script/compile.sh").c_str(),
                                (root_ + "/script/compile.sh").c_str()));
        fd_[0] = fd_[1] = -1;
        ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd_));
        ARG_compiler = "g++";
        data_filename_ = TESTDIR + "/data.zip";
        struct stat stat; 
        lstat(data_filename_.c_str(), &stat);
        data_file_size_ = stat.st_size;
        checksum_ = CheckSum(CMD_DATA) + CheckSum(data_file_size_);
    }

    virtual void TearDown() {
        if (fd_[0] >= 0) {
            close(fd_[0]);
        }
        if (fd_[1] >= 0) {
            close(fd_[1]);
        }
        if (temp_fd_ >= 0) {
            close(temp_fd_);
        }
        system(("rm -rf " + root_).c_str());
    }

    void SendCommand() {
        data_file_size_ = htonl(data_file_size_);
        checksum_ = htons(checksum_);
        Writen(fd_[0], &data_file_size_, sizeof(data_file_size_));
        Writen(fd_[0], &checksum_, sizeof(checksum_));
        if (!data_filename_.empty()) {
            temp_fd_ = open(data_filename_.c_str(), O_RDONLY);
            ASSERT(temp_fd_ != -1);
            int size = Readn(temp_fd_, buf_, sizeof(buf_));
            Writen(fd_[0], buf_, size);
        }
    }

    int fd_[2];
    char buf_[1024 * 16];
    int temp_fd_;
    string root_;
    uint32_t data_file_size_;
    uint16_t checksum_;
    string data_filename_;
};

TEST_F(ExecDataCommandTest, ReadCommandFailure) {
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(0, Readn(fd_[0], buf_, 1));
}

TEST_F(ExecDataCommandTest, InvalidCheckSum) {
    checksum_ = 0;
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, MaxFileSizePlusOne) {
    data_file_size_ = MAX_DATA_FILE_SIZE + 1;
    checksum_ = CheckSum(CMD_DATA) + CheckSum(data_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, CannotCreateProblemDir) {
    ASSERT_EQUAL(0, rmdir("prob"));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, CannotCreateTempDir) {
    ASSERT_EQUAL(0, rmdir("prob"));
    ASSERT_EQUAL(0, close(open("prob", O_RDWR | O_CREAT)));
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, CannotSaveFile) {
    data_filename_ = "";
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, CannotUnzip) {
    data_filename_ = TESTDIR + "/1.in";
    struct stat stat; 
    lstat(data_filename_.c_str(), &stat);
    data_file_size_ = stat.st_size;
    checksum_ = CheckSum(CMD_DATA) + CheckSum(data_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, CheckDataFailure) {
    data_filename_ = TESTDIR + "/data_empty.zip";
    struct stat stat; 
    lstat(data_filename_.c_str(), &stat);
    data_file_size_ = stat.st_size;
    checksum_ = CheckSum(CMD_DATA) + CheckSum(data_file_size_);
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(-1, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(1, Readn(fd_[0], buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
}

TEST_F(ExecDataCommandTest, Success) {
    SendCommand();
    ASSERT_EQUAL(0, shutdown(fd_[0], SHUT_WR));
    ASSERT_EQUAL(0, ExecDataCommand(fd_[1], root_, 0, 0));
    ASSERT_EQUAL(0, shutdown(fd_[1], SHUT_WR));
    ASSERT_EQUAL(2, Readn(fd_[0], buf_, 3));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(READY, (int)buf_[1]);
}



int JudgeMain(const string& root, const string& queue_address, int queue_port, int uid, int gid);

class JudgeMainTest: public TestFixture {
    protected:
        virtual void SetUp() {
            root_ = tmpnam(NULL);
            ASSERT_EQUAL(0, mkdir(root_.c_str(), 0755));
            ASSERT_EQUAL(0, chdir(root_.c_str()));
            ASSERT_EQUAL(0, mkdir("working", 0755));
            ASSERT_EQUAL(0, mkdir("script", 0750));
            ASSERT_EQUAL(0, mkdir("prob", 0750));
            ASSERT_EQUAL(0, symlink((TESTDIR + "/../../script/compile.sh").c_str(),
                                    (root_ + "/script/compile.sh").c_str()));
            server_sock_ = socket(PF_INET, SOCK_STREAM, 6);
            if (server_sock_ == -1) {
                FAIL(strerror(errno));
            }
            int option_value = 1;
            if (setsockopt(server_sock_,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           &option_value,
                           sizeof(option_value)) == -1) {
                FAIL(strerror(errno));
            }
            sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port = 0;
            if (bind(server_sock_,
                     (struct sockaddr*)&address,
                     sizeof(address)) == -1) {
                FAIL(strerror(errno));
            }
            if (listen(server_sock_, 32) == -1) {
                FAIL(strerror(errno));
            }
            socklen_t len = sizeof(address);
            if (getsockname(server_sock_,
                            (struct sockaddr*)&address,
                            &len) == -1) {
                FAIL(strerror(errno));
            }
            port_ = ntohs(address.sin_port);
            int flags = fcntl(server_sock_, F_GETFL, 0);
            ASSERT(flags >= 0);
            ASSERT_EQUAL(0, fcntl(server_sock_, F_SETFL, flags | O_NONBLOCK));
            client_sock_ = -1;
            address_ = "127.0.0.1";
            buf_size_ = 0;
        }

        virtual void TearDown() {
            UninstallHandlers();
            if (pid_ > 0) {
                kill(pid_, SIGKILL);
                waitpid(pid_, NULL, 0);
            }
            if (server_sock_ >= 0) {
                close(server_sock_);
            }
            if (client_sock_ >= 0) {
                close(client_sock_);
            }
            if (temp_fd_ >= 0) {
                close(temp_fd_);
            }
            system(("rm -rf " + root_).c_str());
        }

        int WaitForConnection(bool terminate = false) {
            pid_ = fork();
            if (pid_ < 0) {
                FAIL("Fail to fork");
            }
            if (pid_ == 0) {
                close(server_sock_);
                global::terminated = terminate;
                exit(JudgeMain(root_, address_, port_, 0, 0));
            }
            sockaddr_in address;
            size_t len = sizeof(address);
            for (int i = 0; i < 3; ++i) {
                client_sock_ = accept(server_sock_, (struct sockaddr*)&address, &len);
                if (client_sock_ >= 0) {
                    break;
                }
                if (errno != EWOULDBLOCK) {
                    FAIL(strerror(errno));
                }
                usleep(500);
            }
            if (client_sock_ < 0) {
                return -1;
            }
            close(server_sock_);
            server_sock_ = -1;
            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            ASSERT_EQUAL(0, setsockopt(client_sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));
            ASSERT_EQUAL(0, setsockopt(client_sock_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)));
            return 0;
        }

        int WaitForTermination() {
            int status;
            ASSERT_EQUAL(pid_, waitpid(pid_, &status, 0));
            pid_ = -1;
            ASSERT(WIFEXITED(status));
            return WEXITSTATUS(status);
        }

        void AppendCheckSum() {
            uint16_t checksum = 0;
            for (int i = 0; i < buf_size_; ++i) {
                checksum += (unsigned char)buf_[i];
            }
            checksum = htons(checksum);
            *(uint16_t*)(buf_ + buf_size_) = checksum;
            buf_size_ += sizeof(uint16_t);
        }

        void AppendUint8(uint8_t value) {
            *(uint8_t*)(buf_ + buf_size_) = value;
            buf_size_ += sizeof(uint8_t);
        }

        void AppendUint16(uint16_t value) {
            value = htons(value);
            *(uint16_t*)(buf_ + buf_size_) = value;
            buf_size_ += sizeof(uint16_t);
        }

        void AppendUint32(uint32_t value) {
            value = htonl(value);
            *(uint32_t*)(buf_ + buf_size_) = value;
            buf_size_ += sizeof(uint32_t);
        }

        void AppendFile(const string& filename, int size) {
            if (temp_fd_ >= 0) {
                close(temp_fd_);
            }
            temp_fd_ = open(filename.c_str(), O_RDONLY);
            ASSERT(temp_fd_ >= 0);
            ASSERT_EQUAL(size, Readn(temp_fd_, buf_ + buf_size_, size));
            buf_size_ += size;
        }

        void SendCommand() {
            Writen(client_sock_, buf_, buf_size_);
        }

        void SendJudgeCommand(int problem, int revision) {
            buf_size_ = 0;
            AppendUint8(CMD_JUDGE);
            AppendUint32(problem);
            AppendUint32(revision);
            AppendCheckSum();
            SendCommand();
        }

        void SendCompileCommand(const string& source_filename) {
            struct stat stat; 
            lstat(source_filename.c_str(), &stat);
            int source_file_size = stat.st_size;
            buf_size_ = 0;
            AppendUint8(CMD_COMPILE);
            AppendUint32(0);
            AppendUint8(COMPILER_GPP);
            AppendUint32(source_file_size);
            AppendCheckSum();
            AppendFile(source_filename, source_file_size);
            SendCommand();
        }

        void SendTestCaseCommand(int testcase, int time_limit, int memory_limit, int output_limit) {
            buf_size_ = 0;
            AppendUint8(CMD_TESTCASE);
            AppendUint8(testcase);
            AppendUint16(time_limit);
            AppendUint32(memory_limit);
            AppendUint16(output_limit);
            AppendCheckSum();
            SendCommand();
        }

        void SendDataCommand(const string& data_filename) {
            struct stat stat; 
            lstat(data_filename.c_str(), &stat);
            int data_file_size = stat.st_size;
            buf_size_ = 0;
            AppendUint8(CMD_DATA);
            AppendUint32(data_file_size);
            AppendCheckSum();
            AppendFile(data_filename, data_file_size);
            SendCommand();
        }

        int server_sock_;
        int client_sock_;
        int temp_fd_;
        int port_;
        pid_t pid_;
        string address_;
        string root_;
        char buf_[1024 * 16];
        int buf_size_;
};

TEST_F(JudgeMainTest, InvalidAddress) {
    address_ = "invalid";
    ASSERT_EQUAL(-1, WaitForConnection());
}

TEST_F(JudgeMainTest, CannotCreateWorkingDir) {
    ASSERT_EQUAL(0, rmdir("working"));
    ASSERT_EQUAL(0, WaitForConnection());
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INTERNAL_ERROR, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, SIGTERM) {
    ASSERT_EQUAL(0, WaitForConnection(true));
    ASSERT_EQUAL(0, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(0, WaitForTermination());
}

TEST_F(JudgeMainTest, ReadCommandFailure) {
    ASSERT_EQUAL(0, WaitForConnection());
    ASSERT_EQUAL(0, shutdown(client_sock_, SHUT_WR));
    ASSERT_EQUAL(0, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, InvalidCommand) {
    ASSERT_EQUAL(0, WaitForConnection());
    buf_[0] = 0;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, FirstCommandData) {
    ASSERT_EQUAL(0, WaitForConnection());
    buf_[0] = CMD_DATA;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, FirstCommandTestCase) {
    ASSERT_EQUAL(0, WaitForConnection());
    buf_[0] = CMD_TESTCASE;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, MultipleData) {
    ASSERT_EQUAL(0, WaitForConnection());
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
    SendDataCommand(TESTDIR + "/data.zip");
    ASSERT_EQUAL(2, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(READY, (int)buf_[1]);
    buf_[0] = CMD_DATA;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, TestBeforeDataSynchirnized) {
    ASSERT_EQUAL(0, WaitForConnection());
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
    SendCompileCommand(TESTDIR + "/ac.cc");
    ASSERT_EQUAL(3, Readn(client_sock_, buf_, 3));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(READY, (int)buf_[2]);
    buf_[0] = CMD_TESTCASE;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, TestBeforeCompiled) {
    ASSERT_EQUAL(0, WaitForConnection());
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
    SendDataCommand(TESTDIR + "/data.zip");
    ASSERT_EQUAL(2, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(READY, (int)buf_[1]);
    buf_[0] = CMD_TESTCASE;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, UnnecessaryData) {
    ASSERT_EQUAL(0, WaitForConnection());
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
    SendDataCommand(TESTDIR + "/data.zip");
    ASSERT_EQUAL(2, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(READY, (int)buf_[1]);
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    buf_[0] = CMD_DATA;
    Writen(client_sock_, buf_, 1);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(INVALID_INPUT, (int)buf_[0]);
    ASSERT_EQUAL(1, WaitForTermination());
}

TEST_F(JudgeMainTest, Success) {
    ASSERT_EQUAL(0, WaitForConnection());
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(NO_SUCH_PROBLEM, (int)buf_[0]);
    SendDataCommand(TESTDIR + "/data.zip");
    ASSERT_EQUAL(2, Readn(client_sock_, buf_, 2));
    ASSERT_EQUAL(COMPILING, (int)buf_[0]);
    ASSERT_EQUAL(READY, (int)buf_[1]);
    SendCompileCommand(TESTDIR + "/ac.cc");
    ASSERT_EQUAL(3, Readn(client_sock_, buf_, 3));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(READY, (int)buf_[2]);
    SendTestCaseCommand(1, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
    SendTestCaseCommand(3, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
    SendTestCaseCommand(2, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
    SendTestCaseCommand(1, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(ACCEPTED, (int)buf_[10]);
    SendCompileCommand(TESTDIR + "/wa.cc");
    ASSERT_EQUAL(3, Readn(client_sock_, buf_, 3));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(READY, (int)buf_[2]);
    SendTestCaseCommand(3, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(WRONG_ANSWER, (int)buf_[10]);
    SendJudgeCommand(0, 0);
    ASSERT_EQUAL(1, Readn(client_sock_, buf_, 1));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    SendCompileCommand(TESTDIR + "/wa.cc");
    ASSERT_EQUAL(3, Readn(client_sock_, buf_, 3));
    ASSERT_EQUAL(READY, (int)buf_[0]);
    ASSERT_EQUAL(COMPILING, (int)buf_[1]);
    ASSERT_EQUAL(READY, (int)buf_[2]);
    SendTestCaseCommand(3, 10, 1000, 1000);
    ASSERT_EQUAL(11, Readn(client_sock_, buf_, 11));
    ASSERT_EQUAL(RUNNING, (int)buf_[0]);
    ASSERT_EQUAL(JUDGING, (int)buf_[9]);
    ASSERT_EQUAL(WRONG_ANSWER, (int)buf_[10]);
}
