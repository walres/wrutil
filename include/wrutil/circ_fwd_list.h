/**
 * \file circ_fwd_list.h
 *
 * \brief Intrusive and non-intrusive circular singly linked-lists
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
#ifndef WRUTIL_CIRC_FWD_LIST_H
#define WRUTIL_CIRC_FWD_LIST_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>


namespace wr {


template <typename Node, typename Traits> class intrusive_circ_fwd_list;


template <typename Traits>
class circ_fwd_list_iterator
{
public:
        using this_type = circ_fwd_list_iterator;
        using traits_type = Traits;
        using node_ptr_type = typename traits_type::node_ptr_type;
        using node_ptr_ret_type = typename traits_type::node_ptr_ret_type;
        using value_type = typename traits_type::value_type;
        using pointer = typename traits_type::pointer;
        using reference = typename traits_type::reference;
        using difference_type = typename traits_type::difference_type;
        using iterator_category = std::forward_iterator_tag;

        circ_fwd_list_iterator() : last_(nullptr), pos_(nullptr) {}
        circ_fwd_list_iterator(const this_type &other) = default;
        circ_fwd_list_iterator(this_type &&other) = default;

        // support copying const-type iterators to non-const-type iterators
        template <typename X>
        circ_fwd_list_iterator(const circ_fwd_list_iterator<X> &other) :
                  last_(other.last_), pos_(other.node()) {}

        circ_fwd_list_iterator(const node_ptr_type *last, node_ptr_type pos) :
                last_(last), pos_(pos) {}

        reference operator*() const
                { return *traits_type::get_value_ptr(pos_); }

        pointer operator->() const
                { return traits_type::get_value_ptr(pos_); }

        /// prefix increment
        circ_fwd_list_iterator &
        operator++()
        {
                if (pos_ == (*last_)) {  // *this == last; go to end
                        pos_ = nullptr;
                } else {
                        if (!pos_) {  // *this == end; go to beginning
                                pos_ = *last_;
                        }
                        pos_ = traits_type::next_node(pos_);
                }
                return *this;
        }

        /// postfix increment
        circ_fwd_list_iterator
        operator++(
                int
        )
        {
                this_type old(*this);
                ++(*this);
                return old;
        }

        circ_fwd_list_iterator &operator=(const this_type &other) = default;
        circ_fwd_list_iterator &operator=(this_type &&other) = default;

        // support copying const-type iterators to non-const-type iterators
        template <typename U> circ_fwd_list_iterator &
        operator=(
                circ_fwd_list_iterator<U> &other
        )
        {
                last_ = other.last_;
                pos_ = other.node();
                return *this;
        }

        // support comparing const-type iterators with non-const-type iterators
        template <typename U> bool
                operator==(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ == other.pos_; }

        template <typename U> bool
                operator!=(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ != other.pos_; }

        template <typename U> bool
                operator<=(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ <= other.pos_; }

        template <typename U> bool
                operator<(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ < other.pos_; }

        template <typename U> bool
                operator>=(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ >= other.pos_; }

        template <typename U> bool
                operator>(const circ_fwd_list_iterator<U> &other) const
                        { return pos_ > other.pos_; }

        friend bool operator==(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ == b; }

        friend bool operator==(node_ptr_ret_type a, const this_type &b)
                { return a == b.pos_; }

        friend bool operator!=(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ != b; }

        friend bool operator!=(node_ptr_ret_type a, const this_type &b)
                { return a != b.pos_; }

        friend bool operator<=(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ <= b; }

        friend bool operator<=(node_ptr_ret_type a, const this_type &b)
                { return a <= b.pos_; }

        friend bool operator<(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ < b; }

        friend bool operator<(node_ptr_ret_type a, const this_type &b)
                { return a < b.pos_; }

        friend bool operator>=(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ >= b; }

        friend bool operator>=(node_ptr_ret_type a, const this_type &b)
                { return a >= b.pos_; }

        friend bool operator>(const this_type &a, node_ptr_ret_type b)
                { return a.pos_ > b; }

        friend bool operator>(node_ptr_ret_type a, const this_type &b)
                { return a > b.pos_; }

        void
        swap(
                this_type &other
        )
        {
                std::swap(last_, other.last_);
                std::swap(pos_, other.pos_);
        }

        node_ptr_ret_type node() const { return pos_; }
        explicit operator node_ptr_ret_type() const { return node(); }
        explicit operator bool() const { return (node() != nullptr); }

private:
        template <typename> friend class circ_fwd_list_iterator;
        friend intrusive_circ_fwd_list<typename traits_type::node_type,
                                       typename traits_type::list_traits>;

        const node_ptr_type *last_;
        node_ptr_type        pos_;
};

//--------------------------------------

template <typename Node, typename Alloc = std::allocator<Node>>
struct intrusive_list_traits
{
        using node_type = Node;
        using node_ptr_type = node_type *;
        using const_node_ptr_type = const node_type *;
        using value_type = node_type;
        using reference = node_type &;
        using const_reference = const node_type &;
        using pointer = node_type *;
        using const_pointer = const node_type *;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using allocator_type = Alloc;
        using allocator_traits = std::allocator_traits<allocator_type>;

        static pointer get_value_ptr(node_ptr_type node) { return node; }

        static node_ptr_type next_node(node_ptr_type node)
                { return node->next(); }

        static void set_next_node(node_ptr_type node, node_ptr_type next)
                { node->next(next); }

        static void
        destroy_node(
                allocator_type &allocator,
                node_ptr_type   node
        )
        {
                allocator.destroy(node);
                allocator.deallocate(node, 1);
        }
};

//--------------------------------------

template <typename Node, typename Traits = wr::intrusive_list_traits<Node>>
class intrusive_circ_fwd_list :
        Traits::allocator_type  /* take advantage of empty base class
                                   optimisation where possible */
{
public:
        using this_type = intrusive_circ_fwd_list;
        using traits_type = Traits;
        using node_type = typename traits_type::node_type;
        using node_ptr_type = typename traits_type::node_ptr_type;
        using value_type = typename traits_type::value_type;
        using reference = typename traits_type::reference;
        using const_reference = typename traits_type::const_reference;
        using pointer = typename traits_type::pointer;
        using const_pointer = typename traits_type::const_pointer;
        using size_type = typename traits_type::size_type;
        using difference_type = typename traits_type::difference_type;
        using allocator_type = typename traits_type::allocator_type;
        using allocator_traits = typename traits_type::allocator_traits;

        struct iterator_traits : intrusive_circ_fwd_list::traits_type
        {
                using this_type = iterator_traits;
                using list_traits = Traits;
                using node_ptr_ret_type = typename list_traits::node_ptr_type;
        };

        using iterator = circ_fwd_list_iterator<iterator_traits>;
        using reverse_iterator = std::reverse_iterator<iterator>;

        struct const_iterator_traits : intrusive_circ_fwd_list::traits_type
        {
                using this_type = const_iterator_traits;
                using list_traits = Traits;
                using node_ptr_ret_type
                        = typename list_traits::const_node_ptr_type;
                using pointer = typename list_traits::const_pointer;
                using reference = typename list_traits::const_reference;
        };

        using const_iterator = circ_fwd_list_iterator<const_iterator_traits>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /// \brief Construct empty list with default allocator
        intrusive_circ_fwd_list() {} /* = default; */
                /* NB: GCC 4.x gets confused by '= default' used with
                   default constructor */

        /**
         * \brief Construct empty list with specified allocator
         * \param [in] alloc
         *      the allocator object
         */
        explicit intrusive_circ_fwd_list(const allocator_type &alloc) :
                allocator_type(alloc) {}

        intrusive_circ_fwd_list(
                size_type             count,
                const value_type     &value,
                const allocator_type &alloc = {}
        ) :
                intrusive_circ_fwd_list(alloc)
        {
                assign(count, value);
        }

        intrusive_circ_fwd_list(
                size_type             count,
                const allocator_type &alloc = {}
        ) :
                intrusive_circ_fwd_list(alloc)
        {
                assign(count, {});
        }

        template <typename InIter>
        intrusive_circ_fwd_list(
                InIter                first,
                InIter                last,
                const allocator_type &alloc = {}
        ) :
                intrusive_circ_fwd_list(alloc)
        {
                assign(first, last);
        }

        intrusive_circ_fwd_list(
                const this_type &other
        ) :
                intrusive_circ_fwd_list(allocator_traits::
                                select_on_container_copy_construction(
                                                        other.get_allocator()))
        {
                assign(other.begin(), other.end());
        }

        intrusive_circ_fwd_list(this_type &&other) { *this = std::move(other); }

        intrusive_circ_fwd_list(
                this_type            &&other,
                const allocator_type  &alloc
        ) :
                intrusive_circ_fwd_list(alloc)
        {
                *this = std::move(other);
        }

        intrusive_circ_fwd_list(
                std::initializer_list<value_type>  l,
                const allocator_type              &alloc = {}
        ) :
                intrusive_circ_fwd_list(alloc)
        {
                assign(l);
        }

        ~intrusive_circ_fwd_list() { clear(); }

        reference front()             { return *traits_type::next_node(last_); }
        const_reference front() const { return *traits_type::next_node(last_); }
        reference back()              { return *last_; }
        const_reference back() const  { return *last_; }

        iterator before_begin()              { return end(); }
        const_iterator before_begin() const  { return cend(); }
        const_iterator cbefore_begin() const { return cend(); }

        iterator begin()             { return iterator(&last_, first_node()); }
        const_iterator begin() const { return cbegin(); }
        const_iterator cbegin() const
                { return const_iterator(&last_, first_node()); }

        iterator last()              { return iterator(&last_, last_); }
        const_iterator last() const  { return clast(); }
        const_iterator clast() const { return const_iterator(&last_, last_); }

        iterator end()               { return iterator(&last_, nullptr); }
        const_iterator end() const   { return cend(); }
        const_iterator cend() const  { return const_iterator(&last_, nullptr); }

        iterator make_iterator(node_ptr_type pos)
                { return iterator(&last_, pos); }

        const_iterator make_iterator(node_ptr_type pos) const
                { return make_const_iterator(pos); }

        const_iterator make_const_iterator(node_ptr_type pos) const
                { return const_iterator(&last_, pos); }

        /**
         * \brief Test if list is empty
         *
         * Complexity is constant time. It is recommended to call this
         * function instead of writing <tt>(size() > 0)</tt> or similar,
         * as size() must iterate through the entire list.
         *
         * \return
         *      \c true if list is empty, \c false otherwise
         */
        bool empty() const { return !last_; }

        /**
         * \brief Get number of elements
         *
         * Complexity is linear time as this function must iterate through
         * all elements.
         *
         * \return
         *      number of elements in the list
         */
        size_type size() const { return std::distance(begin(), end()); }

        /// \brief Query maximum possible list size
        size_type max_size() const { return size_type(0) - 1U; }

        /**
         * \brief Erase all list contents
         *
         * All nodes are unlinked then destructed and their memory freed
         * using the list's allocator.
         */
        void
        clear()
        {
                if (!empty()) {
                        unlink_nodes(
                                last_, traits_type::next_node(last_), true);
                }
        }

        /// \brief Get the list's allocator
        allocator_type get_allocator() const { return *this; }

        /**
         * \brief Set list contents to singular value
         *
         * Set the list's contents to exactly \c count nodes each
         * containing a copy of \c value. The copy assignment operator
         * of \c value_type is used to copy and overwrite \c value to
         * existing nodes up to \c count, erasing nodes from the end of
         * the list as necessary. If the list is smaller than \c count
         * then the list's allocator to used create the required number
         * of new nodes then the copy constructor of \c value_type is
         * used to initialise each new node with a copy of \c value.
         *
         * \param [in] count
         *      resulting list size
         * \param [in] value
         *      value to be copied to all nodes
         */
        void
        assign(
                size_type         count,
                const value_type &value
        )
        {
                auto i = before_begin(), j = last();
                for (; (i != j) && count; --count) {
                        *(++i) = value;
                }
                if (i != j) {
                        erase_after(i, ++j);
                }
                while (count--) {
                        emplace_back(value);
                }
        }

        /**
         * \brief Set list contents from another container
         */
        template <typename InIter> void
        assign(
                InIter first,
                InIter last
        )
        {
                auto i = before_begin(), j = this->last();
                for (; (i != j) && (first != last); ++first) {
                        ++i;
                        *i = *first;
                }
                if (i != j) {
                        erase_after(i, ++j);
                }
                for (; first != last; ++first) {
                        emplace_back(*first);
                }
        }

        /**
         * \brief Set list contents from initializer list
         */
        void assign(std::initializer_list<value_type> l)
                { assign(l.begin(), l.end()); }

        iterator
        insert_after(
                const_iterator pos,
                node_ptr_type  node
        )
        {
                if (!empty()) {
                        auto prev = pos.pos_;

                        if (!prev) {  // insert at beginning
                                prev = last_;
                        }

                        auto old_next = traits_type::next_node(prev);
                        traits_type::set_next_node(prev, node);

                        if (pos.pos_ == last_) {
                                last_ = node;
                        }

                        traits_type::set_next_node(node, old_next);
                } else {
                        last_ = node;
                        traits_type::set_next_node(node, node);
                }

                return iterator(&last_, node);
        }

        iterator insert_after(const_iterator pos, const value_type &value)
                { return insert_after(pos, make_node(value)); }

        iterator insert_after(const_iterator pos, value_type &&value)
                { return insert_after(pos, make_node(std::move(value))); }

        iterator
        insert_after(
                const_iterator    pos,
                size_type         count,
                const value_type &value
        )
        {
                iterator i(pos.last_, pos.pos_);
                while (count--) {
                        i = insert_after(i, make_node(value));
                }
                return i;
        }

        template <typename InIter> iterator
        insert_after(
                const_iterator pos,
                InIter         first,
                InIter         last
        )
        {
                iterator i(pos.last_, pos.pos_);
                for (; first != last; ++first) {
                        i = insert_after(i, make_node(*first));
                }
                return i;
        }

        iterator insert_after(const_iterator pos,
                              std::initializer_list<value_type> &l)
                { return insert_after(pos, l.begin(), l.end()); }

        /**
         * \brief Prepend pre-allocated node
         *
         * Insert the \c node allocated by the caller at the beginning
         * of the list. Ownership of \c node is transferred from the
         * caller to \c *this. \c node must be allocated using the same
         * underlying mechanism as the list's allocator or undefined
         * behaviour will result.
         *
         * Equivalent to calling <tt>insert_after(before_begin(), node);
         * </tt>
         *
         * \param [in] node
         *      pointer to the node being prepended
         * \see
         *      push_back(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         */
        void push_front(node_ptr_type node)
                { insert_after(before_begin(), node); }

        /**
         * \brief Allocate and prepend node
         *
         * Allocate a new node using the list's allocator, initialising
         * it with a copy of \c value using the copy constructor of
         * \c value_type, then insert the new node at the beginning of
         * the list.
         *
         * Equivalent to calling <tt>insert_after(before_begin(), node);
         * </tt>
         *
         * \param [in] value
         *      value to be copied and prepended
         * \see
         *      push_back(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         */
        void push_front(const value_type &value)
                { insert_after(before_begin(), value); }

        /**
         * \brief Allocate and prepend node
         *
         * Allocate a new node using the list's allocator, moving the
         * contents of \c value into it using the move constructor of
         * \c value_type, then insert the new node at the beginning of
         * the list.
         *
         * Equivalent to calling <tt>insert_after(before_begin(),
         * std::move(node));</tt>
         *
         * \param [in,out] value
         *      value to be prepended
         * \see
         *      push_back(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         */
        void push_front(value_type &&value)
                { insert_after(before_begin(), std::move(value)); }

        /**
         * \brief Append pre-allocated node
         *
         * Insert the \c node allocated by the caller at the end of the
         * list. Ownership of \c node is transferred from the caller to
         * \c *this. \c node must be allocated using the same underlying
         * mechanism as the list's allocator or undefined behaviour will
         * result.
         *
         * Equivalent to calling <tt>insert_after(last(), node);</tt>
         *
         * \param [in] node
         *      pointer to the node being appended
         * \see
         *      push_front(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         *      
         */
        void push_back(node_ptr_type node) { insert_after(last(), node); }

        /**
         * \brief Allocate and append node
         *
         * Allocate a new node using the list's allocator, initialising
         * it with a copy of \c value using the copy constructor of
         * \c value_type, then insert the new node at the end of
         * the list.
         *
         * Equivalent to calling <tt>insert_after(last(), node);</tt>
         *
         * \param [in] value
         *      value to be copied and appended
         * \see
         *      push_front(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         */
        void push_back(const value_type &value) { insert_after(last(), value); }

        /**
         * \brief Allocate and append node
         *
         * Allocate a new node using the list's allocator, moving the
         * contents of \c value into it using the move constructor of
         * \c value_type, then insert the new node at the end of the
         * list.
         *
         * Equivalent to calling <tt>insert_after(last(),
         * std::move(node));</tt>
         *
         * \param [in,out] value
         *      value to be appended
         * \see
         *      push_front(), insert_after(), emplace_after(),
         *      emplace_front(), emplace_back()
         */
        void push_back(value_type &&value)
                { insert_after(last(), std::move(value)); }

        /**
         * \brief Allocate and insert node with in-situ value
         *      construction
         *
         * Allocate a new node using the list's allocator, initialise it
         * using a value_type constructor chosen according to \c args
         * and insert the node immediately after \c pos.
         *
         * \param [in] pos
         *      position of the node before the insertion point (use
         *      before_begin() to insert at the beginning, last() to
         *      insert at the end)
         * \param args
         *      argument(s) forwarded to value constructor
         * \return
         *      position of the inserted node
         * \see
         *      emplace_front(), emplace_back(), insert_after(),
         *      push_front(), push_back(), before_begin(), last()
         */
        template <typename ...Args> iterator
        emplace_after(
                const_iterator      pos,
                Args           &&...args
        )
        {
                return insert_after(pos,
                                    make_node(std::forward<Args>(args)...));
        }

        /**
         * \brief Allocate and prepend node with in-situ value
         *      construction
         *
         * Allocate a new node using the list's allocator, initialise it
         * using a value_type constructor chosen according to \c args
         * and insert the node at the beginning of the list.
         *
         * Equivalent to calling <tt>emplace_after(before_begin(),
         * std::forward<Args>(args)...);</tt>
         *
         * \param args
         *      argument(s) forwarded to value constructor
         * \return
         *      position of the inserted node
         * \see
         *      emplace_after(), emplace_back(), insert_after(),
         *      push_front(), push_back()
         */
        template <typename ... Args> iterator emplace_front(Args &&...args)
                { return emplace_after(before_begin(),
                                       std::forward<Args>(args)...); }

        /**
         * \brief Allocate and append node with in-situ value
         *      construction
         *
         * Allocate a new node using the list's allocator, initialise it
         * using a value_type constructor chosen according to \c args
         * and insert the node at the end of the list.
         *
         * Equivalent to calling <tt>emplace_after(last(),
         * std::forward<Args>(args)...);</tt>
         *
         * \param args
         *      argument(s) forwarded to value constructor
         * \return
         *      position of the inserted node
         * \see
         *      emplace_after(), emplace_front(), insert_after(),
         *      push_front(), push_back()
         */
        template <typename ... Args> iterator emplace_back(Args &&...args)
                { return emplace_after(last(), std::forward<Args>(args)...); }

        /**
         * \brief Resize list
         *
         * Resize the list to exactly \c count nodes, appending default-
         * constructed nodes or erasing nodes from the end of the list
         * as appropriate until the list is the requested size.
         *
         * \param [in] count
         *      requested list size
         */
        void resize(size_type count) { resize(count, {}); }

        /**
         * \brief Resize list
         *
         * Resize the list to exactly \c count nodes, appending copies
         * of \c value (using the copy constructor of \c node_type) to
         * the end of the list or erasing nodes from the end of the list
         * as appropriate until the list is the requested size.
         *
         * \param [in] count
         *      requested list size
         * \param [in] value
         *      value to be copied when adding extra nodes
         */
        void
        resize(
                size_type         count,
                const value_type &value
        )
        {
                auto i = before_begin(), j = last();

                for (; count && (i != j); --count) {
                        ++i;
                }

                if (i != j) {
                        erase_after(i, ++j);
                } else {
                        while (count--) {
                                emplace_back(value);
                        }
                }
        }

        /**
         * \brief Remove and deallocate single node
         *
         * Unlink the node immediately after that indicated by \c pos
         * then destruct it and free its memory using the list's
         * allocator.
         *
         * \param [in] pos
         *      position of the node before the one to be erased (or
         *      \c before_begin() to remove the first node)
         * \return
         *      position of the node after the erased node, or \c end()
         *      if the last node was erased
         */
        iterator erase_after(const_iterator pos)
                { return erase_after(pos, std::next(pos, 2)); }

        /**
         * \brief Remove and deallocate range of nodes
         *
         * Unlink the range of nodes between \c first and \c last,
         * not including \c first and \c last, then destruct each node
         * in the range and free their memory using the list's
         * allocator. If <tt>((++first) == last)</tt> then this function
         * has no effect.
         *
         * Calling <tt>erase_after(before_begin(), end())</tt> is
         * equivalent to <tt>clear()</tt>.
         *
         * \param [in] first
         *      position of the node before the first one to be erased (or
         *      \c before_begin() to remove the first node)
         * \param [in] last
         *      position of the node after the last one to be erased (or
         *      \c end() to remove the last node)
         * \return
         *      iterator set to same position as \c last
         * \see
         *      pop_front(), detach_after(), detach_front(), clear()
         */
        iterator
        erase_after(
                const_iterator first,
                const_iterator last
        )
        {
                iterator result(last.last_, last.pos_);

                if (empty() || (std::next(first) == last)) {
                        return result;
                }

                auto before = first.pos_, after = last.pos_;

                if (!before) {
                        before = last_;
                }
                if (!after) {
                        after = traits_type::next_node(last_);
                }

                unlink_nodes(before, after, true);
                return result;
        }

        /**
         * \brief Unlink and deallocate first node
         *
         * Unlink the first node in the list then destruct it and free
         * its memory using the list's allocator.
         *
         * \see
         *      erase_after(), detach_after(), detach_front()
         */
        void pop_front() { erase_after(before_begin()); }

        /**
         * \brief Detach single node given its pointer
         *
         * Unlinks \c node from the list, resets its next pointer to
         * \c nullptr and returns it without de-allocating any memory.
         * Ownership of \c node is transferred to the caller.
         * Complexity is linear relative to the position of \c node from
         * the beginning of the list. If \c node does not exist in the
         * list then this function has no effect.
         *
         * \param [in] node
         *      pointer to the node to be detached
         * \return
         *      \c node if it exists in the list, \c nullptr otherwise
         */
        node_ptr_type
        detach(
                node_ptr_type node
        )
        {
                for (auto i = before_begin(); i != last(); ++i) {
                        if (std::next(i).pos_ == node) {
                                return detach_after(i);
                        }
                }
                return nullptr;
        }

        /**
         * \brief Detach single node
         *
         * The node immediately after the one referred to by \c pos is
         * unlinked without de-allocating and its next pointer reset to
         * \c nullptr. A pointer to the detached node is returned,
         * ownership of which is transferred to the caller. Complexity
         * is constant time. If \c pos refers to a node outside \c *this
         * then the result is undefined.
         *
         * \param [in] pos
         *      iterator referencing the node before the target
         * \return
         *      pointer to the detached node
         */
        node_ptr_type detach_after(const_iterator pos)
                { return detach_after(pos, std::next(pos, 2)); }

        /**
         * \brief Detach range of nodes
         *
         * The range of nodes between \c first and \c last (exclusive of
         * both \c first and \c last) is unlinked and the next pointer
         * of the final node in the range is reset to \c nullptr.
         * A pointer to the first node in the detached range is
         * returned, ownership of all the detached nodes is transferred
         * to the caller. Complexity is linear relative to the number of
         * nodes in the range. If either \c first or \c last refer to
         * nodes outside \c *this then the results are undefined.
         *
         * \param [in] first
         *      iterator referencing the node before the first to be
         *      detached
         * \param [in] last
         *      iterator referencing the node after the last to be
         *      detached
         * \return
         *      pointer to the first detached node or \c nullptr if
         *      either \c *this is empty or <tt>[first..last]</tt>
         *      denotes an empty sequence
         */
        node_ptr_type
        detach_after(
                const_iterator first,
                const_iterator last
        )
        {
                if (std::next(first) == last) {
                        return nullptr;
                }

                auto before = first.pos_, after = last.pos_;

                if (!before) {
                        before = last_;
                }
                if (!after) {
                        after = traits_type::next_node(last_);
                }

                return unlink_nodes(before, after, false);
                
        }

        /**
         * \brief Detach first node
         *
         * The first node in the list is unlinked and its next pointer
         * reset to \c nullptr. A pointer to the detached node is
         * returned, ownership of which is transferred to the caller.
         * Complexity is constant time.
         *
         * This function is equivalent to <tt>detach(before_begin());</tt>.
         *
         * \return
         *      pointer to the detached node or \c nullptr if \c *this
         *      is empty
         */
        node_ptr_type detach_front() { return detach_after(before_begin()); }

        /**
         * \brief Swap contents of two lists
         *
         * Swap the contents of \c *this with those of \c other. If the
         * allocator type used is such that <tt>(allocator_traits::
         * propagate_on_container_swap::value == true)</tt> then the
         * allocator objects of \c *this and \c other are also swapped.
         *
         * \param [in,out] other
         *      list to swap contents with
         */
        void
        swap(
                this_type &other
        )
        {
                std::swap(last_, other.last_);
                if (allocator_traits::propagate_on_container_swap::value) {
                        std::swap(alloc_ref(), other.alloc_ref());
                }
        }

        void merge(this_type &other) { merge(other, std::less<value_type>()); }
        void merge(this_type &&other) { merge(other, std::less<value_type>()); }

        template <typename Compare> void
        merge(
                this_type &other,
                Compare    comp
        )
        {
                if (&other == this) {
                        return;
                }

                auto i = before_begin();

                while (!other.empty()) {
                        if (i == last()) {
                                splice_after(i, other);
                        } else {
                                if (comp(*other.begin(), *std::next(i))) {
                                        splice_after(i, other,
                                                     other.before_begin());
                                }
                                ++i;
                        }
                }
        }

        template <typename Compare> void merge(this_type &&other, Compare comp)
                { merge(other, comp); }

        void
        splice_after(
                const_iterator  pos,
                this_type      &other
        )
        {
                if (!other.empty()) {
                        if (empty()) {
                                last_ = other.last_;
                        } else {
                                auto pos_node = pos.pos_;
                                if (!pos_node) {
                                        pos_node = last_;
                                } else if (pos_node == last_) {
                                        last_ = other.last_;
                                }

                                auto first_spliced
                                        = traits_type::next_node(other.last_);

                                traits_type::set_next_node(
                                        other.last_,
                                        traits_type::next_node(pos_node));

                                traits_type::set_next_node(
                                        pos_node, first_spliced);
                        }
                        other.last_ = nullptr;
                }
        }

        void splice_after(const_iterator pos, this_type &&other)
                { splice_after(pos, other); }

        void
        splice_after(
                const_iterator  pos,
                this_type      &other,
                const_iterator  i
        )
        {
                auto i_node = i.pos_;
                if (!i_node) {
                        i_node = other.last_;
                }

                auto target = traits_type::next_node(i_node);

                if (i_node == target) {
                        other.last_ = nullptr;  // last item in list
                } else {
                        traits_type::set_next_node(
                                i_node, traits_type::next_node(target));

                        if (target == other.last_) {
                                other.last_ = i_node;
                        }
                }

                if (empty()) {
                        last_ = target;
                        traits_type::set_next_node(target, last_);
                } else {
                        auto pos_node = pos.pos_;
                        if (!pos_node) {
                                pos_node = last_;
                        } else if (pos_node == last_) {
                                last_ = target;
                        }

                        traits_type::set_next_node(
                                target, traits_type::next_node(pos_node));

                        traits_type::set_next_node(pos_node, target);
                }
        }

        void splice_after(const_iterator pos, this_type &&other,
                          const_iterator i)
                { splice_after(pos, other, i); }

        void
        splice_after(
                const_iterator  pos,
                this_type      &other,
                const_iterator  first,
                const_iterator  last
        )
        {
                for (; std::next(first) != last; ++pos) {
                        splice_after(pos, other, first);
                }
        }

        void splice_after(const_iterator pos, this_type &&other,
                          const_iterator first, const_iterator last)
                { splice_after(pos, other, first, last); }

        void
        reverse()
        {
                auto dest = last();
                while (dest != begin()) {
                        splice_after(dest, *this, before_begin());
                }
        }

        void unique() { unique(std::equal_to<value_type>()); }

        template <typename BinaryPredicate> void
        unique(
                BinaryPredicate pred
        )
        {
                iterator i = begin();
                while (i != last()) {
                        if (pred(*i, *std::next(i))) {
                                erase_after(i);
                        } else {
                                ++i;
                        }
                }
        }

        void sort() { sort(std::less<value_type>()); }

        template <typename BinaryPredicate> void
        sort(
                BinaryPredicate pred
        )
        {
                /* derived from "Bottom-up implementation using lists"
                   at https://en.wikipedia.org/wiki/Merge_sort */
                enum { N_SUBLISTS = 32U };
                this_type sublists[N_SUBLISTS];
                size_type i;

                // merge nodes from *this into sublists
                while (!empty()) {
                        this_type tmp;
                        tmp.splice_after(tmp.before_begin(),
                                         *this, before_begin());
                        i = 0;
                        for (; (i < N_SUBLISTS) && !sublists[i].empty(); ++i) {
                                tmp.merge(sublists[i], pred);
                        }
                        if (i >= N_SUBLISTS) {
                                i = N_SUBLISTS - 1;
                        }
                        sublists[i].splice_after(sublists[i].before_begin(),
                                                 tmp);
                }

                // merge sublists back into *this
                for (auto sub: sublists) {
                        merge(sub, pred);
                }
        }

        void
        remove(
                node_ptr_type node
        )
        {
                for (auto i = before_begin(); i != last(); ++i) {
                        if (node == std::next(i).pos_) {
                                erase_after(i);
                                i = last();
                        }
                }
        }

        void remove(const value_type &value)
                { remove_if([&value](const value_type &value2)
                            { return value2 == value; }); }

        template <typename UnaryPredicate> void
        remove_if(
                UnaryPredicate p
        )
        {
                for (auto i = before_begin(); i != last();) {
                        if (p(*std::next(i))) {
                                erase_after(i);
                        } else {
                                ++i;
                        }
                }
        }

        this_type &
        operator=(
                const this_type &other
        )
        {
                if (&other != this) {
                        if (allocator_traits::
                                   propagate_on_container_copy_assignment::
                                   value) {
                                if (alloc_ref() != other.alloc_ref()) {
                                        clear();
                                }
                                alloc_ref() = other.alloc_ref();
                        }
                        assign(other.begin(), other.end());
                }
                return *this;
        }

        this_type &
        operator=(
                this_type &&other
        )
        {
                if (&other != this) {
                        if (allocator_traits::
                                   propagate_on_container_move_assignment::
                                   value) {
                                alloc_ref() = other.alloc_ref();
                        } else if (alloc_ref() != other.alloc_ref()) {
                                auto i = before_begin(),
                                     j = last(),
                                     p = other.before_begin(),
                                     q = other.last();

                                while ((i != j) && (p != q)) {
                                        ++i;
                                        ++p;
                                        *i = std::move(*p);
                                }
                                if (i != j) {
                                        erase_after(i, ++j);
                                }
                                for (++p, ++q; p != q; ++p) {
                                        emplace_back(std::move(*p));
                                }
                                return *this;
                        }
                        clear();
                        swap(other);
                }
                return *this;
        }

        this_type &
        operator=(
                std::initializer_list<value_type> l
        )
        {
                clear();
                insert_after(before_begin(), l);
                return *this;
        }

protected:
        node_ptr_type first_node() const
                { return last_ ? traits_type::next_node(last_) : nullptr; }

        template <typename ...Args> node_ptr_type
        make_node(
                Args &&...args
        )
        {
                auto node = static_cast<node_type *>(alloc_ref().allocate(1));
                
                try {
                        alloc_ref().construct(node,
                                              std::forward<Args>(args)...);
                } catch (...) {
                        alloc_ref().deallocate(node, 1);
                        throw;
                }

                return node;
        }

        /**
         * \brief Unlink a range of nodes
         *
         * Unlinks a continuous range of nodes, optionally destroying
         * each node via a call to <tt>traits_type::destroy_node()</tt>.
         * If \c before immediately precedes \c after (i.e. the range is
         * empty) then all nodes in the list are unlinked, returning
         * \c nullptr when <tt>(destroy == true)</tt>. When <tt>(destroy
         * == true)</tt> and a subset of the list is unlinked, \c after
         * is returned. Otherwise when <tt>(destroy == false)</tt> the
         * next pointer of the final node of the target range is reset
         * to \c nullptr and a pointer to the first node in the unlinked
         * range is returned; ownership of these nodes is transferred to
         * the caller.
         *
         * This function is used to implement the functions clear(),
         * erase_after() and detach_after().
         *
         * \param [in] before
         *      pointer to the node before the target range
         * \param [in] after
         *      pointer to the node after the target range (or the node
         *      immediately following \c after) to unlink all nodes
         * \param [in] destroy
         *      if true then invoke <tt>traits_type::destroy_node()</tt>
         *      on each node in the target range
         * \return
         *      \li \c nullptr when <tt>(destroy == true)</tt> and an
         *              empty range is specified
         *      \li \c after when <tt>(destroy == true)</tt> and a
         *              nonempty range is specified
         *      \li pointer to first node in the unlinked range when
         *              <tt>(destroy == false)</tt>
         * \see
         *      clear(), erase_after(), detach_after()
         */
        node_ptr_type
        unlink_nodes(
                node_ptr_type before,
                node_ptr_type after,
                bool          destroy
        )
        {
                node_ptr_type next   = traits_type::next_node(before),
                              result = next;

                if (next == after) {  // removing all nodes
                        last_ = {};
                        if (destroy) {
                                result = {};
                        }
                } else {
                        traits_type::set_next_node(before, after);
                        if (destroy) {
                                result = after;
                        }
                }

                do {
                        auto curr = next;
                        next = traits_type::next_node(curr);
                        if (last_ == curr) {
                                last_ = before;
                        }
                        if (destroy) {
                                traits_type::destroy_node(alloc_ref(), curr);
                        } else if (next == after) {
                                traits_type::set_next_node(curr, {});
                        }
                } while (next != after);

                return result;
        }

        allocator_type &alloc_ref()             { return *this; }
        const allocator_type &alloc_ref() const { return *this; }

private:
        node_ptr_type last_ = nullptr;
};

//--------------------------------------

template <typename T, typename Traits> bool
operator==(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        auto i_a = a.begin(), end_a = a.end(),
             i_b = b.begin(), end_b = b.end();

        for (; (i_a != end_a) && (i_b != end_b); ++i_a, ++i_b) {
                if (*i_a == *i_b) {
                        ;
                } else {
                        return false;
                }
        }

        return (i_a == end_a) && (i_b == end_b);
}

//--------------------------------------

template <typename T, typename Traits> bool
operator!=(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        return !(a == b);
}

//--------------------------------------

template <typename T, typename Traits> bool
operator<(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        return std::lexicographical_compare(a.begin(), a.end(),
                                            b.begin(), b.end());
}

//--------------------------------------

template <typename T, typename Traits> bool
operator<=(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        return !std::lexicographical_compare(b.begin(), b.end(),
                                             a.begin(), a.end());
}

//--------------------------------------

template <typename T, typename Traits> bool
operator>(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        return std::lexicographical_compare(b.begin(), b.end(),
                                            a.begin(), a.end());
}

//--------------------------------------

template <typename T, typename Traits> bool
operator>=(
        const intrusive_circ_fwd_list<T, Traits> &a,
        const intrusive_circ_fwd_list<T, Traits> &b
)
{
        return !std::lexicographical_compare(a.begin(), a.end(),
                                             b.begin(), b.end());
}

//--------------------------------------

template <typename T, typename Alloc = std::allocator<T>>
class circ_fwd_list
{
        struct node_type
        {
                using this_type = node_type;

                T value_;
                this_type *next_ = nullptr;

                node_type(const T &value) : value_(value) {}
                node_type(T &&value) : value_(std::move(value)) {}
                template <typename ...Args> node_type(Args &&...args) :
                        value_(std::forward<Args>(args)...) {}

                this_type *next() const { return next_; }
                void next(this_type *n) { next_ = n; }

                operator T &() { return value_; }
                operator const T &() const { return value_; }
        };

        struct list_traits : intrusive_list_traits<node_type>
        {
                using base_type = intrusive_list_traits<node_type>;
                using value_type = T;
                using reference = value_type &;
                using const_reference = const value_type &;
                using pointer = value_type *;
                using const_pointer = const value_type *;
                using allocator_traits = std::allocator_traits<Alloc>;
                using allocator_type = typename allocator_traits::
                                              template rebind_alloc<node_type>;

                static T *get_value_ptr(typename base_type::node_ptr_type node)
                        { return &node->value_; }
        };

        using inner_type = intrusive_circ_fwd_list<node_type, list_traits>;

public:
        using this_type = circ_fwd_list;
        using value_type = typename list_traits::value_type;
        using reference = typename list_traits::reference;
        using const_reference = typename list_traits::const_reference;
        using pointer = typename list_traits::pointer;
        using const_pointer = typename list_traits::const_pointer;
        using size_type = typename list_traits::size_type;
        using difference_type = typename list_traits::difference_type;
        using allocator_type = Alloc;
        using iterator = typename inner_type::iterator;
        using reverse_iterator = typename inner_type::reverse_iterator;
        using const_iterator = typename inner_type::const_iterator;
        using const_reverse_iterator
                = typename inner_type::const_reverse_iterator;

        circ_fwd_list() {} /* = default; */
                /* NB: GCC 4.x gets confused by '= default' used with
                   default constructor */

        explicit circ_fwd_list(const allocator_type &alloc) :
                list_(alloc) {}

        circ_fwd_list(
                size_type             count,
                const value_type     &value,
                const allocator_type &alloc = {}
        ) :
                circ_fwd_list(alloc)
        {
                insert_after(before_begin(), count, value);
        }

        circ_fwd_list(
                size_type             count,
                const allocator_type &alloc = {}
        ) :
                circ_fwd_list(alloc)
        {
                insert_after(before_begin(), count, {});
        }

        template <typename InIter>
        circ_fwd_list(
                InIter                first,
                InIter                last,
                const allocator_type &alloc = {}
        ) :
                list_(first, last, alloc)
        {
        }

        circ_fwd_list(const this_type &other) : list_(other.list_) {}

        circ_fwd_list(this_type &&other) = default;

        circ_fwd_list(this_type &&other, const allocator_type &alloc) :
                list_(std::move(other.list_), alloc) {}

        circ_fwd_list(
                std::initializer_list<value_type>  l,
                const allocator_type              &alloc = {}
        ) :
                circ_fwd_list(alloc)
        {
                insert_after(before_begin(), l);
        }

        ~circ_fwd_list() { clear(); }

        reference front()             { return *begin(); }
        const_reference front() const { return *begin(); }
        reference back()              { return *last(); }
        const_reference back() const  { return *last(); }

        iterator before_begin()              { return end(); }
        const_iterator before_begin() const  { return cend(); }
        const_iterator cbefore_begin() const { return cend(); }

        iterator begin()              { return iterator(list_.begin()); }
        const_iterator begin() const  { return cbegin(); }
        const_iterator cbegin() const { return const_iterator(list_.begin()); }

        iterator last()               { return iterator(list_.last()); }
        const_iterator last() const   { return clast(); }
        const_iterator clast() const  { return const_iterator(list_.last()); }

        iterator end()                { return iterator(list_.end()); }
        const_iterator end() const    { return cend(); }
        const_iterator cend() const   { return const_iterator(list_.end()); }

        bool empty() const            { return list_.empty(); }
        size_type size() const        { return list_.size(); }
        size_type max_size() const    { return list_.max_size(); }
        void clear()                  { list_.clear(); }

        allocator_type get_allocator() const
                { return allocator_type(list_.get_allocator()); }

        void assign(size_type count, const value_type &value)
                { list_.assign(count, value); }

        template <typename InIter> void assign(InIter first, InIter last)
                { list_.assign(first, last); }

        template <typename InIter> void
                assign(std::initializer_list<value_type> l) { list_.assign(l); }

        iterator insert_after(const_iterator pos, const value_type &value)
                { return list_.insert_after(pos, value); }

        iterator insert_after(const_iterator pos, value_type &&value)
                { return list_.insert_after(pos, std::move(value)); }

        iterator insert_after(const_iterator pos, size_type count,
                              const value_type &value)
                { return list_.insert_after(pos, count, value); }

        template <typename InIter> iterator
                insert_after(const_iterator pos, InIter first, InIter last)
                        { return list_.insert_after(pos, first, last); }

        iterator insert_after(const_iterator pos,
                              std::initializer_list<value_type> &l)
                { return list_.insert_after(pos, l.begin(), l.end()); }

        void push_front(const value_type &value)
                { list_.insert_after(before_begin(), value); }

        void push_front(value_type &&value)
                { list_.insert_after(before_begin(), std::move(value)); }

        void push_back(const value_type &value)
                { list_.insert_after(last(), value); }

        void push_back(value_type &&value)
                { list_.insert_after(last(), std::move(value)); }

        template <typename ...Args> iterator
                emplace_after(const_iterator pos, Args &&...args)
                        { return list_.insert_after(
                                        pos, std::forward<Args>(args)...); }

        template <typename ... Args> iterator emplace_front(Args &&...args)
                { return list_.emplace_after(before_begin(),
                                             std::forward<Args>(args)...); }

        template <typename ... Args> iterator emplace_back(Args &&...args)
                { return list_.emplace_after(last(),
                                             std::forward<Args>(args)...); }

        void resize(size_type count) { list_.resize(count); }

        void resize(size_type count, const value_type &value)
                { list_.resize(count, value); }

        iterator erase_after(const_iterator pos)
                { return list_.erase_after(pos); }

        iterator erase_after(const_iterator first, const_iterator last)
                { return list_.erase_after(first, last); }

        void pop_front() { list_.pop_front(); }

        void swap(this_type &other) { list_.swap(other.list_); }

        void merge(this_type &other) { list_.merge(other.list_); }
        void merge(this_type &&other) { list_.merge(std::move(other.list_)); }

        template <typename Compare> void merge(this_type &other, Compare comp)
                { list_.merge(other.list_, comp); }

        template <typename Compare> void merge(this_type &&other, Compare comp)
                { list_.merge(std::move(other.list_), comp); }

        void splice_after(const_iterator pos, this_type &other)
                { list_.splice_after(pos, other.list_); }

        void splice_after(const_iterator pos, this_type &&other)
                { list_.splice_after(pos, std::move(other.list_)); }

        void splice_after(const_iterator pos, this_type &other,
                          const_iterator i)
                { list_.splice_after(pos, other.list_, i); }

        void splice_after(const_iterator pos, this_type &&other,
                          const_iterator i)
                { list_.splice_after(pos, std::move(other.list_), i); }

        void splice_after(const_iterator pos, this_type &other,
                          const_iterator first, const_iterator last)
                { list_.splice_after(pos, other.list_, first, last); }

        void splice_after(const_iterator pos, this_type &&other,
                          const_iterator first, const_iterator last)
                { list_.splice_after(pos, other.list_, first, last); }

        void reverse() { list_.reverse(); }
        void unique() { list_.unique(); }

        template <typename BinaryPredicate> void unique(BinaryPredicate pred)
                { list_.unique(pred); }

        void sort() { list_.sort(); }

        template <typename BinaryPredicate> void sort(BinaryPredicate pred)
                { list_.sort(pred); }

        void remove(const value_type &value)
                { list_.remove_if([&value](const value_type &value2)
                            { return value2 == value; }); }

        template <typename UnaryPredicate> void remove_if(UnaryPredicate p)
                { list_.remove_if(p); }

        this_type &operator=(const this_type &other) = default;
        this_type &operator=(this_type &&other) = default;

        this_type &
        operator=(
                std::initializer_list<value_type> l
        )
        {
                clear();
                insert_after(before_begin(), l);
                return *this;
        }

private:
        inner_type list_;
};

//--------------------------------------

template <typename T, typename Alloc> bool
operator==(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        auto i_a = a.begin(), end_a = a.end(),
             i_b = b.begin(), end_b = b.end();

        for (; (i_a != end_a) && (i_b != end_b); ++i_a, ++i_b) {
                if (*i_a == *i_b) {
                        ;
                } else {
                        return false;
                }
        }

        return (i_a == end_a) && (i_b == end_b);
}

//--------------------------------------

template <typename T, typename Alloc> bool
operator!=(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        return !(a == b);
}

//--------------------------------------

template <typename T, typename Alloc> bool
operator<(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        return std::lexicographical_compare(a.begin(), a.end(),
                                            b.begin(), b.end());
}

//--------------------------------------

template <typename T, typename Alloc> bool
operator<=(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        return !std::lexicographical_compare(b.begin(), b.end(),
                                             a.begin(), a.end());
}

//--------------------------------------

template <typename T, typename Alloc> bool
operator>(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        return std::lexicographical_compare(b.begin(), b.end(),
                                            a.begin(), a.end());
}

//--------------------------------------

template <typename T, typename Alloc> bool
operator>=(
        const circ_fwd_list<T, Alloc> &a,
        const circ_fwd_list<T, Alloc> &b
)
{
        return !std::lexicographical_compare(a.begin(), a.end(),
                                             b.begin(), b.end());
}


} // namespace wr


#endif // !WRUTIL_CIRC_FWD_LIST_H
