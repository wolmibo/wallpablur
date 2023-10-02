#ifndef WALLPABLUR_FLAT_MAP_HPP_INCLUDED
#define WALLPABLUR_FLAT_MAP_HPP_INCLUDED

#include <algorithm>
#include <optional>
#include <span>
#include <utility>
#include <vector>

template<typename Key, typename Value>
class flat_map {
  public:
    template<typename K, typename... Args>
    Value& find_or_create(K&& key, Args&&... args) {
      auto it = std::ranges::find(keys_, std::forward<K>(key));
      if (it != keys_.end()) {
        return values_[it - keys_.begin()];
      }

      keys_.emplace_back(std::forward<K>(key));
      values_.emplace_back(std::forward<Args>(args)...);

      return values_.back();
    }



    template<typename K>
    std::optional<size_t> find_index(K&& key) {
      auto it = std::ranges::find(keys_, std::forward<K>(key));
      if (it != keys_.end()) {
        return it - keys_.begin();
      }
      return {};
    }



    void erase(size_t index) {
      std::swap(keys_.at(index), keys_.back());
      keys_.pop_back();
      std::swap(values_.at(index), values_.back());
      values_.pop_back();
    }



    [[nodiscard]] std::span<Key>   keys()   { return keys_;   }
    [[nodiscard]] std::span<Value> values() { return values_; }

    [[nodiscard]] const Key& key(size_t index) const {
      return keys_.at(index);
    }

    [[nodiscard]] Value& value(size_t index) {
      return values_.at(index);
    }

    [[nodiscard]] size_t size() const { return keys_.size(); }



    template<typename K, typename...Args>
    void emplace(K&& key, Args... args) {
      keys_.emplace_back(std::forward<K>(key));
      values_.emplace_back(std::forward<Args>(args)...);
    }



  private:
    std::vector<Key>   keys_;
    std::vector<Value> values_;
};

#endif // WALLPABLUR_FLAT_MAP_HPP_INCLUDED
