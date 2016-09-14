/**
 * \file numeric_cast.h
 *
 * \brief Wrapper for the Boost Numeric Conversion library's numeric_cast
 *        function
 *
 * \copyright
 * \parblock
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
 * \endparblock
 */
#ifndef WRUTIL_NUMERIC_CAST_H
#define WRUTIL_NUMERIC_CAST_H

#include <boost/numeric/conversion/cast.hpp>


namespace wr {


using boost::numeric_cast;
using boost::numeric::bad_numeric_cast;
using boost::numeric::positive_overflow;
using boost::numeric::negative_overflow;


} // namespace wr


#endif // !WRUTIL_NUMERIC_CAST_H
