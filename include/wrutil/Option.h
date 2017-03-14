/**
 * \file Option.h
 *
 * \brief Command-line option processing API
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
#ifndef WRUTIL_OPTION_H
#define WRUTIL_OPTION_H

#include <functional>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <wrutil/Config.h>
#include <wrutil/ArgVBuilder.h>
#include <wrutil/string_view.h>
#include <wrutil/u8string_view.h>  // for wr::to_int()


namespace wr {


class WRUTIL_API Option
{
public:
        struct Error;
        class UnknownOption;
        class MissingArgument;
        class InvalidArgument;

        using NamesInitList = std::initializer_list<const char *>;
        using OptionsInitList = std::initializer_list<Option>;
        using Flags = unsigned int;

        /**
         * \brief Code to execute upon parsing an option
         *
         * \c Action wraps a callable object provided by the client for
         * attachment to an Option object and will be executed on parsing that
         * option.
         *
         * \c Action allows a variety of function signatures to be used for
         * conciseness of client code, and depending on what the client wishes
         * to accomplish:
         *
         * - \code void f();\endcode (1)
         * - \code int f();\endcode (2)
         * - \code void f(wr::string_view arg);\endcode (3)
         * - \code int f(wr::string_view arg);\endcode (4)
         * - \code void f(wr::string_view opt, wr::string_view arg);
         *   \endcode (5)
         * - \code int f(wr::string_view opt, wr::string_view arg);
         *   \endcode (6)
         * - \code int f(wr::string_view arg, int argc,
         *               const char * const argv);\endcode (7)
         * - \code int f(wr::string_view opt, wr::string_view arg,
         *               int argc, const char * const argv);\endcode (8)
         *
         * (1) and (2) provide for simple code that does not require the name
         * or argument of the matched option; (3) and (4) pass the option's
         * argument, if present; (5) and (6) pass both the option name and
         * the option's argument (opt will be empty when handling a non-option
         * argument); (7) and (8) are intended for Option::parse() and pass the
         * count (argc) and string array (argv) of any further unparsed
         * argument(s), allowing the code to consume any number of available
         * arguments.
         *
         * The integer return value of signatures (2), (4), (6), (7) and (8)
         * above is interpreted differently by Option::parse() and
         * Option::parseSubOption():
         *
         * - Option::parse()
         *      - a negative value will cause Option::parse() to stop and return
         *        the index of the next unparsed argument (for most Option
         *        objects), or throw an UnknownOption exception (for an Option
         *        object registered with the name Option::UNKNOWN used to
         *        handle unrecognised options)
         *      - a value of zero will cause parsing to continue from the next
         *        unparsed argument
         *      - a positive value will cause parsing to skip over the
         *        indicated number of arguments and continue parsing if
         *        further arguments exist
         * - Option::parseSubOptions()
         *      - a negative value will cause Option::parseSubOptions() to stop
         *        and return the index of the next unparsed character in opt_arg
         *        (for most Option objects), or throw an UnknownOption exception
         *        (for an Option object registered with the name Option::UNKNOWN
         *        used to handle unrecognised options)
         *      - a value of zero will cause parsing to continue from the next
         *        unparsed character
         *      - a positive value will cause parsing to skip over the
         *        indicated number of characters and continue parsing if
         *        further characters exist
         */
        class WRUTIL_API Action
        {
        public:
                Action() = default;
                Action(const Action &) = default;
                Action(Action &&) = default;

                Action(Action &other) :
                        Action(static_cast<const Action &>(other)) {}
                                // avoid infinite recursion with template version

                template <typename Fn> Action(Fn &&f)
                        { make(std::forward<Fn>(f)); }

                ~Action() = default;

                void reset() { action_ = {}; }

                Action &operator=(const Action &) = default;
                Action &operator=(Action &&) = default;

                Action &operator=(Action &other)
                        { return *this = static_cast<const Action &>(other); }
                                // avoid infinite recursion with template version

                template <typename Fn> Action &operator=(Fn &&f)
                        { make(std::forward<Fn>(f)); return *this; }

                explicit operator bool() const
                        { return static_cast<bool>(action_); }

                int operator()(string_view opt, string_view arg,
                               int argc, const char * const *argv) const;

        private:
                template <typename T> using enable_if_void
                        = typename std::enable_if<
                                std::is_same<T, void>::value>::type;

                template <typename T> using enable_if_int
                        = typename std::enable_if<
                                std::is_convertible<T, int>::value>::type;

                /*
                 * use SFINAE on 2nd parameter to pick appropriate callable type
                 * 3rd parameter used where needed to make overloads look
                 * different enough to satisfy the C++ compiler
                 */
                template <typename Fn> void
                make(
                        Fn                             f,
                        enable_if_void<decltype(f())> * = {}
                )
                {
                        action_ = [f](
                                string_view,
                                string_view,
                                int,
                                const char * const *
                        )
                        {
                                f();
                                return 0;
                        };
                }

                template <typename Fn> void
                make(
                        Fn                            f,
                        enable_if_int<decltype(f())> * = {}
                )
                {
                        action_ = [f](
                                string_view,
                                string_view,
                                int,
                                const char * const *
                        )
                        {
                                return f();
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                          f,
                        enable_if_void<decltype(f(string_view{}))> * = {},
                        void                                       * = {}
                )
                {
                        action_ = [f](
                                string_view,
                                string_view         arg,
                                int,
                                const char * const *
                        )
                        {
                                f(arg);
                                return 0;
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                         f,
                        enable_if_int<decltype(f(string_view{}))> * = {},
                        void                                      * = {}
                )
                {
                        action_ = [f](
                                string_view,
                                string_view         arg,
                                int,
                                const char * const *
                        )
                        {
                                return f(arg);
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                           f,
                        enable_if_void<decltype(f(string_view{},
                                                  string_view{}))>  * = {},
                        void                                       ** = {}
                )
                {
                        action_ = [f](
                                string_view         opt,
                                string_view         arg,
                                int,
                                const char * const *
                        )
                        {
                                f(opt, arg);
                                return 0;
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                          f,
                        enable_if_int<decltype(f(string_view{},
                                                 string_view{}))>  * = {},
                        void                                      ** = {}
                )
                {
                        action_ = [f](
                                string_view         opt,
                                string_view         arg,
                                int,
                                const char * const *
                        )
                        {
                                return f(opt, arg);
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                               f,
                        enable_if_int<
                                decltype(f(string_view{}, int(0),
                                           (const char * const *)(0)))> * = {},
                        void                                          *** = {}
                )
                {
                        action_ = [f](
                                string_view,
                                string_view         arg,
                                int                 remaining_argc,
                                const char * const *remaining_argv
                        )
                        {
                                return f(arg, remaining_argc, remaining_argv);
                        };
                }

                template <typename Fn> void
                make(
                        Fn                                                f,
                        enable_if_int<
                                decltype(f(string_view{},
                                           string_view{}, int(0),
                                           (const char * const *)(0)))>  * = {},
                        void                                          **** = {}
                )
                {
                        action_ = [f](
                                string_view         opt,
                                string_view         arg,
                                int                 remaining_argc,
                                const char * const *remaining_argv
                        )
                        {
                                return f(opt, arg, remaining_argc,
                                         remaining_argv);
                        };
                }

                std::function<int (string_view, string_view,
                                   int, const char * const *)> action_;
        };

        /**
         * \brief Package of one or more option names
         */
        class WRUTIL_API Names
        {
        public:
                Names() = default;
                Names(const char * const *names, size_t count)
                        { copy(names, count); }
                        
                Names(const std::string *names, size_t count)
                        { copy(names, count); }

                Names(const char *name) : Names(&name, 1) {}
                Names(const std::string &name) : Names(name.c_str()) {}

                template <size_t N> Names(const char * const (&names)[N])
                        { copy(names, N); }

                Names(NamesInitList names) :
                        Names(names.begin(), names.size()) {}

                Names(const Names &other) { copy(other.names_, other.size_); }
                Names(Names &&other) : Names() { *this = std::move(other); }

                ~Names() { delete [] names_; }

                Names &operator=(const Names &other)
                        { return copy(other.names_, other.size_); }

                Names &operator=(Names &&other);

                const std::string &operator[](size_t i) const
                        { return names_[i]; }

                size_t size() const { return size_; }

                const std::string *begin() const { return names_; }
                const std::string *end() const   { return names_ + size_; }

        private:
                template <typename T> Names &copy(const T *names, size_t count);

                const std::string *names_ = nullptr;
                size_t             size_  = 0;
        };

        /**
         * \brief Collect Option objects for use with Option::parse()
         * and Option::parseSubOptions()
         *
         * \c Table is a helper class used to assemble a collection of Option
         * objects from different kinds of source which may be a constant
         * array, an initializer list or a dynamic array with explicit size.
         */
        class WRUTIL_API Table
        {
        public:
                /// construct from static array of Option objects
                template <size_t N> Table(const Option (&options)[N]) :
                        options_(options), size_(N) {}

                /// construct from Option initializer list
                Table(OptionsInitList options) :
                        options_(options.begin()), size_(options.size()) {}

                /// construct from dynamic array
                Table(const Option *options, size_t count) :
                        options_(options), size_(count) {}

                const Option *begin() const { return options_; }
                const Option *end() const   { return options_ + size_; }

        private:
                const Option *options_;
                size_t        size_;
        };

        enum
        {
                ARG_REQUIRED           = 1U,
                ARG_OPTIONAL           = (1U << 1),
                NON_EMPTY_ARG          = (1U << 2),
                NON_EMPTY_ARG_REQUIRED = ARG_REQUIRED | NON_EMPTY_ARG,
                NON_EMPTY_ARG_OPTIONAL = ARG_OPTIONAL | NON_EMPTY_ARG,
                JOINED_ARG_ONLY        = (1U << 3),
                SEPARATE_ARG_ONLY      = (1U << 4),
                SUB_OPT_SELF_PARSE_ARG = (1U << 5)
        };

        static const char * const UNKNOWN;

        Option() = default;
        Option(const Option &) = default;
        Option(Option &&) = default;

        Option(Names names, Flags flags, Action action);
        Option(Names names, Action action) :
                Option(std::move(names), 0, std::move(action)) {}
        Option(Names names) : Option(std::move(names), 0, {}) {}

        Option &operator=(const Option &) = default;
        Option &operator=(Option &&) = default;

        const Names &names() const   { return names_; }
        Flags flags() const          { return flags_; }
        const Action &action() const { return action_; }
        bool argRequired() const     { return (flags_ & ARG_REQUIRED) != 0; }
        bool allowsEmptyArg() const  { return !(flags_ & NON_EMPTY_ARG); }
        bool argIsOptional() const   { return (flags_ & ARG_OPTIONAL) != 0; }
        bool takesArg() const
                { return (flags_ & (ARG_REQUIRED | ARG_OPTIONAL)) != 0; }
        bool joinedArgOnly() const { return (flags_ & JOINED_ARG_ONLY) != 0; }
        bool separateArgOnly() const
                { return (flags_ & SEPARATE_ARG_ONLY) != 0; }

        enum
        {
                ARGV_TO_UTF8 = 1
        };

        static int parse(const Table &options, int argc, const char **argv,
                         int pos, unsigned int flags = 0);

        static int parse(const Table &options, int argc, char **argv, int pos,
                         unsigned int flags = 0)
                { return parse(options,
                               argc, (const char **) argv, pos, flags); }

        static size_t parseSubOptions(const Table &sub_options,
                                      string_view opt_name, string_view opt_arg,
                                      size_t pos = 0);

        static string_view prefix(string_view opt_name);

        static ArgVStorage toArgVector(const string_view &command);

        static ArgVStorage localToUTF8(int argc, const char **argv);

        static ArgVStorage localToUTF8(int argc, char **argv)
                { return localToUTF8(argc, (const char **) argv); }

        template <typename T> static T
                toInt(const string_view &s,
                      size_t *end_code_point_offset = nullptr, int base = 10,
                      T min_val = std::numeric_limits<T>::min(),
                      T max_val = std::numeric_limits<T>::max());

        template <typename T> static T
                toFloat(const string_view &s,
                        size_t *end_code_point_offset = nullptr,
                        T min_val = std::numeric_limits<T>::min(),
                        T max_val = std::numeric_limits<T>::max());

private:
        Names  names_;
        Flags  flags_ = 0;
        Action action_;
};

//--------------------------------------

struct WRUTIL_API Option::Error :
        public std::runtime_error
{
        template <typename ...Args> Error(const char *fmt, Args &&...args) :
                std::runtime_error(
                        printStr(fmt, std::forward<Args>(args)...)) {}
};

//--------------------------------------

class WRUTIL_API Option::UnknownOption :
        public Option::Error
{
public:
        UnknownOption();
        UnknownOption(string_view opt_name);
        UnknownOption(string_view opt_name, string_view sub_opt_name);

        string_view optionName() const { return opt_name_; }
        string_view subOptionName() const { return sub_opt_name_; }

private:
        std::string opt_name_, sub_opt_name_;
};

//--------------------------------------

class WRUTIL_API Option::MissingArgument :
        public Option::Error
{
public:
        MissingArgument();
        MissingArgument(string_view opt_name);
        MissingArgument(string_view opt_name, string_view sub_opt_name);

        string_view optionName() const { return opt_name_; }
        string_view subOptionName() const { return sub_opt_name_; }

private:
        std::string opt_name_, sub_opt_name_;
};

//--------------------------------------

class WRUTIL_API Option::InvalidArgument :
        public Option::Error
{
public:
        InvalidArgument(string_view reason);
        InvalidArgument(string_view opt_name, string_view arg,
                        string_view reason);

        InvalidArgument(string_view opt_name, string_view sub_opt_name,
                        string_view arg, string_view reason);

        string_view optionName() const { return opt_name_; }
        string_view subOptionName() const { return sub_opt_name_; }
        string_view argument() const { return arg_; }
        string_view reason() const { return reason_; }

private:
        std::string opt_name_, sub_opt_name_, arg_, reason_;
};

//--------------------------------------

template <typename T> T
Option::toInt(
        const string_view &s,
        size_t            *end_code_point_offset,
        int                base,
        T                  min_val,
        T                  max_val
) // static
try {
        return wr::to_int<T>(s, end_code_point_offset, base, min_val, max_val);
} catch (const std::exception &err) {
        throw Option::InvalidArgument(err.what());
}

//--------------------------------------

template <typename T> T
Option::toFloat(
        const string_view &s,
        size_t            *end_code_point_offset,
        T                  min_val,
        T                  max_val
) // static
try {
        return wr::to_float<T>(s, end_code_point_offset, min_val, max_val);
} catch (const std::exception &err) {
        throw Option::InvalidArgument(err.what());
}


} // namespace wr


#endif // !WRUTIL_OPTION_H
