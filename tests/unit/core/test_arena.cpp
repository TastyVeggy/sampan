#include <gtest/gtest.h>

#include <stdexcept>

#include "sampan/core/arena.hpp"

namespace {

struct Tracker {
  explicit Tracker(int &destructions) : destructions_(&destructions) {
  }

  ~Tracker() {
    ++*destructions_;
  }

  int *destructions_;
};

struct alignas(64) OverAligned {
  std::byte byte{};
};

TEST(Arena, HonorsAlignmentAndTracksUsage) {
  sampan::core::Arena arena{16};
  auto *first = static_cast<std::byte *>(arena.allocate(1, alignof(std::byte)));
  auto *aligned = static_cast<std::byte *>(arena.allocate(8, alignof(double)));

  EXPECT_NE(first, nullptr);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(aligned) % alignof(double), 0);
  EXPECT_EQ(arena.bytes_used(), 9);
}

TEST(Arena, SupportsOverAlignedObjects) {
  sampan::core::Arena arena;
  OverAligned *object = arena.create<OverAligned>();

  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(object) % alignof(OverAligned), 0);
}

TEST(Arena, RejectsInvalidConstructionAndAllocations) {
  EXPECT_THROW(static_cast<void>(sampan::core::Arena{0}),
               std::invalid_argument);

  sampan::core::Arena arena;
  EXPECT_THROW(static_cast<void>(arena.allocate(0, 1)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(arena.allocate(1, 3)), std::invalid_argument);
}

TEST(Arena, GrowsWhenCurrentBlockHasInsufficientCapacity) {
  sampan::core::Arena arena{16};
  [[maybe_unused]] void *first = arena.allocate(12, 1);
  [[maybe_unused]] void *second = arena.allocate(12, 1);

  EXPECT_EQ(arena.bytes_used(), 24);
}

TEST(Arena, DestroysObjectsOnReset) {
  sampan::core::Arena arena;
  int destructions = 0;
  [[maybe_unused]] Tracker *first = arena.create<Tracker>(destructions);
  [[maybe_unused]] Tracker *second = arena.create<Tracker>(destructions);

  arena.reset();

  EXPECT_EQ(destructions, 2);
  EXPECT_EQ(arena.bytes_used(), 0);
}

} // namespace
