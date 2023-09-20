#include "shared.h"

#include <stdexcept>

#include "gtest/gtest.h"
namespace {

struct copyable_s_ {
  copyable_s_() = default;
  copyable_s_(const copyable_s_&) = default;
  //copyable_s_  operator=(      copyable_s_ ) = default;
  copyable_s_& operator=(const copyable_s_&) = default;

  copyable_s_(copyable_s_&&) = delete;
  copyable_s_& operator=(copyable_s_&&) = delete;
};

struct moveable_s_ {
  moveable_s_() = default;
  moveable_s_(const moveable_s_&) = delete;
  //moveable_s_  operator=(      moveable_s_ ) = delete;
  moveable_s_& operator=(const moveable_s_&) = delete;

  moveable_s_(moveable_s_&&) = default;
  moveable_s_& operator=(moveable_s_&&) = default;
};

template<typename T>
struct special_moveable_s_ {
  T value;
  special_moveable_s_() = default;
  special_moveable_s_(const special_moveable_s_&) = delete;
  special_moveable_s_& operator=(const special_moveable_s_&) = delete;

  special_moveable_s_(special_moveable_s_&&) = default;
  special_moveable_s_& operator=(special_moveable_s_&&) = default;
};

struct copyable_moveable_s_ {};

using namespace marslander;
using namespace std;

TEST(SharedTests, parcel_making_of) {
  copyable_s_ c{};
  moveable_s_ m{};
  copyable_moveable_s_ z{};

  [[maybe_unused]]
  auto p1 = make_parcel(c);
  auto p1_c{p1};

  [[maybe_unused]]
  auto p2 = make_parcel(move(m));
  //auto p2_c{p2}; // error
  auto p2_c{move(p2)}; 

  [[maybe_unused]]
  auto p3 = make_parcel(z);
  auto p3_c{p3};

  [[maybe_unused]]
  auto p4 = make_parcel(move(z));
  auto p4_c{p4};
}

TEST(SharedTests, parcel_throw_on_non_copyable_copy_attempt) {
  ASSERT_THROW({
    auto bad = bind([](const parcel<moveable_s_>& p) { },
      make_parcel(moveable_s_{}));
    auto consumer = [](function<void()>&& f) { f(); };
    consumer(bad);
  },
  logic_error);
}

TEST(SharedTests, parcel_non_copyable_transfer) {
  //consumer(bind(void (*some_func)(moveable_s_&&), moveable_s_{})); // error

  auto consumer = [](function<void()>&& f) { f(); };

  using mv_string = special_moveable_s_<string>;
  auto receiver_const = [](const string& expected, const parcel<mv_string>& p) {
    ASSERT_EQ(expected, p.value().value);
  };

  auto receiver = [](const string& expected, parcel<mv_string>& p) {
    mv_string local{p.unwrap()};
    ASSERT_EQ(expected, local.value);
  };

  string s;
  s = "recieve_const";
  consumer(bind(receiver_const, s, make_parcel(mv_string{s})));

  s = "proxy recieve_const call";
  consumer([&receiver_const, &s, p = make_parcel(mv_string{s})]() { receiver_const(s, p); });

  s = "recieve call";
  consumer(bind(receiver, s, make_parcel(mv_string{s})));
}

} // namespace
