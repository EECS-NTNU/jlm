/*
 * Copyright 2025 Håvard Krogstie <krogstie.havard@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-registry.hpp>
#include <test-util.hpp>

#include <jlm/util/iterator_range.hpp>
#include <jlm/util/IteratorWrapper.hpp>

#include <cassert>
#include <memory>
#include <unordered_set>
#include <vector>

static void
TestPtrVector()
{
  int a = 10;
  int b = 20;
  int c = 30;

  std::vector<int *> ints = { &a, &b, &c };
  using ItType = jlm::util::PtrIterator<int, decltype(ints)::iterator>;

  // Act 1 + Assert 1 - iterate manually using operator ++
  ItType it(ints.begin());
  assert(*it == 10);
  it++;
  assert(*it == 20);
  it++;
  assert(it != ItType(ints.end()));
  it++;
  assert(it == ItType(ints.end()));

  // Act 2 - modify targets through range based for loop
  jlm::util::IteratorRange range(ItType(ints.begin()), ItType(ints.end()));
  for (auto & i : range)
  {
    i++;
  }

  // Assert 2
  assert(a == 11);
  assert(b == 21);
  assert(c == 31);
}

JLM_UNIT_TEST_REGISTER("jlm/util/TestIteratorWrapper-TestPtrVector", TestPtrVector);

static void
TestPtrUnorderedSet()
{
  int a = 10;
  int b = 20;
  int c = 30;

  std::unordered_set<int *> set = { &a, &b, &c };
  using ItType = jlm::util::PtrIterator<int, decltype(set)::iterator>;

  // Act - modify targets through range based for loop
  jlm::util::IteratorRange range(ItType(set.begin()), ItType(set.end()));
  for (auto & i : range)
  {
    i++;
  }

  // Assert
  assert(a == 11);
  assert(b == 21);
  assert(c == 31);
}

JLM_UNIT_TEST_REGISTER("jlm/util/TestIteratorWrapper-TestPtrUnorderedSet", TestPtrUnorderedSet);

static void
TestUniquePtrVector()
{
  // Arrange
  std::vector<std::unique_ptr<int>> vector;
  vector.emplace_back(std::make_unique<int>(10));
  vector.emplace_back(std::make_unique<int>(20));
  vector.emplace_back(std::make_unique<int>(30));

  // The iterator still works with a const_iterator, as the pointer itself is not const
  using ItType = jlm::util::PtrIterator<int, decltype(vector)::const_iterator>;

  // Act
  jlm::util::IteratorRange range(ItType(vector.begin()), ItType(vector.end()));
  for (auto & i : range)
  {
    i++;
  }

  // Assert
  assert(*vector[0] == 11);
  assert(*vector[1] == 21);
  assert(*vector[2] == 31);
}

JLM_UNIT_TEST_REGISTER("jlm/util/TestIteratorWrapper-TestUniquePtrVector", TestUniquePtrVector);

static void
TestMapValuePtr()
{
  // Arrange
  std::unordered_map<size_t, std::unique_ptr<int>> map;
  map.insert({ 1, std::make_unique<int>(10) });
  map.insert({ 2, std::make_unique<int>(20) });
  map.insert({ 3, std::make_unique<int>(30) });

  // The iterator still works with a const_iterator, as the pointer itself is not const
  using ItType = jlm::util::MapValuePtrIterator<int, decltype(map)::const_iterator>;

  // Act
  jlm::util::IteratorRange range(ItType(map.begin()), ItType(map.end()));
  for (auto & i : range)
  {
    i++;
  }

  // Assert
  assert(*map[1] == 11);
  assert(*map[2] == 21);
  assert(*map[3] == 31);
}

JLM_UNIT_TEST_REGISTER("jlm/util/TestIteratorWrapper-TestMapValuePtr", TestMapValuePtr);
