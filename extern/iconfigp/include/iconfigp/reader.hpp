#ifndef ICONFIGP_READER_HPP_INCLUDED
#define ICONFIGP_READER_HPP_INCLUDED

#include "iconfigp/exception.hpp"
#include "iconfigp/located-string.hpp"

#include <string_view>
#include <tuple>

#include <cctype>



namespace iconfigp {

class reader {
  public:
    explicit reader(std::string_view source) :
      source_{source},
      start_{source.data()}
    {}



    [[nodiscard]] task_type task() const { return task_; }

    void task(task_type t) {
      task_ = t;
    }



    [[nodiscard]] const char*      ptr()       const { return source_.data();  }
    [[nodiscard]] size_t           offset()    const { return ptr() - start_;  }
    [[nodiscard]] bool             eof()       const { return source_.empty(); }
    [[nodiscard]] char             peek()      const { return source_.front(); }

    [[nodiscard]] std::string_view remaining() const { return source_; }



    void skip(size_t count = 1) {
      source_.remove_prefix(count);
    }

    void skip_ignored() {
      while (!eof()) {
        if (peek() == '#') {
          skip_line();
        } else if (isspace(peek()) != 0) {
          skip_whitespace();
        } else {
          break;
        }
      }
    }

    void skip_whitespace() {
      while (!eof() && isspace(peek()) != 0) {
        skip();
      }
    }



    void skip_until(std::string_view controls) {
      while (!eof() && controls.find(peek()) == std::string_view::npos) {
        if (peek() == '\\') {
          skip();
          if (eof()) {
            raise_exception(syntax_error_type::invalid_escape_sequence, offset() - 1);
          }
          skip();
        } else if (peek() == '\'' || peek() == '"') {
          auto start = offset();
          std::ignore = read_up_until_closing_quotation_mark();

          if (eof()) {
            raise_exception(syntax_error_type::missing_quotation_mark, start);
          }
          if (peek() == '\n') {
            raise_exception(syntax_error_type::missing_quotation_mark_eol, start);
          }
          skip();
        } else {
          skip();
        }
      }
    }






    [[nodiscard]] located_string read_until_one_of(std::string_view controls) {
      skip_whitespace_within_line();

      auto start = offset();



      if (peek() != '"' && peek() != '\'') {
        auto [content, end] = read_escaped_until_one_of(controls);

        while (!content.empty() && isspace(content.back()) != 0) {
          content.pop_back();
        }

        return located_string{std::move(content), start, end - start};
      }



      auto content = read_up_until_closing_quotation_mark();

      if (eof()) {
        raise_exception(syntax_error_type::missing_quotation_mark, start);
      }
      if (peek() == '\n') {
        raise_exception(syntax_error_type::missing_quotation_mark_eol, start);
      }

      skip();
      size_t size = offset() - start;

      skip_whitespace_within_line();
      assert_peek_is_one_of(controls);

      return located_string{std::move(content), start, size};
    }





  private:
    std::string_view source_;
    const char*      start_;
    task_type        task_  {task_type::toplevel};



    [[noreturn]] void raise_exception(syntax_error_type error, size_t off) const {
      throw syntax_exception{error, off, task_};
    }





    void skip_line() {
      while (!eof() && peek() != '\n') {
        skip();
      }
    }



    void skip_whitespace_within_line() {
      while (!eof() && peek() != '\n' && isspace(peek()) != 0) {
        skip();
      }
    }





    void assert_peek_is_one_of(std::string_view options) const {
      if (!eof() && options.find(peek()) == std::string_view::npos) {
        raise_exception(syntax_error_type::unexpected_character, offset());
      }
    }





    [[nodiscard]] std::pair<std::string, size_t> read_escaped_until_one_of(
        std::string_view controls
    ) {
      std::string content;

      auto last_non_whitespace = offset();

      for (; !eof() && controls.find(peek()) == std::string_view::npos; skip()) {
        if (peek() != '\\') {
          if (isspace(peek()) == 0) {
            last_non_whitespace = offset();
          }
          content.push_back(peek());
          continue;
        }

        skip();
        if (eof()) {
          raise_exception(syntax_error_type::invalid_escape_sequence, offset() - 1);
        }

        if (peek() == 'n') {
          content.push_back('\n');
        } else {
          content.push_back(peek());
        }

        last_non_whitespace = offset();
      }

      return {content, last_non_whitespace + 1};
    }



    [[nodiscard]] std::string read_up_until_closing_quotation_mark() {
      if (peek() == '"') {
        skip();
        return std::move(read_escaped_until_one_of("\"\n").first);
      }

      skip();
      std::string content;
      for (; !eof() && peek() != '\'' && peek() != '\n'; skip()) {
        content.push_back(peek());
      }
      return content;
    }
};

}

#endif // ICONFIGP_READER_HPP_INCLUDED
