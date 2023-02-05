#ifndef ICONFIGP_GROUP_HPP_INCLUDED
#define ICONFIGP_GROUP_HPP_INCLUDED

#include "iconfigp/exception.hpp"
#include "iconfigp/key-value.hpp"
#include "iconfigp/opt-ref.hpp"

#include <algorithm>
#include <span>
#include <vector>



namespace iconfigp {


class group {
  friend class section;

  public:
    [[nodiscard]] size_t                     offset()  const { return offset_;         }

    [[nodiscard]] bool                       empty()   const { return values_.empty(); }
    [[nodiscard]] std::span<const key_value> entries() const { return values_;         }



    [[nodiscard]] opt_ref<const key_value> unique_key(std::string_view name) const {
      opt_ref<const key_value> output;

      for (const auto& kv: values_) {
        if (kv.key() == name) {
          if (output) {
            throw multiple_definitions_exception{*output, kv, false};
          }
          output = kv;
        }
      }

      return output;
    }



    [[nodiscard]] const key_value& require_unique_key(std::string_view name) const {
      if (auto ref = unique_key(name)) {
        return *ref;
      }
      throw missing_key_exception{std::string{name}, offset_};
    }



    [[nodiscard]] size_t count_keys(std::string_view name) const {
      return std::ranges::count_if(values_,
          [name](const auto& kv) { return kv.key() == name; });
    }





  private:
    size_t                 offset_;
    std::vector<key_value> values_;



    group(size_t offset) :
      offset_{offset}
    {}

    template<typename... Args>
    void append(Args&&... args) {
      values_.push_back(key_value{std::forward<Args>(args)...});
    }
};


}

#endif // ICONFIGP_GROUP_HPP_INCLUDED
