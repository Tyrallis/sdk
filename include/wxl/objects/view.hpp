// wxl::ObjectView: a snapshot of the objects of one class that were in view.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef WXL_OBJECTS_VIEW_HPP
#define WXL_OBJECTS_VIEW_HPP

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

#include "wxl/abi.h"

namespace wxl
{
    /**
     * @brief A snapshot of the objects of one class that were in view when it was taken.
     *
     * Holds handles, not objects: an element is built on access, so an object that left between
     * the snapshot and the visit reads as gone rather than as garbage. Iterates in a range-for and
     * with <algorithm>; the iterator yields T by value.
     */
    template <class T>
    class ObjectView
    {
    public:
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = T;

            iterator() = default;
            explicit iterator(const wxl_handle* at) : at_(at) {}

            T         operator*() const { return T(*at_); }
            iterator& operator++()
            {
                ++at_;
                return *this;
            }
            iterator operator++(int)
            {
                iterator before = *this;
                ++at_;
                return before;
            }
            bool operator==(const iterator& other) const { return at_ == other.at_; }
            bool operator!=(const iterator& other) const { return at_ != other.at_; }

        private:
            const wxl_handle* at_ = nullptr;
        };

        using value_type     = T;
        using const_iterator = iterator;

        ObjectView() = default;
        explicit ObjectView(std::vector<wxl_handle> handles) : handles_(std::move(handles)) {}

        iterator begin() const { return iterator(handles_.data()); }
        iterator end() const { return iterator(handles_.data() + handles_.size()); }

        size_t size() const { return handles_.size(); }
        bool   empty() const { return handles_.empty(); }
        T      operator[](size_t index) const { return T(handles_[index]); }

        /// The handles themselves, for the bulk calls.
        const std::vector<wxl_handle>& Handles() const { return handles_; }

        /// The same objects, materialised.
        std::vector<T> ToVector() const { return std::vector<T>(begin(), end()); }

    private:
        std::vector<wxl_handle> handles_;
    };
} // namespace wxl

#endif // WXL_OBJECTS_VIEW_HPP
