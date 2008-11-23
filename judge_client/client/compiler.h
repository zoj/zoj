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

#ifndef __COMPILE_H
#define __COMPILE_H

#include <string>
#include <map>
#include <vector>

using namespace std;

class Compiler {
  public:
    Compiler(int id, const string& compiler_name, const string& source_file_extension)
        : id_(id), compiler_name_(compiler_name), source_file_extension_(source_file_extension) {
    }

    int id() const {
        return id_;
    }

    const string& compiler_name() const {
        return compiler_name_;
    }

    const string& source_file_extension() const {
        return source_file_extension_;
    }

    int Compile(int sock, const string& source_file_name) const;

  private:
    int id_;
    string compiler_name_;
    string source_file_extension_;
};

class CompilerManager {
  public:
    static const CompilerManager* GetInstance();

    vector<const Compiler*> GetAllSupportedCompilers() const;

    const Compiler* GetCompiler(int compiler) const;

    const Compiler* GetCompilerByExtension(const string& extension) const;

  private:
    CompilerManager(const string& supported_compilers);

    static CompilerManager* instance_;
    map<int, const Compiler*> compiler_map_;

  friend class ExecCompileCommandTest;
  friend class CheckDataTest;
};

#endif
