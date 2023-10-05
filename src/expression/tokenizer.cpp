#include "wallpablur/expression/tokenizer.hpp"

using namespace expression;



namespace {
  [[nodiscard]] token_type type_from_content(std::string_view input) {
    if (input == "||") return token_type::logical_or;
    if (input == "&&") return token_type::logical_and;
    if (input == "!")  return token_type::logical_not;
    if (input == "(")  return token_type::paren_open;
    if (input == ")")  return token_type::paren_close;

    return token_type::value;
  }
}



token::token(std::string_view content, size_t offset):
  content_{content},
  offset_ {offset},
  type_   {type_from_content(content_)}
{}



void token::throw_error(std::string message) const {
  throw iconfigp::value_parse_exception::range_exception{
    std::move(message) + std::string{content()},
    offset(), content().size()
  };
}



instruction token::to_instruction() const {
  switch (type_) {
    case token_type::logical_not: return instruction::logical_not;
    case token_type::logical_and: return instruction::logical_and;
    case token_type::logical_or:  return instruction::logical_or;

    default:
      throw_error("Not an instruction ");
  }
}





tokenizer::tokenizer(std::string_view input) :
  reader_{input}
{}





namespace {
  [[nodiscard]] token read_value(iconfigp::reader& reader) {
    auto start   = reader.offset();
    auto content = reader.remaining();

    reader.skip();
    reader.skip_until("()!&|");

    content = content.substr(0, reader.offset() - start);

    while (!content.empty() && isspace(content.back()) != 0) {
      content.remove_suffix(1);
    }

    return token{content, start};
  }
}



std::optional<token> tokenizer::next() {
  reader_.skip_whitespace();

  if (reader_.eof()) {
    return {};
  }



  if (std::string_view{"!()"}.find(reader_.peek()) != std::string_view::npos) {
    auto content = reader_.remaining().substr(0, 1);
    reader_.skip();

    return token{content, reader_.offset() - 1};
  }



  if (std::string_view{"&|"}.find(reader_.peek()) != std::string_view::npos) {
    auto remaining = reader_.remaining();

    reader_.skip();
    if (reader_.eof() || reader_.peek() != remaining.front()) {
      throw iconfigp::value_parse_exception::range_exception{
        "Invalid operator",
        reader_.offset() - 1, 1
      };
    }
    reader_.skip();

    return token{remaining.substr(0, 2), reader_.offset() - 2};
  }

  return read_value(reader_);
}
