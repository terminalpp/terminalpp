#include "buffer.h"

namespace ui {

    void Buffer::create(Size const & size) {
        ASSERT(rows_ == nullptr);
        for (int i = 0; i < size.height(); ++i)
            rows_[i] = new Cell[size.width()];
        size_ = size;
    }

    void Buffer::clear() {
        if (rows_ != nullptr) {
            for (int i = 0; i < size_.height(); ++i)
                delete [] rows_[i];
            delete [] rows_;
            rows_ = nullptr;
            size_ = Size{0,0};
        }
    }


} // namespace ui