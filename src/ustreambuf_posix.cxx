/**
 * \file ustreambuf_posix.cxx
 *
 * \brief POSIX platform support for Unicode iostreams wr::ucin, wr::ucout,
 *        wr::ucerr and wr::uclog
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2016 James S. Waller
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
#include <unistd.h>
#include <stdio.h>
#include <streambuf>
#include <wrutil/codecvt.h>  // for u8buffer_convert and codecvt_utf8_narrow
#include <wrutil/uiostream.h>


namespace wr {


class u8streambuf :
        public u8buffer_convert
{
public:
        using this_t = u8streambuf;
        using base_t = u8buffer_convert;

        explicit
        u8streambuf(
                std::streambuf *bytebuf,
                std::locale     loc
        ) :
                u8buffer_convert(bytebuf, new codecvt_utf8_narrow(loc), {})
        {
                overflow(EOF);    // ready base class
        }
};

//--------------------------------------

std::streambuf *
getUStreamBuf(
        std::streambuf           *underlying_streambuf,
        FILE                     *c_stream,
        std::ios_base::openmode   /* mode */,
        std::streambuf          *&overlying_streambuf
)
{
        int fd = fileno(c_stream);

        if ((fd < 0) || !isatty(fd)) {
                /* underlying medium probably redirected file
                   so simply exchange raw UTF-8 */
                return underlying_streambuf;
        }

        // underlying medium is a terminal, arrange transcoding where necessary
        std::locale loc("");

        if (underlying_streambuf) {
                loc = underlying_streambuf->getloc();

                std::string name = loc.name();

                if (name == "C") {
                        loc = std::locale("");
                }
        } else {
                loc = std::locale("");
        }

        if (is_utf8(loc)) {  // terminal uses UTF-8 so no transcoding necessary
                return underlying_streambuf;
        } else {
                return overlying_streambuf
                        = new u8streambuf(underlying_streambuf, loc);
        }
}


} // namespace wr
