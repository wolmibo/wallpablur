#ifndef WALLPABLUR_JSON_UTILS_HPP_INCLUDED
#define WALLPABLUR_JSON_UTILS_HPP_INCLUDED

#include <optional>
#include <string_view>

#include <rapidjson/document.h>



namespace json {
  void assert_parse_success(rapidjson::ParseResult);



  [[nodiscard]] std::optional<std::reference_wrapper<const rapidjson::Value>>
  find_member(const rapidjson::Value&, const rapidjson::Value::Ch*);



  [[nodiscard]] std::optional<unsigned int>
  member_to_uint(const rapidjson::Value&, const rapidjson::Value::Ch*);

  [[nodiscard]] std::optional<int>
  member_to_int (const rapidjson::Value&, const rapidjson::Value::Ch*);

  [[nodiscard]] std::optional<std::string_view>
  member_to_str (const rapidjson::Value&, const rapidjson::Value::Ch*);

  [[nodiscard]] std::optional<bool>
  member_to_bool(const rapidjson::Value&, const rapidjson::Value::Ch*);

  [[nodiscard]] std::optional<rapidjson::Value::ConstArray>
  member_to_array(const rapidjson::Value&, const rapidjson::Value::Ch*);
}


#endif // WALLPABLUR_JSON_UTILS_HPP_INCLUDED
