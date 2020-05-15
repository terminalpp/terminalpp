#pragma once

#include "helpers.h"

namespace helpers {

    class Buffer {
    public:

        Buffer(size_t capacity = 8):
            data_{new char[capacity]},
            size_{0},
            capacity_{capacity} {
            ASSERT(capacity > 0);
        }

        Buffer(Buffer && from):
            data_{from.data_},
            size_{from.size_},
            capacity_{from.capacity_} {
            from.data_ = nullptr;
        }

        ~Buffer() {
            delete data_;
        }

        size_t size() const {
            return size_;
        }

        size_t capacity() const {
            return capacity_;
        }

        void clear() {
            size_ = 0;
        }

        char * begin() {
            return data_;
        }

        char const * begin() const {
            return data_;
        }

        char * end() {
            return data_ + size_;
        }

        char const * end() const {
            return data_ + size_;
        }

        Buffer & operator << (char x) {
            ASSERT(data_ != nullptr);
            if (size_ == capacity_)
                grow();
            data_[size_++] = x;
            return *this;
        }

        Buffer & operator << (std::string const & str) {
            while (size_ + str.size() > capacity_)
                grow();
            memcpy(data_ + size_, str.c_str(), str.size());
            size_ += str.size();
            return *this;
        }

        char * release() {
            char * result = data_;
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            return result;
        }


    private:

        void grow() {
            char * x = new char[capacity_ * 2];
            memcpy(x, data_, capacity_);
            capacity_ *= 2;
            delete [] data_;
            data_ = x;
        }


        char * data_;
        size_t size_;
        size_t capacity_;

    }; // helpers::Buffer


} // namespace helpers