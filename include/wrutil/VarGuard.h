/**
 * \file VarGuard.h
 *
 * \brief Class for scope-based saving and reverting of variable values
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2013-2016 James S. Waller
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * \endparblock
 */
#ifndef WRUTIL_VAR_GUARD_H
#define WRUTIL_VAR_GUARD_H

#include <utility>  // for std::move()


namespace wr {


template <typename T>
class VarGuard
{
public:
        VarGuard(T &current) : current_(current), saved_(current) {}

        VarGuard(const VarGuard &other) = delete;
        VarGuard(VarGuard &&) = delete;

        ~VarGuard()            { current_ = std::move(saved_); }

        T &commit()            { saved_ = current_; return current_; }
        T &rollback()          { return current_ = saved_; }

        T &saved()             { return saved_; }
        const T &saved() const { return saved_; }

        VarGuard &operator=(VarGuard &r) = delete;
        VarGuard &operator=(VarGuard &&r) = delete;


private:
        T &current_, saved_;
};


} // namespace wr


#endif // WRUTIL_VAR_GUARD_H
