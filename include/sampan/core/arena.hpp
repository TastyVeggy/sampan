#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace sampan::core {

// monotonic allocator to make life easier
class Arena {
public:
  explicit Arena(std::size_t initial_capacity = 4096);
  ~Arena() noexcept;

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  Arena(Arena &&) noexcept = default;
  Arena &operator=(Arena &&) noexcept = default;

  [[nodiscard]] void *allocate(std::size_t size, std::size_t alignment);

  template <typename T, typename... Args>
    requires std::constructible_from<T, Args...> && std::destructible<T>
  [[nodiscard]] T *create(Args &&...args) {
    void *storage = allocate(sizeof(T), alignof(T));
    T *object = std::construct_at(static_cast<T *>(storage),
                                  std::forward<Args>(args)...);

    if constexpr (!std::is_trivially_destructible_v<T>) {
      destructors_.push_back({object, [](void *pointer) noexcept {
                                std::destroy_at(static_cast<T *>(pointer));
                              }});
    }
    return object;
  }

  void reset() noexcept;

  [[nodiscard]] std::size_t bytes_used() const noexcept {
    return bytes_used_;
  }

private:
  struct Block {
    struct Deleter {
      std::size_t alignment{alignof(std::max_align_t)};
      void operator()(std::byte *pointer) const noexcept;
    };

    std::unique_ptr<std::byte[], Deleter> storage;
    std::size_t capacity{0};
    std::size_t offset{0};
    std::size_t alignment{alignof(std::max_align_t)};
  };

  struct Destructor {
    void *object;
    void (*destroy)(void *) noexcept;
  };

  struct BlockRequest {
    std::size_t minimum_capacity;
    std::size_t minimum_alignment;
  };

  Block &add_block(BlockRequest request);

  std::vector<Block> blocks_;
  std::vector<Destructor> destructors_;
  std::size_t initial_capacity_;
  std::size_t bytes_used_{0};
};

} // namespace sampan::core
