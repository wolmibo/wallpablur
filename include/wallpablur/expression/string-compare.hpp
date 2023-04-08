#ifndef WALLPABLUR_EXPRESSION_STRING_COMPARE_HPP_INCLUDED
#define WALLPABLUR_EXPRESSION_STRING_COMPARE_HPP_INCLUDED

#include <optional>
#include <string>
#include <string_view>
#include <utility>



namespace expression {

class string_compare {
  public:
    enum class mode {
      equal,
      starts_with,
      ends_with,
      contains,
    };



    string_compare(std::string&& value, mode m) :
      value_{std::move(value)},
      mode_ {m}
    {}



    [[nodiscard]] bool evaluate(std::string_view value) const {
      switch (mode_) {
        case mode::equal:           return value == value_;
        case mode::starts_with:     return value.starts_with(value_);
        case mode::ends_with:       return value.ends_with  (value_);
        case mode::contains:        return value.contains   (value_);
      }

      return false;
    }



  private:
    std::string value_;
    mode        mode_;
};



[[nodiscard]] std::optional<std::pair<string_compare, std::string_view>>
parse_string_compare(std::string_view);

};

#endif // WALLPABLUR_EXPRESSION_STRING_COMPARE_HPP_INCLUDED
