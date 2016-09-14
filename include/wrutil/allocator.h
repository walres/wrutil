/**
 * \file allocator.h
 *
 * \brief custom allocator types
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
#ifndef WRUTIL_ALLOCATOR_H
#define WRUTIL_ALLOCATOR_H

#include <memory>
#include <stdexcept>


namespace wr
{


struct no_allocator
{
public:
        using this_type = no_allocator;
        using value_type = void;
        using pointer = void *;
        using const_pointer = const void *;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;
        template <typename> struct rebind { using other = this_type; };
        using is_always_equal = std::true_type;

        pointer
        allocate(
                size_type                           /* n */,
                std::allocator<void>::const_pointer /* hint */ = nullptr
        )
        {
                throw std::bad_alloc();
        }

        template <typename U, typename ...Args> void construct(U *, Args &&...)
                { throw std::logic_error(
                                "automatic element construction disabled"); }

        void deallocate(pointer /* p */, size_type /* n */) { /* no-op */ }

        template <typename U> void destroy(U */* p */) { /* no-op */ }

        size_type max_size() const { return 0; }

        bool operator==(const this_type &) { return true; }
        bool operator!=(const this_type &) { return false; }
};


} // namespace wr


#endif // !WRUTIL_ALLOCATOR_H
