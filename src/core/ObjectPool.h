// ObjectPool.h - fixed-capacity recycling container. Particle effects are the
// main user: they are spawned and destroyed constantly, so slots are reused
// instead of allocating on every hit.
#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace aa {

template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t capacity = 512) { init(capacity); }

    void init(std::size_t capacity) {
        items_.assign(capacity, T{});
        alive_.assign(capacity, 0);
        free_.clear();
        free_.reserve(capacity);
        for (std::size_t i = capacity; i > 0; --i) free_.push_back(i - 1);
        aliveCount_ = 0;
    }

    // Returns a recycled slot, or nullptr when the pool is exhausted.
    template <typename... Args>
    T* acquire(Args&&... args) {
        if (free_.empty()) return nullptr;
        std::size_t index = free_.back();
        free_.pop_back();
        items_[index] = T(std::forward<Args>(args)...);
        alive_[index] = 1;
        ++aliveCount_;
        return &items_[index];
    }

    void release(std::size_t index) {
        if (index >= items_.size() || !alive_[index]) return;
        alive_[index] = 0;
        free_.push_back(index);
        --aliveCount_;
    }

    // fn(T&) is called for every live slot; returning false from the functor
    // releases that slot afterwards.
    template <typename Fn>
    void forEachAlive(Fn&& fn) {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (!alive_[i]) continue;
            if (!fn(items_[i])) release(i);
        }
    }

    void clear() {
        free_.clear();
        for (std::size_t i = items_.size(); i > 0; --i) {
            alive_[i - 1] = 0;
            free_.push_back(i - 1);
        }
        aliveCount_ = 0;
    }

    std::size_t aliveCount() const { return aliveCount_; }
    std::size_t capacity() const { return items_.size(); }

private:
    std::vector<T> items_;
    std::vector<char> alive_;
    std::vector<std::size_t> free_;
    std::size_t aliveCount_ = 0;
};

}  // namespace aa
