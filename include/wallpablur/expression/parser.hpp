#ifndef WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED
#define WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/expression/tokenizer.hpp"

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
        for (auto token = p.tokenizer_.next(); token; token = p.tokenizer_.next()) {
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
    using command = typename boolean<T>::command;

    tokenizer            tokenizer_;
    std::vector<token>   stack_;

    std::vector<command> commands_;



    explicit parser(std::string_view input) :
      tokenizer_{input}
    {}



    [[nodiscard]] bool last_is_value() const {
      return !commands_.empty() &&
        (!std::holds_alternative<instruction>(commands_.back()) ||
         std::get<instruction>(commands_.back()) != instruction::stack_store);
    }



    void stack_pop() {
      auto lsv = stack_.back();
      stack_.pop_back();

      if (!lsv.is_operator()) {
        return;
      }

      if (!last_is_value()) {
        lsv.throw_error("Missing right operand of ");
      }

      commands_.emplace_back(lsv.to_instruction());
    }



    void process(token lsv) {
      switch (lsv.type()) {
        case token_type::paren_open:
        case token_type::logical_not:
          if (last_is_value()) {
            lsv.throw_error("Missing operator before ");
          }
          stack_.push_back(lsv);
          break;

        case token_type::paren_close:
          cleanup_stack(lsv);
          break;

        case token_type::logical_and:
        case token_type::logical_or:
          if (!last_is_value()) {
            lsv.throw_error("Missing left operand of ");
          }
          cleanup_stack(lsv);
          stack_.push_back(lsv);
          commands_.emplace_back(instruction::stack_store);
          break;

        case token_type::value:
          if (last_is_value()) {
            lsv.throw_error("Missing operator before ");
          }

          if (auto t = T::from_token(lsv)) {
            commands_.emplace_back(std::move(*t));
          } else if (auto b = iconfigp::value_parser<bool>::parse(lsv.content())) {
            commands_.emplace_back(*b
                ? instruction::constant_true : instruction::constant_false);
          } else {
            lsv.throw_error("Unknown token ");
          }
          break;
      }
    }



    void cleanup_stack(token lsv) {
      if (lsv.type() == token_type::paren_close) {
        while (!stack_.empty() && stack_.back().type() != token_type::paren_open) {
          stack_pop();
        }

        if (stack_.empty() || stack_.back().type() != token_type::paren_open) {
          throw iconfigp::value_parse_exception::range_exception{
            "Unexpected closing parentheses",
            lsv.offset(), lsv.content().size()
          };
        }

        stack_.pop_back();

      } else if (lsv.is_operator()) {
        auto op = lsv.precedence();

        while (!stack_.empty() && stack_.back().precedence() <= op) {
          stack_pop();
        }
      }
    }



    void cleanup_stack() {
      while (!stack_.empty() && stack_.back().type() != token_type::paren_open) {
        stack_pop();
      }

      if (!stack_.empty()) {
        throw iconfigp::value_parse_exception::range_exception{
          "Missing closing parentheses",
          stack_.back().offset(), stack_.back().content().size()
        };
      }

      if (!last_is_value()) {
        throw iconfigp::value_parse_exception::range_exception{
          "Expression does not have a value",
          0, 0
        };
      }
    }
};

}

#endif // WALLPABLUR_EXPRESSION_PARSER_HPP_INCLUDED
