#include <ShardedUnorderedConcurrentMap.h>
#include <UnorderedConcurrentMap.h>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

namespace {
  using ::Concurrent::ShardedUnorderedMap;
  using ::Concurrent::UnorderedMap;

  // helper constant for static_asserts in constexpr flow control.
  template <typename...>
  static inline constexpr bool always_false_v = false;

  template <typename map_type>
  static auto initialize_test_map() -> map_type {
    using key_type    = typename map_type::key_type;
    using mapped_type = typename map_type::mapped_type;
    if constexpr (std::is_same_v<key_type, std::string> && std::is_integral_v<mapped_type>) {
      return map_type{
          {"foo", 1},
          {"bar", 2},
          {"baz", 3},
      };
    } else if constexpr (std::is_same_v<key_type, std::string> && std::is_floating_point_v<mapped_type>) {
      return map_type{
          {"foo", 1.1},
          {"bar", 2.1},
          {"baz", 3.1},
      };
    } else if constexpr (std::is_same_v<key_type, std::string> && std::is_same_v<mapped_type, std::string>) {
      return map_type{
          {"foo", "qux"},
          {"bar", "quux"},
          {"baz", "quuux"},
      };
    } else if constexpr (std::is_integral_v<key_type> && std::is_integral_v<mapped_type>) {
      return map_type{
          {1, 1},
          {2, 2},
          {3, 3},
      };
    } else if constexpr (std::is_integral_v<key_type> && std::is_same_v<mapped_type, std::string>) {
      return map_type{
          {1, "foo"},
          {2, "bar"},
          {3, "baz"},
      };
    } else {
      static_assert(always_false_v<key_type, mapped_type>, "Unhandled <key_type, mapped_type> combination.");
    }
  }

  // Common test cases for both
  // ::Concurrent::ShardedUnorderedMap, and
  // ::Concurrent::UnorderedMap.
  template <typename T>
  class CommonConcurrentUnorderedMapTests : public ::testing::Test {};
  class UnshardedConcurrentUnorderedMapTests : public ::testing::Test {};
  class ShardedConcurrentUnorderedMapTests : public ::testing::Test {};

  TYPED_TEST_SUITE_P(CommonConcurrentUnorderedMapTests);
  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, DefaultConstructor) {
    using map_type = TypeParam;
    using key_type = typename map_type::key_type;
    map_type umap;
    constexpr typename map_type::size_type expected_size = 0;
    ASSERT_EQ(expected_size, umap.size());
    ASSERT_TRUE(umap.empty());

    try {
      umap.at(key_type());
      FAIL() << "Expected std::out_of_range.";
    } catch (std::out_of_range const &) {
    } catch (...) {
      FAIL() << "Expected std::out_of_range.";
    }

    try {
      umap[key_type()];
      FAIL() << "Expected std::out_of_range.";
    } catch (std::out_of_range const &) {
    } catch (...) {
      FAIL() << "Expected std::out_of_range.";
    }

    ASSERT_EQ(0, umap.erase(key_type())) << "Expected no elements to be removed.";
    ASSERT_EQ(0, umap.count(key_type())) << "Expected no elements to be found.";
    ASSERT_FALSE(umap.find(key_type())) << "Expected no elements to be found.";

    auto data = umap.data();
    ASSERT_EQ(expected_size, data.size());
    ASSERT_TRUE(data.empty());

    ASSERT_EQ(map_type(), umap);
    ASSERT_FALSE(umap != map_type());
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, CopyConstructor) {
    using map_type = TypeParam;

    auto umap1 = initialize_test_map<map_type>();
    ASSERT_FALSE(umap1.empty());
    auto umap2{umap1};
    ASSERT_EQ(umap1, umap2);
    auto umap3(umap2);
    ASSERT_EQ(umap1, umap3);
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, MoveConstructor) {
    using map_type = TypeParam;

    auto umap1 = initialize_test_map<map_type>();
    ASSERT_FALSE(umap1.empty());
    auto umap2{initialize_test_map<map_type>()};
    ASSERT_EQ(umap1, umap2);
    auto umap3(initialize_test_map<map_type>());
    ASSERT_EQ(umap1, umap3);
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, CopyAssignment) {
    using map_type = TypeParam;

    auto umap1 = initialize_test_map<map_type>();
    ASSERT_FALSE(umap1.empty());
    auto umap2 = umap1;
    ASSERT_EQ(umap1, umap2);
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, MoveAssignment) {
    using map_type = TypeParam;

    auto umap = initialize_test_map<map_type>();
    ASSERT_FALSE(umap.empty());
    auto old_data = umap.data();
    umap          = initialize_test_map<map_type>();
    ASSERT_EQ(old_data, umap.data());
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, get_allocator) {
    using map_type = TypeParam;

    auto umap     = initialize_test_map<map_type>();
    auto max_size = umap.get_allocator().max_size();
    ASSERT_NE(0, max_size);
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, empty) {
    using map_type = TypeParam;

    map_type m;
    ASSERT_TRUE(m.empty());
    m = initialize_test_map<map_type>();
    ASSERT_FALSE(m.empty());
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, size) {
    using map_type = TypeParam;

    map_type m;
    ASSERT_EQ(0, m.size());
    m = initialize_test_map<map_type>();
    ASSERT_LT(0, m.size());
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, clear) {
    using map_type = TypeParam;

    map_type m = initialize_test_map<map_type>();
    ASSERT_FALSE(m.empty());
    m.clear();
    ASSERT_TRUE(m.empty());
  }

  TYPED_TEST_P(CommonConcurrentUnorderedMapTests, insert) {
    using map_type    = TypeParam;
    using value_type  = typename map_type::value_type;
    using key_type    = typename map_type::key_type;
    using mapped_type = typename map_type::mapped_type;
    using node_type   = typename map_type::node_type;

    // insert(const value_type &value)
    {
      map_type m;
      value_type v;
      ASSERT_TRUE(m.empty());
      ASSERT_TRUE(m.insert(v));
      ASSERT_FALSE(m.empty());
      ASSERT_FALSE(m.insert(v));
      ASSERT_EQ(v.second, m.at(v.first));
    }
    // insert(value_type &&value)
    {
      map_type m;
      ASSERT_TRUE(m.empty());
      ASSERT_TRUE(m.insert(value_type()));
      ASSERT_FALSE(m.insert(value_type()));
      ASSERT_EQ(value_type().second, m.at(value_type().first));
    }
    // insert(std::initializer_list<value_type> ilist)
    {
      map_type m;
      ASSERT_TRUE(m.empty());
      m.insert({
          {key_type(), mapped_type()},
      });
      ASSERT_FALSE(m.empty());
      ASSERT_EQ(mapped_type(), m.at(key_type()));
    }
    // insert(node_type &&nh)
    {
      map_type m;
      value_type v;
      ASSERT_TRUE(m.empty());
      ASSERT_TRUE(m.insert(v));
      ASSERT_FALSE(m.empty());
      node_type node = m.extract(v.first);
      ASSERT_FALSE(node.empty());
      ASSERT_TRUE(m.empty());
      ASSERT_EQ(node.mapped(), v.second);
      ASSERT_TRUE(m.insert(std::move(node)));
      ASSERT_FALSE(m.empty());
      ASSERT_EQ(v.second, m.at(v.first));
    }
  }

  REGISTER_TYPED_TEST_SUITE_P(CommonConcurrentUnorderedMapTests, // Comments so clang-format keeps
                              DefaultConstructor,                // these lines broken.
                              CopyConstructor,                   //
                              MoveConstructor,                   //
                              CopyAssignment,                    //
                              MoveAssignment,                    //
                              get_allocator,                     //
                              empty,                             //
                              size,                              //
                              clear,                             //
                              insert                             //
  );
  using Types = ::testing::Types<                    // Comments so clang-format keeps
      UnorderedMap<std::string, uint32_t>,           // these lines broken.
      UnorderedMap<std::string, std::string>,        //
      UnorderedMap<std::string, float>,              //
      UnorderedMap<int32_t, uint64_t>,               //
      UnorderedMap<int64_t, size_t>,                 //
      UnorderedMap<int32_t, std::string>,            //
      UnorderedMap<int64_t, std::string>,            //
      ShardedUnorderedMap<std::string, uint32_t>,    //
      ShardedUnorderedMap<std::string, std::string>, //
      ShardedUnorderedMap<std::string, float>,       //
      ShardedUnorderedMap<int32_t, uint64_t>,        //
      ShardedUnorderedMap<int64_t, size_t>,          //
      ShardedUnorderedMap<int32_t, std::string>,     //
      ShardedUnorderedMap<int64_t, std::string>>;    //
  INSTANTIATE_TYPED_TEST_SUITE_P(TypedTests, CommonConcurrentUnorderedMapTests, Types);

  TEST_F(UnshardedConcurrentUnorderedMapTests, IListConstructor) {
    UnorderedMap<std::string, std::string> umap{
        {"foo", "qux"},
        {"bar", "quux"},
        {"baz", "quuux"},
    };

    ASSERT_FALSE(umap.empty());
    ASSERT_EQ(3, umap.size());
    ASSERT_EQ("qux", umap.at("foo"));
    ASSERT_EQ("quux", umap.at("bar"));
    ASSERT_EQ("quuux", umap.at("baz"));
  }
  TEST_F(ShardedConcurrentUnorderedMapTests, IListConstructor) {
    ShardedUnorderedMap<std::string, std::string> umap{
        {"foo", "qux"},
        {"bar", "quux"},
        {"baz", "quuux"},
    };

    ASSERT_FALSE(umap.empty());
    ASSERT_EQ(3, umap.size());
    ASSERT_EQ("qux", umap.at("foo"));
    ASSERT_EQ("quux", umap.at("bar"));
    ASSERT_EQ("quuux", umap.at("baz"));
  }

  TEST_F(UnshardedConcurrentUnorderedMapTests, IListAssignment) {
    UnorderedMap<std::string, std::string> umap = {
        {"foo", "qux"},
        {"bar", "quux"},
        {"baz", "quuux"},
    };

    ASSERT_FALSE(umap.empty());
    ASSERT_EQ(3, umap.size());
    ASSERT_EQ("qux", umap.at("foo"));
    ASSERT_EQ("quux", umap.at("bar"));
    ASSERT_EQ("quuux", umap.at("baz"));
  }
  TEST_F(ShardedConcurrentUnorderedMapTests, IListAssignment) {
    ShardedUnorderedMap<std::string, std::string> umap = {
        {"foo", "qux"},
        {"bar", "quux"},
        {"baz", "quuux"},
    };

    ASSERT_FALSE(umap.empty());
    ASSERT_EQ(3, umap.size());
    ASSERT_EQ("qux", umap.at("foo"));
    ASSERT_EQ("quux", umap.at("bar"));
    ASSERT_EQ("quuux", umap.at("baz"));
  }

  TEST_F(UnshardedConcurrentUnorderedMapTests, max_size) {
    UnorderedMap<std::string, std::string> umap;
    ASSERT_LT(0, umap.max_size());
  }
} // anonymous namespace