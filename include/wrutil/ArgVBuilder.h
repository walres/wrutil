/**
 * \file ArgVBuilder.h
 *
 * \brief API for breaking command strings into argument vectors
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
#ifndef WRUTIL_ARGV_BUILDER_H
#define WRUTIL_ARGV_BUILDER_H

#include <utility>
#include <vector>
#include <wrutil/Config.h>
#include <wrutil/string_view.h>


namespace wr {


using ArgVStorage = std::pair<std::vector<const char *>, std::vector<char>>;

//--------------------------------------

class WRUTIL_API ArgVBuilder
{
public:
        ArgVBuilder();

        const char * const *argv();
        string_view operator[](size_t i) const;

        bool empty() const  { return storage_.first.empty(); }
        size_t size() const { return storage_.first.size(); }

        void clear();
        ArgVStorage extract();

        void append(const string_view &arg);
        void insert(size_t pos, const string_view &arg);
        void erase(size_t pos);

private:
        void freeze();
        void thaw();

        ArgVStorage storage_;
        bool        frozen_;
};


} // namespace wr


#endif // !WRUTIL_ARGV_BUILDER_H
