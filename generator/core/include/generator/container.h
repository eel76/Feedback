#pragma once
#include <algorithm>
#include <cassert>
#include <limits>
#include <map>

namespace generator::container {

  template <typename K, typename V> struct interval_map {
    explicit interval_map(V const& val) {
      m_map.insert(m_map.end(), std::make_pair(std::numeric_limits<K>::lowest(), val));
    }

    void assign(K const& keyBegin, K const& keyEnd, V const& val) {
      if (!(keyBegin < keyEnd))
        return;

      constexpr auto emplace_hint = [](auto& map, auto hint, K const& key, V const& value) {
        if (hint->second == value)
          return hint;

        if (hint->first < key)
          return map.emplace_hint(++hint, key, value);

        hint->second = value;
        return hint;
      };

      auto obsoleteBegin = --m_map.upper_bound(keyBegin);
      auto obsoleteEnd   = --m_map.upper_bound(keyEnd);

      auto const valBehind = obsoleteEnd->second;

      if (obsoleteBegin == m_map.begin() || obsoleteBegin->first < keyBegin)
        ++obsoleteBegin;

      auto hint = --m_map.erase(obsoleteBegin, ++obsoleteEnd);

      hint = emplace_hint(m_map, hint, keyBegin, val);
      emplace_hint(m_map, hint, keyEnd, valBehind);
    }

    bool is_constant() const noexcept {
      assert(is_canonical());
      return (++m_map.begin() == m_map.end());
    }

    bool is_canonical() const noexcept {
      auto const position =
      std::adjacent_find(begin(m_map), end(m_map), [](auto&& lhs, auto&& rhs) { return lhs.second == rhs.second; });
      return position == end(m_map);
    }

    V const& operator[](K const& key) const noexcept {
      return (--m_map.upper_bound(key))->second;
    }

  private:
    std::map<K, V> m_map;
  };
} // namespace generator::container
