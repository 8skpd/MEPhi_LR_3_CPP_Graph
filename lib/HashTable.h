#pragma once

#include <functional>
#include <utility>
#include "DynamicArray.h"
#include "Option.h"

// Хеш-таблица с разрешением коллизий методом цепочек.

template <typename TKey, typename TValue, typename Hasher = std::hash<TKey>>
class HashTable {
private:
    struct KeyValuePair {
        TKey key;
        TValue value;
        
        KeyValuePair() = default;
        KeyValuePair(const TKey& k, const TValue& v) : key(k), value(v) {}
    };

    using Bucket = DynamicArray<KeyValuePair>;

    DynamicArray<Bucket> buckets_;
    size_t count_ = 0;
    Hasher hasher_;

    static constexpr double kLoadFactorGrow = 0.75;
    static constexpr double kLoadFactorShrink = 0.25;

    size_t get_bucket_index(const TKey& key) const {
        return hasher_(key) % buckets_.size();
    }

    long long find_in_bucket(size_t bucket_idx, const TKey& key) const {
        const Bucket& bucket = buckets_[bucket_idx];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].key == key) return static_cast<long long>(i);
        }
        return -1;
    }

    void rehash(size_t new_capacity) {
        DynamicArray<Bucket> new_buckets(new_capacity);
        for (size_t i = 0; i < buckets_.size(); ++i) {
            for (size_t j = 0; j < buckets_[i].size(); ++j) {
                const auto& pair = buckets_[i][j];
                size_t new_idx = hasher_(pair.key) % new_capacity;
                new_buckets[new_idx].push_back(pair);
            }
        }
        buckets_ = std::move(new_buckets);
    }

    void check_resize() {
        const size_t capacity = buckets_.size();
        if (capacity == 0) return;
        const double load = static_cast<double>(count_) / capacity;

        if (load > kLoadFactorGrow) {
            rehash(capacity * 2);
        } else if (capacity > 4 && load < kLoadFactorShrink) {
            rehash(capacity / 2);
        }
    }

public:
    explicit HashTable(size_t initial_capacity = 4)
        : buckets_(initial_capacity) {}

    bool Add(const TKey& key, const TValue& value) {
        if (buckets_.size() == 0) {
            buckets_ = DynamicArray<Bucket>(4);
        }
        size_t idx = get_bucket_index(key);
        if (find_in_bucket(idx, key) != -1) return false;

        buckets_[idx].push_back(KeyValuePair(key, value));
        ++count_;
        check_resize();
        return true;
    }

    Option<TValue> Get(const TKey& key) const {
        if (buckets_.size() == 0) return None<TValue>();
        size_t idx = get_bucket_index(key);
        long long pos = find_in_bucket(idx, key);
        if (pos == -1) return None<TValue>();
        return Some<TValue>(buckets_[idx][static_cast<size_t>(pos)].value);
    }

    void Set(const TKey& key, const TValue& value) {
        if (buckets_.size() == 0) {
            buckets_ = DynamicArray<Bucket>(4);
        }
        size_t idx = get_bucket_index(key);
        long long pos = find_in_bucket(idx, key);
        if (pos != -1) {
            buckets_[idx][static_cast<size_t>(pos)].value = value;
        } else {
            buckets_[idx].push_back(KeyValuePair(key, value));
            ++count_;
            check_resize();
        }
    }

    bool ContainsKey(const TKey& key) const {
        if (buckets_.size() == 0) return false;
        size_t idx = get_bucket_index(key);
        return find_in_bucket(idx, key) != -1;
    }

    bool Remove(const TKey& key) {
        if (buckets_.size() == 0) return false;
        size_t idx = get_bucket_index(key);
        long long pos = find_in_bucket(idx, key);
        if (pos == -1) return false;

        buckets_[idx].erase(static_cast<size_t>(pos));
        --count_;
        check_resize();
        return true;
    }

    size_t GetCount() const { return count_; }
    size_t GetCapacity() const { return buckets_.size(); }

    class Iterator {
    private:
        const HashTable* table_;
        size_t bucket_idx_;
        size_t item_idx_;

        void advance_to_next() {
            while (bucket_idx_ < table_->buckets_.size()) {
                if (item_idx_ < table_->buckets_[bucket_idx_].size()) return;
                ++bucket_idx_;
                item_idx_ = 0;
            }
        }

    public:
        Iterator(const HashTable* t, size_t b, size_t i)
            : table_(t), bucket_idx_(b), item_idx_(i) {
            advance_to_next();
        }

        const KeyValuePair& operator*() const {
            return table_->buckets_[bucket_idx_][item_idx_];
        }

        Iterator& operator++() {
            ++item_idx_;
            advance_to_next();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return bucket_idx_ != other.bucket_idx_ || item_idx_ != other.item_idx_;
        }
    };

    Iterator begin() const { return Iterator(this, 0, 0); }
    Iterator end() const { return Iterator(this, buckets_.size(), 0); }
};