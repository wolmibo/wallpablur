#include "wallpablur/json/utils.hpp"

#include <rapidjson/error/en.h>

#include <logcerr/log.hpp>



void json::assert_parse_success(rapidjson::ParseResult code) {
  if (!code.IsError()) {
    return;
  }

  const auto *message = rapidjson::GetParseError_En(code.Code());

  throw std::runtime_error{logcerr::format("rapidjson error: {} ({})",
      message, code.Offset())};
}





std::optional<std::reference_wrapper<const rapidjson::Value>> json::find_member(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
){
  if (auto it = value.FindMember(name); it != value.MemberEnd() && !it->value.IsNull()) {
    return std::ref(it->value);
  }

  return {};
}





std::optional<unsigned int> json::member_to_uint(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
) {
  return find_member(value, name)
            .transform([name](const auto& val) {
              if (val.get().IsUint()) {
                return val.get().GetUint();
              }
              throw std::invalid_argument{
                std::string{"expected type unsigned int for member "} + name};
            });
}



std::optional<int> json::member_to_int(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
) {
  return find_member(value, name)
            .transform([name](const auto& val) {
              if (val.get().IsInt()) {
                return val.get().GetInt();
              }
              throw std::invalid_argument{
                std::string{"expected type int for member "} + name};
            });
}



std::optional<bool> json::member_to_bool(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
) {
  return find_member(value, name)
            .transform([name](const auto& val) {
              if (val.get().IsBool()) {
                return val.get().GetBool();
              }
              throw std::invalid_argument{
                std::string{"expected type int for member "} + name};
            });
}



std::optional<std::string_view> json::member_to_str(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
) {
  return find_member(value, name)
            .transform([name](const auto& val) {
              if (val.get().IsString()) {
                return std::string_view{val.get().GetString(),
                  val.get().GetStringLength()};
              }
              throw std::invalid_argument{
                std::string{"expected type int for member "} + name};
            });
}



std::optional<rapidjson::Value::ConstArray> json::member_to_array(
    const rapidjson::Value&     value,
    const rapidjson::Value::Ch* name
) {
  return find_member(value, name)
            .transform([name](const auto& val) {
              if (val.get().IsArray()) {
                return val.get().GetArray();
              }
              throw std::invalid_argument{
                std::string{"expected type array for member "} + name};
            });
}
