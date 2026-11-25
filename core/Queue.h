#pragma once

#include <utility>
#include "../lib/DynamicArray.h"
#include "../lib/Option.h"

/*
FIFO-очередь на кольцевом буфере поверх DynamicArray.
Все операции амортизированно O(1), память O(N).

Требуется от T: конструктор по умолчанию и присваивание.
*/


template <typename T>
class Queue {
private:
    DynamicArray<T> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;

    void grow() {
        const size_t old_cap = buf_.size();
        const size_t new_cap = (old_cap == 0) ? 4 : old_cap * 2;
        DynamicArray<T> fresh(new_cap);
        for (size_t i = 0; i < count_; ++i) {
            fresh[i] = buf_[(head_ + i) % old_cap];
        }
        buf_ = std::move(fresh);
        head_ = 0;
        tail_ = count_;
    }

public:
    Queue() = default;

    void enqueue(const T& item) {
        if (count_ == buf_.size()) grow();
        buf_[tail_] = item;
        tail_ = (tail_ + 1) % buf_.size();
        ++count_;
    }

    Option<T> dequeue() {
        if (count_ == 0) return None<T>();
        T value = buf_[head_];
        head_ = (head_ + 1) % buf_.size();
        --count_;
        return Some<T>(std::move(value));
    }

    Option<T> front() const {
        if (count_ == 0) return None<T>();
        return Some<T>(buf_[head_]);
    }

    bool   empty() const { return count_ == 0; }
    size_t size()  const { return count_; }
};