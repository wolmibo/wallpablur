#ifndef ICONFIGP_SERIALIZE_HPP_INCLUDED
#define ICONFIGP_SERIALIZE_HPP_INCLUDED

#include <string>
#include <string_view>



namespace iconfigp {

[[nodiscard]] std::string serialize(std::string_view, std::string_view = "\n\"");

class section;
class group;
class key_value;

[[nodiscard]] std::string serialize(const section&,   size_t = 0, std::string_view = "");
[[nodiscard]] std::string serialize(const group&,     size_t = 0);
[[nodiscard]] std::string serialize(const key_value&);

}

#endif // ICONFIGP_SERIALIZE_HPP_INCLUDED
