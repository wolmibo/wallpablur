#ifndef ICONFIGP_EXCEPTION_HPP_INCLUDED
#define ICONFIGP_EXCEPTION_HPP_INCLUDED

#include "iconfigp/format.hpp"
#include "iconfigp/key-value.hpp"

#include <stdexcept>



namespace iconfigp {

class exception : public std::runtime_error {
  protected:
    explicit exception(const std::string& message) :
      std::runtime_error{message}
    {}
};



[[nodiscard]] std::string format_exception(const exception&,
    std::string_view source = "", bool colored = false);





class missing_key_exception: public exception {
  public:
    missing_key_exception(std::string key, size_t offset) :
      exception{iconfigp::format("missing key {} in group", key)},
      key_     {std::move(key)},
      offset_  {offset}
    {}



    [[nodiscard]] std::string_view key()    const { return key_;    }
    [[nodiscard]] size_t           offset() const { return offset_; }



  private:
    std::string key_;
    size_t      offset_;
};





class value_parse_exception: public exception {
  public:
    value_parse_exception(key_value value, std::string target, std::string format) :
      exception{iconfigp::format("cannot parse value of {} as {}", value.key(), target)},
      value_   {std::move(value)},
      target_  {std::move(target)},
      format_  {std::move(format)}
    {}



    [[nodiscard]] const key_value& value()  const { return value_;  }

    [[nodiscard]] std::string_view target() const { return target_; }
    [[nodiscard]] std::string_view format() const { return format_; }



  private:
    key_value   value_;
    std::string target_;
    std::string format_;
};





class multiple_definitions_exception: public exception {
  public:
    multiple_definitions_exception(
        key_value definition1,
        key_value definition2,
        bool      per_section
    ) :
      exception   {iconfigp::format("multiple definitions of the same key {}",
                     definition1.key())},
      definition1_{std::move(definition1)},
      definition2_{std::move(definition2)},
      per_section_{per_section}
    {}



    [[nodiscard]] const key_value& definition1() const { return definition1_; }
    [[nodiscard]] const key_value& definition2() const { return definition2_; }
    [[nodiscard]] bool             per_section() const { return per_section_; }



  private:
    key_value definition1_;
    key_value definition2_;
    bool      per_section_;
};





enum class syntax_error_type {
  /// reader errors
  missing_quotation_mark,
  missing_quotation_mark_eol,
  invalid_escape_sequence,
  unexpected_character,

  ///  parser errors
  unexpected_semicolon,
  missing_section_end,
  missing_value,
  empty_key,
};



enum class task_type {
  key,
  value,
  section,
  toplevel,
};



class syntax_exception : public exception {
  public:
    syntax_exception(
        syntax_error_type type,
        size_t            offset,
        task_type         task = task_type::toplevel
    ) :
      exception{"syntax error in input"},
      type_    {type},
      offset_  {offset},
      task_    {task}
    {}



    [[nodiscard]] syntax_error_type type()   const { return type_;   }
    [[nodiscard]] size_t            offset() const { return offset_; }

    [[nodiscard]] task_type         task()   const { return task_;   }



  private:
    syntax_error_type type_;
    size_t            offset_;
    task_type         task_;
};

}


#endif // ICONFIGP_EXCEPTION_HPP_INCLUDED
