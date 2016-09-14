/**
 * \file Option.cxx
 *
 * \brief Implementation of command-line processing API
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
#include <ctype.h>
#include <errno.h>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <wrutil/Format.h>
#include <wrutil/codecvt.h>
#include <wrutil/ctype.h>
#include <wrutil/Option.h>
#include <wrutil/utf8.h>


namespace wr {


WRUTIL_API const char * const Option::UNKNOWN = "<UNKNOWN>";


WRUTIL_API
Option::Option(
        Names  names,
        Flags  flags,
        Action action
) :
        names_ (std::move(names)),
        flags_ (flags),
        action_(std::move(action))
{
}

//--------------------------------------

namespace {

struct OptionsByPrefix
{
        class Entry
        {
        public:
                Entry() : opt_(nullptr) {}

                Entry(string_view stem, const Option &opt) :
                        stem_(stem), opt_(&opt) {}

                bool operator<(string_view stem) const
                {
                        return stem_ < stem;
                }

                friend bool operator<(string_view stem, const Entry &other)
                {
                        return stem < other.stem_;
                }

                bool operator<(const Entry &other) const
                {
                        return stem_ < other;
                }

                bool match(string_view stem, char delim) const
                {
                        if (!stem.has_prefix(stem_)) {
                                return false;
                        }

                        bool exact_match = stem.size() == stem_.size();

                        if (!exact_match && (delim != 0)) {
                                auto pos = stem.find_first_not_of(" \t\n\r\f\v",
                                                                  stem_.size());
                                exact_match = (pos != stem.npos)
                                                && (stem[pos] == delim);
                        }

                        if (!opt_->takesArg() || opt_->separateArgOnly()) {
                                return exact_match;
                        } else if ((stem_.size() > 1)
                                                && !ispunct(stem_.back())) {
                                // joined argument delimiter '=' or ':'
                                auto trailing = stem.substr(stem_.size());

                                if (trailing.trim_left().empty()) {
                                        return exact_match;
                                }

                                return (trailing.front() == '=')
                                                || (trailing.front() == ':');
                        } else {
                                return true;
                        }
                }

                void stem(const string_view &s) { stem_ = s; }
                const string_view &stem() const { return stem_; }
                void opt(const Option &opt)     { opt_ = &opt; }
                const Option *opt() const       { return opt_; }

        private:
                string_view    stem_;
                const Option  *opt_;
        };

        std::vector<Entry> options_;
                /**< options sorted by (1) descending name-stem length and
                     (2) ascending name-stem content */
        bool short_only_ = true;
                ///< true if all option names under this prefix are single char

        void insert(string_view stem, const Option &opt)
        {
                options_.insert(
                       std::lower_bound(options_.begin(), options_.end(), stem),
                       { stem, opt });

                short_only_ = short_only_
                                && !stem.empty() && stem.has_max_size(1);
        }

        const Entry *find(string_view stem, char delim = 0) const
        {
                auto match = [this, &stem, delim](const Entry &entry) {
                        return entry.match(stem, delim);
                };

                auto i = std::find_if(options_.begin(), options_.end(), match);
                return (i != options_.end()) ? &(*i) : nullptr;
        }
};

} // anonymous namespace

//--------------------------------------
/*
 * Normal handler and non-option argument handler return values:
 *      x < 0: stop processing
 *      x = 0: continue
 *      x > 0: continue, if ARG_REQUIRED or ARG_OPTIONAL flag is specified for
 *             option then consume x extra arguments
 *
 * Unknown-option handler return values:
 *      x < 0: throw UnknownOption exception
 *      x = 0: continue
 *      x > 0: continue, if ARG_REQUIRED or ARG_OPTIONAL flag is specified for
 *             option then consume x extra arguments
 */
WRUTIL_API int
Option::parse(
        const Table &options,
        int          argc,
        const char **argv,
        int          pos,
        unsigned int flags
) // static
{
        if (pos < 0) {
                throw std::invalid_argument(printStr(
                        "Option::parse() requires pos >= 0, %d given", pos));
        }

        const Option           *nonopt_handler = nullptr;
        OptionsByPrefix::Entry  unknown_handler;
        std::map<string_view, OptionsByPrefix> prefixes;
        string_view pfx;
        ArgVStorage utf8_args;

        if (flags & ARGV_TO_UTF8) {
                utf8_args = localToUTF8(argc, argv);
                argv = utf8_args.first.data();
        }

        for (const Option &opt: options) {
                for (string_view name: opt.names()) {
                        if (name == UNKNOWN) {
                                unknown_handler.opt(opt);
                                continue;
                        } else if (name.empty()) {
                                nonopt_handler = &opt;
                                continue;
                        }

                        pfx = prefix(name);
                        auto j = prefixes.find(pfx);

                        if (j == prefixes.end()) {
                                j = prefixes.insert({pfx, {}}).first;
                                j->second.short_only_
                                        = !pfx.empty() && pfx.has_max_size(1);
                        }

                        string_view stem = name;
                        stem.remove_prefix(pfx.size());
                        j->second.insert(stem, opt);
                }
        }

        string_view                opt;
        std::string                full_opt;  // including prefix
        string_view                arg;
        decltype(prefixes.begin()) i;

        while (pos < argc) try {
                arg = {};

                const OptionsByPrefix::Entry *entry = nullptr;

                if (opt.empty()) {
                        opt = string_view(argv[pos]).trim();
                        pfx = prefix(opt);
                        full_opt.assign(pfx.data(), pfx.size());
                        i = prefixes.find(pfx);
                        opt.remove_prefix(pfx.size());
                }

                if (i != prefixes.end()) {
                        if (i->second.short_only_) {
                                entry = i->second.find(opt.substr(0, 1));
                        } else {
                                entry = i->second.find(opt);
                        }

                        if (!entry && !pfx.empty()) {
                                size_t opt_len;

                                if (i->second.short_only_) {
                                        // all options are single-character
                                        opt_len = 1;
                                } else {
                                        opt_len = opt.find_first_of(":=");
                                }

                                if (unknown_handler.opt()) {
                                        if (!unknown_handler.opt()->takesArg()){
                                                opt_len = opt.size();
                                        }
                                        unknown_handler.stem(
                                                        opt.substr(0, opt_len));
                                        entry = &unknown_handler;
                                } else {
                                        throw UnknownOption(pfx.to_string()
                                                            + opt.to_string());
                                }
                        }
                } else if (!pfx.empty()) {
                        throw UnknownOption(pfx.to_string() + opt.to_string());
                }

                if (!entry) {  // non-option argument
                        arg = opt;
                        opt = {};
                        ++pos;
                        if (nonopt_handler) {
                                int result = nonopt_handler->action_(
                                        "", arg, argc - pos, argv + pos);
                                if (result < 0) {
                                        break;
                                }
                                pos += result;
                        }
                        continue;
                }

                full_opt.replace(full_opt.begin() + pfx.size(),
                                 full_opt.end(), entry->stem().data(),
                                 entry->stem().size());

                opt.remove_prefix(entry->stem().size());
                        /* slice off option name from opt, anything left is
                           either a joined argument or grouped short options */

                bool have_arg = false;

                if (!entry->opt()->takesArg()) {
                        ;
                } else if (!opt.empty()) {
                        // joined argument or grouped single-character options
                        if (!entry->opt()->separateArgOnly()) {
                                // argument joined with option
                                arg = opt.trim();
                                opt = {};
                                have_arg = true;
                                if (entry->stem().has_min_size(2)
                                           && !ispunct(entry->stem().back())) {
                                        /* skip = or : between option
                                           and argument */
                                        arg.remove_prefix(1);
                                }
                        } /* else grouped single-character options; earlier
                             matching ruled out multi-character option name */
                } else if (!entry->opt()->joinedArgOnly()) {
                        // separate argument
                        ++pos;
                        if (!entry->stem().empty()
                                            && ispunct(entry->stem().back())) {
                                // arg must be directly joined to option
                                if (entry->opt()->argIsOptional()) {
                                        ;
                                } else if (unknown_handler.opt()) {
                                        unknown_handler.stem(opt);
                                        entry = &unknown_handler;
                                } else {
                                        throw UnknownOption(full_opt);
                                }
                        } else if (pos < argc) {
                                arg = argv[pos];
                                have_arg = true;
                        } else if (entry->opt()->argIsOptional()) {
                                --pos;
                        } else {
                                throw MissingArgument(full_opt);
                        }
                        opt = {};
                } else if (entry->opt()->argIsOptional()) {
                        opt = {};
                } else {
                        throw MissingArgument(full_opt);
                }

                if (have_arg && arg.empty()
                             && !entry->opt()->allowsEmptyArg()) {
                        throw InvalidArgument(full_opt, arg,
                                              "non-empty argument required");
                }

                if (opt.empty()) {
                        ++pos;
                }

                int result = entry->opt()->action_(
                                        full_opt, arg, argc - pos, argv + pos);
                if (result < 0) {
                        if (entry == &unknown_handler) {
                                throw UnknownOption(full_opt);
                        }
                        break;
                } else if (result > 0) {
                        pos += result;
                        opt = {};
                }
        } catch (InvalidArgument &err) {
                if (err.optionName().empty() || err.argument().empty()) {
                        throw InvalidArgument(full_opt, arg, err.reason());
                }
                throw;
        } catch (MissingArgument &err) {
                if (err.optionName().empty()) {
                        throw MissingArgument(full_opt);
                }
                throw;
        } catch (UnknownOption &err) {
                if (err.optionName().empty()) {
                        throw UnknownOption(full_opt);
                }
                throw;
        }

        return pos;
}

//--------------------------------------

WRUTIL_API size_t
Option::parseSubOptions(
        const Table &sub_options,
        string_view  opt_name,
        string_view  opt_arg,
        size_t       pos
) // static
{
        OptionsByPrefix        sorted_sub_opts;
        OptionsByPrefix::Entry unknown_handler;

        for (const Option &sub_option: sub_options) {
                for (string_view name: sub_option.names()) {
                        if (name == UNKNOWN) {
                                unknown_handler.opt(sub_option);
                        } else {
                                sorted_sub_opts.insert(name, sub_option);
                        }
                }
        }

        string_view content = opt_arg.substr(pos);

        while (!content.empty()) {
                auto delim_pos    = content.find_first_of(",:=");
                auto sub_opt_name = content.substr(0, delim_pos).trim();

                const OptionsByPrefix::Entry *entry
                                = sorted_sub_opts.find(sub_opt_name);
                if (!entry) {
                        if (sub_opt_name.empty()) {
                                auto col = opt_arg.size() - content.size() + 1;
                                throw InvalidArgument(opt_name, opt_arg,
                                        printStr("missing sub-option name at column %u",
                                                 col));
                        } else if (unknown_handler.opt()) {
                                entry = &unknown_handler;
                        } else {
                                throw UnknownOption(opt_name, sub_opt_name);
                        }
                }

                content.remove_prefix(std::min(delim_pos,
                                               size_t(string_view::npos)));

                string_view sub_opt_arg;
                bool        have_sub_opt_arg = false;

                if (!content.empty()) {
                        char delim = content.front();
                        content.remove_prefix(1).trim();

                        switch (delim) {
                        case ':': case '=':
                                have_sub_opt_arg = true;
                                if (entry->opt()->flags_
                                               & SUB_OPT_SELF_PARSE_ARG) {
                                        sub_opt_arg = content;
                                } else {
                                        sub_opt_arg = content.split(',')
                                                             .first.trim();
                                }
                                break;
                        case ',':
                                break;
                        default:
                                throw std::logic_error(printStr(
                                        "unexpected character '%c'", delim));
                                break;
                        }
                }

                if (have_sub_opt_arg) {
                        if (!entry->opt()->allowsEmptyArg()) {
                                if (sub_opt_arg.empty()) {
                                        throw InvalidArgument(opt_name,
                                                sub_opt_name, sub_opt_arg,
                                                "non-empty argument required");
                                }
                        }
                } else if (entry->opt()->argRequired()) {
                        throw MissingArgument(opt_name, sub_opt_name);
                }

                int result = 0;

                if (entry->opt()->action_) try {
                        result = entry->opt()->action_(
                                        sub_opt_name, sub_opt_arg, 0, nullptr);
                } catch (InvalidArgument &err) {
                        if (err.optionName().empty()
                                        || err.subOptionName().empty()
                                        || err.argument().empty()) {
                                throw InvalidArgument(opt_name, sub_opt_name,
                                                     sub_opt_arg, err.reason());
                        }
                        throw;
                } catch (MissingArgument &err) {
                        if (err.optionName().empty()
                                        || err.subOptionName().empty()) {
                                throw MissingArgument(opt_name, sub_opt_name);
                        }
                        throw;
                } catch (UnknownOption &err) {
                        if (err.optionName().empty()
                                        || err.subOptionName().empty()) {
                                throw UnknownOption(opt_name, sub_opt_name);
                        }
                        throw;
                }

                if (result < 0) {
                        if (entry == &unknown_handler) {
                                throw UnknownOption(opt_name, sub_opt_name);
                        }
                        errno = EINVAL;
                        break;
                } else if ((result > 0) || have_sub_opt_arg) {
                        content = content.substr(result)
                                         .split(',').second.trim();
                }
        }

        return opt_arg.size() - content.size();
}

//--------------------------------------

WRUTIL_API string_view
Option::prefix(
        string_view opt_name
) // static
{
        auto pfx_end = opt_name.begin();

        if (!opt_name.empty()) {
                switch (opt_name.front()) {
                case '-':
                        ++pfx_end;
                        if ((pfx_end != opt_name.end()) && (*pfx_end == '-')) {
                                ++pfx_end;
                        }
                        break;
                case '+':
#ifdef _WIN32
                case '/':
#endif
                        ++pfx_end;
                        break;
                default:
                        break;
                }
        }

        return { opt_name.begin(), pfx_end };
}

//--------------------------------------

WRUTIL_API ArgVStorage
Option::toArgVector(
        const string_view &command
) // static
{
        ArgVBuilder builder;

        auto add_arg = [](
                ArgVBuilder &builder,
                string_view  arg
        )
        {
                if (!arg.empty()) {
                        if ((arg.front() == '\'') || (arg.front() == '"')) {
                                if (arg.back() == arg.front()) {
                                        arg.remove_prefix(1);
                                        arg.remove_suffix(1);
                                }
                        }
                        builder.append(arg);
                }
        };

        auto        i = command.begin(), j = command.end();
        string_view arg(i, i);
        char        quote = 0;

        while (i < j) {
                switch (char c = *(i++)) {
                case '\'': case '"':
                        if (quote && (quote == c)) {
                                quote = 0;
                        } else if (!quote) {
                                quote = c;
                        }
                        arg = { arg.begin(), i };
                        break;
                default:
                        if (!c || (isspace(c) && !quote)) {
                                add_arg(builder, arg);
                                arg = { i, i };
                        } else {
                                arg = { arg.begin(), i };
                        }
                        break;
                }
        }

        add_arg(builder, arg);
        return builder.extract();
}

//--------------------------------------

WRUTIL_API ArgVStorage
Option::localToUTF8(
        int          argc,
        const char **argv
) // static
{
        if (argc <= 0) {
                return {};
        }

        std::unique_ptr<wr::codecvt_utf8_narrow> codecvt(
                                                   new wr::codecvt_utf8_narrow);
        std::vector<const char *>                argv2;

        if (codecvt->always_noconv()) {
                // local encoding is also UTF-8, use shallow copy
                argv2.assign(argv, argv + argc);
                return { std::move(argv2), {} };
        }

        wr::u8string_convert<> converter(codecvt.release());
        ArgVBuilder            builder;

        for (int i = 0; i < argc; ++i) {
                builder.append(converter.to_utf8(argv[i]));
        }

        return builder.extract();
}

//--------------------------------------

template <typename T> WRUTIL_API Option::Names &
Option::Names::copy(
        const T *names,
        size_t   count
)
{
        std::string *my_names;

        if (size_ >= count) {
                my_names = const_cast<std::string *>(names_);
        } else if (count > 0) {
                my_names = new std::string[count];
        } else {
                my_names = nullptr;
        }

        for (size_t i = 0; i < count; ++i) {
                my_names[i] = names[i];
        }

        if (names_ != my_names) {
                delete [] names_;
        }

        names_ = my_names;
        size_ = count;
        return *this;
}

template WRUTIL_API Option::Names &
Option::Names::copy<const char *>(const char * const *, size_t);

template WRUTIL_API Option::Names &
Option::Names::copy<std::string>(const std::string *, size_t);

//--------------------------------------

WRUTIL_API Option::Names &
Option::Names::operator=(
        Names &&other
)
{
        if (&other != this) {
                delete [] names_;
                size_ = 0;
                std::swap(names_, other.names_);
                std::swap(size_, other.size_);
        }

        return *this;
}

//--------------------------------------

WRUTIL_API
Option::UnknownOption::UnknownOption() :
        Error("unknown option")
{
}

//--------------------------------------

WRUTIL_API
Option::UnknownOption::UnknownOption(
        string_view opt_name
) :
        Error    ("unknown option '%s'", opt_name),
        opt_name_(opt_name.to_string())
{
}

//--------------------------------------

WRUTIL_API
Option::UnknownOption::UnknownOption(
        string_view opt_name,
        string_view sub_opt_name
) :
        Error        ("'%s' is not a sub-option of '%s'",
                      sub_opt_name, opt_name),
        opt_name_    (opt_name.to_string()),
        sub_opt_name_(sub_opt_name.to_string())
{
}

//--------------------------------------

WRUTIL_API
Option::MissingArgument::MissingArgument() :
        Error("missing option argument")
{
}

//--------------------------------------

WRUTIL_API
Option::MissingArgument::MissingArgument(
        string_view opt_name
) :
        Error    ("option '%s' requires an argument", opt_name),
        opt_name_(opt_name.to_string())
{
}

//--------------------------------------

WRUTIL_API
Option::MissingArgument::MissingArgument(
        string_view opt_name,
        string_view sub_opt_name
) :
        Error        ("sub-option '%s' of '%s' requires an argument",
                      sub_opt_name, opt_name),
        opt_name_    (opt_name.to_string()),
        sub_opt_name_(sub_opt_name.to_string())
{
}

//--------------------------------------

WRUTIL_API
Option::InvalidArgument::InvalidArgument(
        string_view reason
) :
        Error("invalid argument: %s", reason)
{
}

//--------------------------------------

WRUTIL_API
Option::InvalidArgument::InvalidArgument(
        string_view opt_name,
        string_view arg,
        string_view reason
) :
        Error    ("invalid argument \"%s\" for option '%s': %s",
                  arg, opt_name, reason),
        opt_name_(opt_name.to_string()),
        arg_     (arg.to_string()),
        reason_  (reason.to_string())
{
}

//--------------------------------------

WRUTIL_API
Option::InvalidArgument::InvalidArgument(
        string_view opt_name,
        string_view sub_opt_name,
        string_view arg,
        string_view reason
) :
        Error        ("invalid argument \"%s\" for sub-option '%s' of '%s': %s",
                      arg, sub_opt_name, opt_name, reason),
        opt_name_    (opt_name.to_string()),
        sub_opt_name_(sub_opt_name.to_string()),
        arg_         (arg.to_string()),
        reason_      (reason.to_string())
{
}

//--------------------------------------

WRUTIL_API int
Option::Action::operator()(
        string_view         opt,
        string_view         arg,
        int                 remaining_argc,
        const char * const *remaining_argv
) const
{
        if (action_) {
                return action_(opt, arg, remaining_argc, remaining_argv);
        } else {
                return 0;
        }
}


} // namespace wr
