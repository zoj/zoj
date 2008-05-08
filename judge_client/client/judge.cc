/*
 * Copyright 2007 Xu, Chuan <xuchuan@gmail.com>
 *
 * This file is part of ZOJ.
 *
 * ZOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either revision 3 of the License, or
 * (at your option) any later revision.
 *
 * ZOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ZOJ. if not, see <http://www.gnu.org/licenses/>.
 */
#include <dirent.h>
#include <sys/types.h>

#include "check.h"
#include "global.h"
#include "compile.h"
#include "logging.h"
#include "run.h"
#include "trace.h"
#include "util.h"

DEFINE_ARG(string, compiler, "All compilers supported by this client");

// Returns true if the specified file type is supported by the server
bool IsSupportedCompiler(const string& compiler) {
    vector<string> supportedCompilers;
    SplitString(ARG_compiler, ',', &supportedCompilers);
    return find(supportedCompilers.begin(),
                supportedCompilers.end(),
                compiler) != supportedCompilers.end();
}

int ExecJudgeCommand(int sock,
                     const string& root,
                     int* problem_id,
                     int* revision) {
    uint32_t _problem_id;
    uint32_t _revision;
    uint16_t checksum;
    if (ReadUint32(sock, &_problem_id) == -1 ||
        ReadUint32(sock, &_revision) == -1 ||
        ReadUint16(sock, &checksum)) {
        return -1;
    }
    *problem_id = _problem_id;
    *revision = _revision;
    LOG(INFO)<<StringPrintf("Problem:%u Revision:%u",
                            (unsigned int)*problem_id,
                            (unsigned int)*revision);
    if (CheckSum(CMD_JUDGE) +
        CheckSum(_problem_id) + 
        CheckSum(_revision) != checksum) {
        LOG(ERROR)<<"Invalid checksum "<<checksum;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    string problem_dir = StringPrintf("%s/prob/%u/%u",
                                      root.c_str(),
                                      *problem_id,
                                      *revision);
    if (access(problem_dir.c_str(), F_OK) == 0) {
        SendReply(sock, READY);
        return 0;
    } else if (errno != ENOENT) {
        LOG(SYSCALL_ERROR)<<"Fail to access "<<problem_dir;
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    } else {
        LOG(ERROR)<<"No such problem";
        SendReply(sock, NO_SUCH_PROBLEM);
        return 1;
    }
}

int ExecCompileCommand(int sock,
                       const string& root,
                       const string& working_root,
                       int* compiler) {
    uint32_t submission_id;
    uint8_t compiler_id;
    uint32_t source_file_size;
    uint16_t checksum;
    if (ReadUint32(sock, &submission_id) == -1 ||
        ReadUint8(sock, &compiler_id) == -1 ||
        ReadUint32(sock, &source_file_size) == -1 ||
        ReadUint16(sock, &checksum) == -1) {
        return -1;
    }
    LOG(INFO)<<StringPrintf("Submission:%u Compiler:%u",
                            (unsigned int)submission_id,
                            (unsigned int)compiler_id);
    if (CheckSum(CMD_COMPILE) +
        CheckSum(submission_id) +
        CheckSum(compiler_id) +
        CheckSum(source_file_size) != checksum) {
        LOG(ERROR)<<"Invalid checksum "<<checksum;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    *compiler = -1;
    const int COMPILER_NUM =
        sizeof(global::COMPILER_LIST) / sizeof(global::COMPILER_LIST[0]);
    for (int i = 0; i < COMPILER_NUM; ++i) {
        if (compiler_id == global::COMPILER_LIST[i].id) {
            *compiler = compiler_id;
            break;
        }
    }
    if (*compiler < 0 || !IsSupportedCompiler(global::COMPILER_LIST[compiler_id].compiler)) {
        LOG(ERROR)<<"Invalid compiler "<<(int)compiler_id;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    LOG(INFO)<<"Compiler:"<<global::COMPILER_LIST[*compiler].compiler;
    SendReply(sock, READY);
    string source_filename =
        StringPrintf("p.%s",
                     global::COMPILER_LIST[*compiler].source_file_type);
    LOG(INFO)<<"Saving source file "<<source_filename<<". Length:"<<source_file_size;
    if (SaveFile(sock, source_filename.c_str(), source_file_size) == -1) {
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    }

    if (DoCompile(sock, root, *compiler, source_filename) == -1) {
        return -1;
    }
    SendReply(sock, READY);
    return 0;
}

int ExecTestCaseCommand(int sock,
                        const string& root,
                        int problem_id,
                        int revision,
                        int compiler,
                        int uid,
                        int gid) {
    uint8_t testcase;
    uint16_t time_limit;
    uint32_t memory_limit;
    uint16_t output_limit;
    uint16_t checksum;
    if (ReadUint8(sock, &testcase) == -1 ||
        ReadUint16(sock, &time_limit) == -1 ||
        ReadUint32(sock, &memory_limit) == -1 ||
        ReadUint16(sock, &output_limit) == -1 ||
        ReadUint16(sock, &checksum)) {
        return -1;
    }
    LOG(INFO)<<StringPrintf("Testcase:%u TL:%u ML:%u OL:%u",
                            (unsigned int)testcase,
                            (unsigned int)time_limit,
                            (unsigned int)memory_limit,
                            (unsigned int)output_limit);
    if (CheckSum(CMD_TESTCASE) +
        CheckSum(testcase) + 
        CheckSum(time_limit) + 
        CheckSum(memory_limit) + 
        CheckSum(output_limit) != checksum) {
        LOG(ERROR)<<"Invalid checksum "<<checksum;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    string problem_dir = StringPrintf("%s/prob/%u/%u", root.c_str(), problem_id, revision);
    if (access(problem_dir.c_str(), F_OK) == -1) {
        LOG(SYSCALL_ERROR)<<"Fail to access "<<problem_dir;
        if (errno != ENOENT) {
            SendReply(sock, INTERNAL_ERROR);
        } else {
            SendReply(sock, INVALID_INPUT);
        }
        return -1;
    }
    string input_filename = StringPrintf("%s/%u.in",
                                         problem_dir.c_str(),
                                         (unsigned int)testcase);
    string output_filename = StringPrintf("%s/%u.out",
                                          problem_dir.c_str(),
                                          (unsigned int)testcase);
    string special_judge_filename = problem_dir + "/judge";
    if (access(input_filename.c_str(), F_OK) == -1) {
        LOG(ERROR)<<"Invalid test case "<<testcase;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    if (unlink("input") < 0) {
        if (errno != ENOENT) {
            LOG(SYSCALL_ERROR)<<"Fail to remove symlink input";
            SendReply(sock, INTERNAL_ERROR);
            return -1;
        }
    }
    if (unlink("output") < 0) {
        if (errno != ENOENT) {
            LOG(SYSCALL_ERROR)<<"Fail to remove symlink output";
            SendReply(sock, INTERNAL_ERROR);
            return -1;
        }
    }
    if (symlink(input_filename.c_str(), "input") == -1) {
        LOG(SYSCALL_ERROR)<<"Fail to create symlink to "<<input_filename;
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    }
    int result = DoRun(sock,
                       compiler,
                       time_limit,
                       memory_limit,
                       output_limit,
                       uid,
                       gid);
    if (result) {
        return result == -1 ? -1 : 0;
    }
    if (symlink(output_filename.c_str(), "output") == -1) {
        LOG(SYSCALL_ERROR)<<"Fail to create symlink to "<<output_filename;
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    }
    return DoCheck(sock, uid, special_judge_filename);
}

int CheckData(int sock, const string& root, const string& data_dir) {
    DIR* dir = opendir(data_dir.c_str());
    if (dir == NULL) {
        LOG(SYSCALL_ERROR)<<"Can not open dir "<<data_dir;
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    }
    int ret = 0;
    vector<int> in, out;
    string judge;
    int compiler = -1;
    for (;;) {
        struct dirent* entry = readdir(dir);
        if (entry == NULL) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, "data.zip") == 0) {
            continue;
        }
        struct stat status;
        lstat(StringPrintf("%s/%s", data_dir.c_str(), entry->d_name).c_str(),
              &status);
        if (!S_ISREG(status.st_mode)) {
            LOG(ERROR)<<"Invalid file "<<entry->d_name;
            ret = -1;
            break;
        }
        int index;
        if (StringEndsWith(entry->d_name, ".in")) {
            if (sscanf(entry->d_name, "%d.in", &index) != 1) {
                LOG(ERROR)<<"Invalid filename "<<entry->d_name;
                ret = -1;
                break;
            }
            in.push_back(index);
        } else if (StringEndsWith(entry->d_name, ".out")) {
            if (sscanf(entry->d_name, "%d.out", &index) != 1) {
                LOG(ERROR)<<"Invalid filename "<<entry->d_name;
                ret = -1;
                break;
            }
            out.push_back(index);
        } else if (StringStartsWith(entry->d_name, "judge.")) {
            string source_file_type = entry->d_name + 6;
            for (int i = 0; i < sizeof(global::COMPILER_LIST) / sizeof(global::COMPILER_LIST[0]); ++i) {
                if (global::COMPILER_LIST[i].source_file_type == source_file_type) {
                    if (IsSupportedCompiler(global::COMPILER_LIST[i].compiler)) {
                        compiler = i;
                    }
                    break;
                }
            }
            if (compiler < 0) {
                LOG(ERROR)<<"Unsupported judge source file type "<<source_file_type;
                ret = -1;
                break;
            }
            judge = entry->d_name;
        } else {
            LOG(ERROR)<<"Invalid filename "<<entry->d_name;
            ret = -1;
            break;
        }
    }
    closedir(dir);
    if (ret == 0) {
        if (in.empty()) {
            LOG(ERROR)<<"Empty directory "<<data_dir;
            SendReply(sock, INVALID_INPUT);
            return -1;
        }
        sort(in.begin(), in.end());
        sort(out.begin(), out.end());
        if (judge.empty()) {
            for (int i = 0; i < in.size(); ++i) {
                if (i >= out.size() || in[i] < out[i]) {
                    LOG(ERROR)<<"No "<<in[i]<<".out found for "<<in[i]<<".in";
                    ret = -1;
                    break;
                } else if (in[i] > out[i]) {
                    LOG(ERROR)<<"No "<<out[i]<<".in found for "<<out[i]<<".out";
                    ret = -1;
                    break;
                }
            }
            if (out.size() > in.size()) {
                LOG(ERROR)<<"No "<<out[in.size()]<<".in found for "
                          <<out[in.size()]<<".out";
                ret = -1;
            }
        } else if (DoCompile(sock, root, compiler, data_dir + "/" + judge) == -1) {
            return -1;
        }
    }
    if (ret < 0) {
        SendReply(sock, INVALID_INPUT);
    }
    return ret;
}

int ExecDataCommand(int sock, const string& root, unsigned int problem_id, unsigned int revision) {
    uint32_t size;
    uint16_t checksum;
    if (ReadUint32(sock, &size) == -1 ||
        ReadUint16(sock, &checksum) == -1) {
        return -1;
    }
    if (CheckSum(CMD_DATA) + CheckSum(size) != checksum) {
        LOG(ERROR)<<"Invalid checksum "<<checksum;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }

    if (size > MAX_DATA_FILE_SIZE) {
        LOG(ERROR)<<"File size too large: "<<size;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }

    string problem_dir = StringPrintf("%s/prob/%u", root.c_str(), problem_id);
    string revisionDir = StringPrintf("%s/%u", problem_dir.c_str(), revision);
    string tempDir = StringPrintf("%s.%u.%s",
                                  revisionDir.c_str(),
                                  getpid(),
                                  GetLocalTimeAsString("%Y%m%d%H%M%S").c_str());
    LOG(INFO)<<"Creating temporary directory "<<tempDir;
    if (mkdir(tempDir.c_str(), 0750) == -1) {
        if (errno == ENOENT) {
            LOG(INFO)<<"Up level directory missing";
            LOG(INFO)<<"Creating problem directory "<<problem_dir;
            if (mkdir(problem_dir.c_str(), 0750) == -1) {
                if (errno != EEXIST) {
                    LOG(SYSCALL_ERROR)<<"Fail to create dir "<<problem_dir;
                    SendReply(sock, INTERNAL_ERROR);
                    return -1;
                }
            }
            LOG(INFO)<<"Creating temporary directory "<<tempDir;
            if (mkdir(tempDir.c_str(), 0750) == -1) {
                LOG(SYSCALL_ERROR)<<"Fail to create dir "<<tempDir;
                SendReply(sock, INTERNAL_ERROR);
                return -1;
            }
        } else if (errno != EEXIST) {
            LOG(SYSCALL_ERROR)<<"Fail to create dir "<<tempDir;
            SendReply(sock, INTERNAL_ERROR);
            return -1;
        }
    }
    LOG(INFO)<<"Saving data file. Size: "<<size;
    if (SaveFile(sock, tempDir + "/data.zip", size) == -1) {
        SendReply(sock, INTERNAL_ERROR);
        return -1;
    }
    LOG(INFO)<<"Unzipping data file";
    string command = StringPrintf("unzip '%s/data.zip' -d '%s'",
                                  tempDir.c_str(), tempDir.c_str());
    int result = system(command.c_str());
    if (result) {
        LOG(ERROR)<<"Fail to unzip data file. Command: "<<command;
        SendReply(sock, INVALID_INPUT);
        return -1;
    }
    LOG(INFO)<<"Checking data";
    if (CheckData(sock, root, tempDir) == -1) {
        return -1;
    }
    if (rename(tempDir.c_str(), revisionDir.c_str()) == -1) {
        LOG(SYSCALL_ERROR)<<"Fail to rename "<<tempDir<<" to "<<revisionDir;
        SendReply(sock, INTERNAL_ERROR);
        system(StringPrintf("rm -rf '%s'", tempDir.c_str()).c_str());
        return -1;
    }
    SendReply(sock, READY);
    return 0;
}

int JudgeMain(const string& root, const string& queue_address, int queue_port, int uid, int gid) {
    string working_root = StringPrintf("%s/working/%u", root.c_str(), getpid());
    int sock = ConnectTo(queue_address, queue_port);
    if (sock < 0) {
        return 1;
    }
    if (mkdir(working_root.c_str(), 0777) < 0) {
        if (errno != EEXIST) {
            LOG(SYSCALL_ERROR)<<"Fail to create dir "<<working_root;
            SendReply(sock, INTERNAL_ERROR);
            close(sock);
            return 1;
        }
    }
    if (chdir(working_root.c_str()) < 0) {
        LOG(SYSCALL_ERROR)<<"Fail to change working dir to "<<working_root;
        SendReply(sock, INTERNAL_ERROR);
        close(sock);
        return 1;
    }

    // installs handlers for tracing.
    InstallHandlers();

    int ret = 0;
    int problem_id = -1;
    int revision = -1;
    int compiler = 0;
    bool data_ready = false;
    // Loops until SIGTERM or SIGPIPE is received.
    while (!global::terminated) {
        uint8_t command;
        if (ReadUint8(sock, &command) == -1) {
            ret = 1;
            break;
        }
        if (command == CMD_JUDGE) {
            int result = ExecJudgeCommand(sock, root, &problem_id, &revision);
            if (result == -1) {
                ret = 1;
                break;
            }
            if (result == 0) {
                data_ready = true;
            } else {
                data_ready = false;
            }
            compiler = -1;
            // clear all temporary files.
            system(StringPrintf("rm -f %s/*", working_root.c_str()).c_str());
        } else if (command == CMD_DATA) {
            if (problem_id < 0) {
                LOG(ERROR)<<"No problem specified.";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (data_ready) {
                LOG(ERROR)<<"Data synchronization is not required.";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (ExecDataCommand(sock, root, problem_id, revision) == -1) {
                ret = 1;
                break;
            }
            data_ready = 1;
        } else if (command == CMD_COMPILE) {
            if (compiler >= 0) {
                LOG(ERROR)<<"Already compiled";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (ExecCompileCommand(sock,
                                   root,
                                   working_root,
                                   &compiler) == -1) {
                ret = 1;
                break;
            }
        } else if (command == CMD_TESTCASE) {
            if (problem_id < 0) {
                LOG(ERROR)<<"No problem specified.";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (data_ready == 0) {
                LOG(ERROR)<<"Data is not ready";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (compiler < 0) {
                LOG(ERROR)<<"Program is not compiled";
                SendReply(sock, INVALID_INPUT);
                ret = 1;
                break;
            }
            if (ExecTestCaseCommand(sock,
                                    root,
                                    problem_id,
                                    revision,
                                    compiler,
                                    uid,
                                    gid) == -1) {
                ret = 1;
                break;
            }
        } else {
            LOG(ERROR)<<"Invalid command "<<(int)command;
            SendReply(sock, INVALID_INPUT);
            ret = 1;
            break;
        }
    }
    close(sock);
    system(StringPrintf("rm -rf %s", working_root.c_str()).c_str());
    return ret;
}
