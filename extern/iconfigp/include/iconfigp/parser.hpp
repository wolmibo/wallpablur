#ifndef ICONFIGP_PARSER_HPP_INCLUDED
#define ICONFIGP_PARSER_HPP_INCLUDED

#include "iconfigp/exception.hpp"
#include "iconfigp/reader.hpp"
#include "iconfigp/section.hpp"

#include <optional>
#include <string_view>
#include <utility>



namespace iconfigp {

class parser {
  public:
    [[nodiscard]] static section parse(std::string_view input) {
      parser p{input};
      p.parse_input();
      return std::move(p.root_);
    }



  private:
    reader    reader_;
    task_type task_  {task_type::toplevel};
    section   root_  {"", 0};



    explicit parser(std::string_view input) :
      reader_{input}
    {}



    void task(task_type type) {
      task_ = type;
      reader_.task(type);
    }



    [[noreturn]] void raise_error(syntax_error_type type) {
      raise_error(type, reader_.offset());
    }

    [[noreturn]] void raise_error(syntax_error_type type, size_t offset) {
      throw syntax_exception{type, offset, task_};
    }





    void parse_input() {
      task(task_type::toplevel);

      reader_.skip_ignored();

      while (!reader_.eof()) {
        if (reader_.peek() == '[') {
          parse_section();

        } else if (reader_.peek() == '-') {
          root_.new_group(reader_.offset());
          reader_.skip();

        } else if (reader_.peek() == ';') {
          raise_error(syntax_error_type::unexpected_semicolon);

        } else {
          parse_key_value();
        }

        task(task_type::toplevel);
        reader_.skip_ignored();
      }
    }





    void parse_section() {
      task(task_type::section);

      auto section_start = reader_.offset();

      std::vector<std::string> section_path;
      while (!reader_.eof() && reader_.peek() != ']') {
        reader_.skip();
        section_path.emplace_back(reader_.read_until_one_of(".]").take_string());
      }

      if (reader_.eof()) {
        raise_error(syntax_error_type::missing_section_end, section_start);
      }
      reader_.skip();

      root_.select_section(section_path, section_start);
    }



    void parse_key_value() {
      task(task_type::key);
      auto key = reader_.read_until_one_of("=;\n");

      if (reader_.eof() || reader_.peek() != '=') {
        raise_error(syntax_error_type::missing_value);
      }

      if (key.content().empty()) {
        raise_error(syntax_error_type::empty_key);
      }

      reader_.skip();

      task(task_type::value);
      auto value = reader_.read_until_one_of("\n;[");

      if (!reader_.eof() && reader_.peek() == ';') {
        reader_.skip();
      }

      root_.append(std::move(key), std::move(value));
    }
};

}

#endif // ICONFIGP_PARSER_HPP_INCLUDED
