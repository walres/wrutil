/**
 * \file uiostream.cxx
 *
 * \brief initialisation and tear-down code for the uin, uout, uerr and ulog
 *        iostream objects
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
#include <wrutil/Config.h>
#include <stdlib.h>
#include <atomic>
#include <iostream>
#include <new>
#include <wrutil/uiostream.h>


namespace wr {


/*
 * These must be defined as raw storage to ensure that uiostream_init
 * is solely responsible for initialising them; if this is not done then
 * normal static initialisation clobbers the objects after
 * uiostream_init::uiostream_init() has been called due to unpredictable
 * static initialisation order with dynamic shared libraries)
 *
 * libc++ also uses this method to manage initialisation of
 * std::cin/out/err/log
 */
alignas(std::istream) static char uin_[sizeof(std::istream)];
alignas(std::ostream) static char uout_[sizeof(std::ostream)],
                                  uerr_[sizeof(std::ostream)],
                                  ulog_[sizeof(std::ostream)];

WRUTIL_API std::istream &uin  = *reinterpret_cast<std::istream *>(&uin_);
WRUTIL_API std::ostream &uout = *reinterpret_cast<std::ostream *>(&uout_),
                        &uerr = *reinterpret_cast<std::ostream *>(&uerr_),
                        &ulog = *reinterpret_cast<std::ostream *>(&ulog_);

static std::atomic_uint  uio_init_instances;
static std::streambuf   *uin_own_rdbuf  = nullptr,
                        *uout_own_rdbuf = nullptr,
                        *uerr_own_rdbuf = nullptr;

//--------------------------------------

std::streambuf *getUStreamBuf(std::streambuf *underlying_streambuf,
                              FILE *c_stream, std::ios_base::openmode mode,
                              std::streambuf *&overlying_streambuf);
        // platform-dependent, refer to ustreambuf_<platform>.cxx

//--------------------------------------

WRUTIL_API
uiostream_init::uiostream_init()
{
        if (!(uio_init_instances++)) {  // first call
                static std::ios_base::Init init_std_streams;

                new (uin_) std::istream(getUStreamBuf(std::cin.rdbuf(), stdin,
                                                      std::ios_base::in,
                                                      uin_own_rdbuf));

                new (uout_) std::ostream(getUStreamBuf(std::cout.rdbuf(),
                                                       stdout,
                                                       std::ios_base::out,
                                                       uout_own_rdbuf));

                new (uerr_) std::ostream(getUStreamBuf(std::cerr.rdbuf(),
                                                       stderr,
                                                       std::ios_base::out,
                                                       uerr_own_rdbuf));

                new (ulog_) std::ostream(uerr.rdbuf());

                uin.tie(&uout);
                std::unitbuf(uerr);
                uerr.tie(&uout);
        }
}

//--------------------------------------

WRUTIL_API
uiostream_init::~uiostream_init()
{
        if (!(--uio_init_instances)) {  // last call
                uin.rdbuf(nullptr);
                delete uin_own_rdbuf;
                uout.flush();
                uout.rdbuf(nullptr);
                delete uout_own_rdbuf;
                uerr.rdbuf(nullptr);
                ulog.flush();
                ulog.rdbuf(nullptr);
                delete uerr_own_rdbuf;

                using std::istream;
                uin.~istream();

                using std::ostream;
                uout.~ostream();
                uerr.~ostream();
                ulog.~ostream();
        }
}


} // namespace wr
