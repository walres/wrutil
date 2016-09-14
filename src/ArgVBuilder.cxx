/**
 * \file ArgVBuilder.cxx
 *
 * \brief Implementation of ArgVBuilder API
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
#include <stddef.h>
#include <wrutil/ArgVBuilder.h>


namespace wr {


WRUTIL_API ArgVBuilder::ArgVBuilder() : frozen_(false) {}

//--------------------------------------

WRUTIL_API const char * const *
ArgVBuilder::argv()
{
        freeze();
        return storage_.first.data();
}

//--------------------------------------

WRUTIL_API string_view
ArgVBuilder::operator[](
        size_t i
) const
{
        const char *result = storage_.first[i];

        if (!frozen_) {
                result = storage_.second.data()
                                + reinterpret_cast<size_t>(result);
        }

        return result;
}

//--------------------------------------

WRUTIL_API void
ArgVBuilder::clear()
{
        storage_.first.clear();
        storage_.second.clear();
        frozen_ = false;
}

//--------------------------------------

WRUTIL_API ArgVStorage
ArgVBuilder::extract()
{
        freeze();
        ArgVStorage result = std::move(storage_);
        clear();
        return result;
}

//--------------------------------------

WRUTIL_API void
ArgVBuilder::append(
        const string_view &arg
)
{
        insert(size(), arg);
}

//--------------------------------------

WRUTIL_API void
ArgVBuilder::insert(
        size_t             pos,
        const string_view &arg
)
{
        thaw();
        storage_.first.push_back(
                reinterpret_cast<const char *>(storage_.second.size()));
        storage_.second.insert(storage_.second.end(), arg.begin(), arg.end());
        storage_.second.push_back('\0');
}

//--------------------------------------

WRUTIL_API void
ArgVBuilder::erase(
        size_t pos
)
{
        storage_.first.erase(storage_.first.begin() + pos);
}

//--------------------------------------

void
ArgVBuilder::freeze()
{
        if (!frozen_) {
                for (const char *&arg: storage_.first) {
                        arg = storage_.second.data()
                                + reinterpret_cast<size_t>(arg);
                }
                frozen_ = true;
        }
}

//--------------------------------------

void
ArgVBuilder::thaw()
{
        if (frozen_) {
                for (const char *&arg: storage_.first) {
                        arg = reinterpret_cast<const char *>
                                        (arg - storage_.second.data());
                }
                frozen_ = false;
        }
}


} // namespace wr
