#ifndef WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED
#define WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"

#include <limits>
#include <vector>

#include <iconfigp/exception.hpp>
#include <iconfigp/reader.hpp>
#include <iconfigp/value-parser.hpp>



namespace expression {

template<typename T>
class parser {
  public:
    [[nodiscard]] static boolean<T> parse(std::string_view input) {
      parser<T> p{input};
      try {
        for (auto token = p.next_token(); token; token = p.next_token()) {
          p.process(*token);
        }
        p.cleanup_stack();

        return boolean<T>{std::move(p.commands_)};

      } catch (const iconfigp::syntax_exception& ex) {
        throw iconfigp::value_parse_exception::range_exception{
          ex.what(),
          ex.offset(), 0
        };
      }
    }



  private:
    static constexpr size_t no_operator{std::numeric_limits<size_t>::max()};

    using instruction = typename boolean<T>::instruction;
    using command     = typename boolean<T>::command;



    iconfigp::reader     reader_;
    std::vector<command> commands_;



    struct located_string_view {
      std::string_view content;
      size_t           offset{0};

      void throw_error(std::string message) {
        throw iconfigp::value_parse_exception::range_exception{
          std::move(message) + std::string{content},
          offset, content.size()
        };
      }
    };

    std::vector<located_string_view> stack_;





    explicit parser(std::string_view input) :
      reader_{input}
    {}



    [[nodiscard]] bool last_is_value() const {
      return !commands_.empty() &&
        (!std::holds_alternative<instruction>(commands_.back()) ||
         std::get<instruction>(commands_.back()) != instruction::stack_store);
    }



    [[nodiscard]] size_t operator_precedence(located_string_view lsv) {
      if (lsv.content == "||") { return 2; }
      if (lsv.content == "&&") { return 1; }
      if (lsv.content == "!")  { return 0; }

      return no_operator;
    }



    [[nodiscard]] bool is_operator(located_string_view lsv) {
      return operator_precedence(lsv) != no_operator;
    }



    void stack_pop() {
      auto lsv = stack_.back();

      if (is_operator(lsv) && !last_is_value()) {
        lsv.throw_error("Missing right operand of ");
      }

      if      (lsv.content == "&&") { commands_.emplace_back(instruction::logical_and); }
      else if (lsv.content == "||") { commands_.emplace_back(instruction::logical_or);  }
      else if (lsv.content == "!" ) { commands_.emplace_back(instruction::logical_not); }

      stack_.pop_back();
    }



    void process(located_string_view lsv) {
      if (lsv.content == "(" || lsv.content == "!") {
        if (last_is_value()) {
          lsv.throw_error("Missing operator before ");
        }
        stack_.push_back(lsv);

      } else if (lsv.content == ")") {
        cleanup_stack(lsv);

      } else if (lsv.content == "&&" || lsv.content == "||") {
        if (!last_is_value()) {
          lsv.throw_error("Missing left operand of ");
        }
        cleanup_stack(lsv);
        stack_.push_back(lsv);
        commands_.emplace_back(instruction::stack_store);

      } else {
        if (last_is_value()) {
          lsv.throw_error("Missing operator before ");
        }

        if (auto t = T::from_string(lsv.content)) {
          commands_.emplace_back(std::move(*t));
        } else if (auto b = iconfigp::value_parser<bool>::parse(lsv.content)) {
          commands_.emplace_back(*b
              ? instruction::constant_true : instruction::constant_false);
        } else {
          lsv.throw_error("Unknown token ");
        }
      }
    }



    void cleanup_stack(located_string_view lsv) {
      if (lsv.content == ")") {
        while (!stack_.empty() && stack_.back().content != "(") {
          stack_pop();
        }

        if (stack_.empty() || stack_.back().content != "(") {
          throw iconfigp::value_parse_exception::range_exception{
            "Unexpected closing parentheses",
            lsv.offset, lsv.content.size()
          };
        }

        stack_.pop_back();

      } else if (is_operator(lsv)) {
        auto op = operator_precedence(lsv);

        while (!stack_.empty() && operator_precedence(stack_.back()) <= op) {
          stack_pop();
        }
      }
    }



    void cleanup_stack() {
      while (!stack_.empty() && stack_.back().content != "(") {
        stack_pop();
      }

      if (!stack_.empty()) {
        throw iconfigp::value_parse_exception::range_exception{
          "Missing closing parentheses",
          stack_.back().offset, stack_.back().content.size()
        };
      }

      if (!last_is_value()) {
        throw iconfigp::value_parse_exception::range_exception{
          "Expression does not have a value",
          0, 0
        };
      }
    }





    [[nodiscard]] std::optional<located_string_view> next_token() {
      reader_.skip_whitespace();

      if (reader_.eof()) {
        return {};
      }

      if (std::string_view{"!()"}.find(reader_.peek()) != std::string_view::npos) {
        auto content = reader_.remaining().substr(0, 1);
        reader_.skip();

        return located_string_view {
          .content = content,
          .offset  = reader_.offset() - 1
        };
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

        return located_string_view {
          .content = remaining.substr(0, 2),
          .offset  = reader_.offset() - 2
        };
      }



      auto start   = reader_.offset();
      auto content = reader_.remaining();

      reader_.skip();
      reader_.skip_until("()!&|");

      content = content.substr(0, reader_.offset() - start);

      while (!content.empty() && isspace(content.back()) != 0) {
        content.remove_suffix(1);
      }

      return located_string_view {
        .content = content,
        .offset  = start
      };
    }
};

}

#endif // WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED
