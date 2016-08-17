#include "util/map_utils.h"

#include "gtest.h"

namespace tervuren {
namespace util {

namespace {

TEST(MapCompositionTest, EmptyArgumentProducesEmptyMap) {
  std::unordered_map<int, int> empty_map;
  std::unordered_map<int, int> one_two{{1, 2}};
  // If one of the maps to compose with is empty, the composition is empty.
  EXPECT_TRUE(Compose(empty_map, empty_map).empty());
  EXPECT_TRUE(Compose(empty_map, one_two).empty());
  EXPECT_TRUE(Compose(one_two, empty_map).empty());
  // If no value of the first map is a key of the second map, the composition is
  // empty.
  EXPECT_TRUE(Compose(one_two, one_two).empty());
}

// The composition of identity maps should be the identity map over the common
// set of keys and values.
TEST(MapCompositionTest, CompositionOfIdentityIsIdentity) {
  std::unordered_map<int, int> identity_02{{0, 0}, {1, 1}, {2, 2}};
  std::unordered_map<int, int> identity_01{{0, 0}, {1, 1}};
  std::unordered_map<int, int> identity_12{{1, 1}, {2, 2}};
  std::unordered_map<int, int> identity_11{{1, 1}};
  EXPECT_EQ(identity_02, Compose(identity_02, identity_02));
  // If the entries in one identity map are contained in the other, the result
  // is the smaller identity map.
  EXPECT_EQ(identity_01, Compose(identity_02, identity_01));
  EXPECT_EQ(identity_01, Compose(identity_01, identity_02));
  // The composition only has values in the first that appear as keys in the
  // second.
  EXPECT_EQ(identity_11, Compose(identity_01, identity_12));
}

// A map 'm' is injective if for all distinct keys 'a' and 'b', the values
// 'm[a]'  and 'm[b]' are also different.
TEST(MapCompositionTest, CompositionOfInjectiveMaps) {
  std::unordered_map<int, int> increments{{0, 1}, {1, 2}, {2, 3}};
  std::unordered_map<int, int> decrements{{1, 0}, {2, 1}, {3, 2}};
  std::unordered_map<int, int> identity_02{{0, 0}, {1, 1}, {2, 2}};
  EXPECT_EQ(identity_02, Compose(increments, decrements));
}

TEST(MapPreimageTest, PreimageProperties) {
  // The preimage of the empty function is an empty function.
  std::unordered_map<int, int> empty_map;
  std::unordered_map<int, std::unordered_set<int>> empty_pre;
  EXPECT_EQ(empty_pre, Preimage(empty_map));
  // An injective function will have singleton preimages.
  std::unordered_map<int, int> increments{{0, 1}, {1, 2}, {2, 3}};
  std::unordered_map<int, std::unordered_set<int>> pre_increments{
      {1, {0}}, {2, {1}}, {3, {2}}};
  EXPECT_EQ(pre_increments, Preimage(increments));
  // A non-injective function will have non-singleton preimages.
  std::unordered_map<int, int> parity{{0, 0}, {1, 1}, {2, 0}, {3, 1}};
  std::unordered_map<int, std::unordered_set<int>> pre_parity{{0, {0, 2}},
                                                              {1, {1, 3}}};
  EXPECT_EQ(pre_parity, Preimage(parity));
}

}  // namespace
}  // namespace util
}  // namespace tervuren
