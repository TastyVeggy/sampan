#include "sampan/core/arena.hpp"
#include <cstddef>
#include <stdexcept>

#include <ranges>

namespace sampan::core {
namespace {

[[nodiscard]] std::size_t align_up(const std::size_t value,
                                   const std::size_t alignment) {
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

} // namespace

Arena::Arena(const std::size_t initial_capacity)
    : initial_capacity_(initial_capacity) {
  if (initial_capacity == 0) {
    throw std::invalid_argument("Arena initial capacity must be positive");
  }
}

Arena::~Arena() noexcept {
  reset();
}

void Arena::Block::Deleter::operator()(
    std::byte *const pointer) const noexcept {
  ::operator delete[](pointer, std::align_val_t{alignment});
}

void *Arena::allocate(const std::size_t size, const std::size_t alignment) {
  if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
    throw std::invalid_argument(
        "Arena allocation requires non-zero power-of-two alignment.");
  }
  // alignment - 1 because starting offset may shift forward by alignment - 1
  // bytes
  auto block = blocks_.empty()
                   ? &add_block({.minimum_capacity = size + alignment - 1,
                                 .minimum_alignment = alignment})
                   : &blocks_.back();

  std::size_t offset = align_up(block->offset, alignment);
  if (block->alignment < alignment || offset > block->capacity ||
      size > block->capacity - offset) {
    block = &add_block({.minimum_capacity = size + alignment - 1,
                        .minimum_alignment = alignment});
    offset = align_up(block->offset, alignment);
  }
  std::byte *result = block->storage.get() + offset;
  block->offset = offset + size;
  bytes_used_ += size;
  return result;
};

void Arena::reset() noexcept {
  for (Destructor &destructor : std::views::reverse(destructors_)) {
    destructor.destroy(destructor.object);
  }
  destructors_.clear();
  blocks_.clear();
  bytes_used_ = 0;
}

Arena::Block &Arena::add_block(const BlockRequest request) {
  const std::size_t previous =
      blocks_.empty() ? initial_capacity_ : blocks_.back().capacity;
  const std::size_t doubled =
      previous > std::numeric_limits<std::size_t>::max() / 2
          ? std::numeric_limits<std::size_t>::max()
          : previous * 2;
  const std::size_t capacity = std::max(request.minimum_capacity, doubled);
  const std::size_t alignment =
      std::max(alignof(std::max_align_t), request.minimum_alignment);

  Block::Deleter deleter{alignment};
  std::unique_ptr<std::byte[], Block::Deleter> storage(
      static_cast<std::byte *>(
          ::operator new[](capacity, std::align_val_t{alignment})),
      deleter);

  return blocks_.emplace_back(std::move(storage), capacity, 0, alignment);
}

} // namespace sampan::core
