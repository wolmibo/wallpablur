// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_EXPRESSION_TOKENIZER_HPP_INCLUDED
#define WALLPABLUR_EXPRESSION_TOKENIZER_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"

#include <limits>
#include <string_view>

#include <iconfigp/exception.hpp>
#include <iconfigp/reader.hpp>



namespace expression {



enum class token_type : size_t {
  logical_not = 0,
  logical_and = 1,
  logical_or  = 2,

  paren_open  = 1000,
  paren_close = 1001,

  value       = 2000
};



class token {
  public:
    static constexpr size_t no_operator = std::numeric_limits<size_t>::max();



    token(std::string_view, size_t = 0);



    [[nodiscard]] std::string_view content() const { return content_; }
    [[nodiscard]] size_t           offset()  const { return offset_;  }
    [[nodiscard]] token_type       type()    const { return type_;    }

    [[nodiscard]] size_t precedence()    const { return std::to_underlying(type_);  }
    [[nodiscard]] bool   is_operator()   const { return precedence() < 1000;        }



    [[nodiscard]] instruction to_instruction() const;



    [[noreturn]] void throw_error(std::string) const;



  private:
    std::string_view content_;
    size_t           offset_;

    token_type       type_;
};





class tokenizer {
  public:
    explicit tokenizer(std::string_view);

    [[nodiscard]] std::optional<token> next();



  private:
    iconfigp::reader reader_;
};

}

#endif // WALLPABLUR_EXPRESSION_TOKENIZER_HPP_INCLUDED
