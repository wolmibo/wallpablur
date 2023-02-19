#ifndef WALLPABLUR_EXPRESSION_BOOLEAN_HPP_INCLUDED
#define WALLPABLUR_EXPRESSION_BOOLEAN_HPP_INCLUDED

#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include <cstddef>



namespace expression {

template<typename T>
class boolean {
  public:
    enum class instruction {
      constant_true,
      constant_false,

      logical_not,

      logical_and,
      logical_or,

      stack_store
    };

    using command = std::variant<instruction, T>;



    boolean() = default;

    boolean(bool value) :
      commands_{value ? instruction::constant_true : instruction::constant_false}
    {}

    explicit boolean(std::vector<command>&& commands) :
      commands_{std::move(commands)}
    {
      assert_stack_safety();
    }



    template<typename... Args>
    [[nodiscard]] bool evaluate(Args&&... args) const {
      stack_clear();

      bool current{true};

      for (const auto& cmd: commands_) {
        if (std::holds_alternative<T>(cmd)) {
          current = std::get<T>(cmd).evaluate(std::forward<Args>(args)...);
        } else {
          switch (std::get<instruction>(cmd)) {
            case instruction::constant_true:
              current = true;
              break;
            case instruction::constant_false:
              current = false;
              break;

            case instruction::logical_not:
              current = !current;
              break;

            case instruction::logical_and:
              current = stack_pop() && current;
              break;
            case instruction::logical_or:
              current = stack_pop() || current;
              break;

            case instruction::stack_store:
              stack_push(current);
              break;
          }
        }
      }

      return current;
    }





  private:
    std::vector<command> commands_;



    enum class totally_not_bool : bool { yes = true, no = false};

    mutable std::vector<totally_not_bool> stack_;

    [[nodiscard]] bool stack_pop() const {
      auto value = std::to_underlying(stack_.back());
      stack_.pop_back();
      return value;
    }

    void stack_push(bool value) const {
      stack_.push_back(static_cast<totally_not_bool>(value));
    }

    void stack_clear() const {
      stack_.clear();
    }





    [[nodiscard]] static std::ptrdiff_t stack_change(const command& c) {
      if (std::holds_alternative<T>(c)) {
        return 0;
      }

      switch (std::get<instruction>(c)) {
        case instruction::constant_true:
        case instruction::constant_false:
        case instruction::logical_not:
          return 0;
        case instruction::logical_or:
        case instruction::logical_and:
          return -1;
        case instruction::stack_store:
          return 1;
      }

      return 0;
    }



    void assert_stack_safety() const {
      std::ptrdiff_t sum{0};
      for (const auto& cmd: commands_) {
        sum += stack_change(cmd);
        if (sum < 0) {
          throw std::runtime_error{"expression::boolean: more stack pops than pushes"};
        }
      }
      if (sum != 0) {
        throw std::runtime_error{"expression::boolean: more stack pushes than pops"};
      }
    }
};

}


#endif // WALLPABLUR_EXPRESSION_BOOLEAN_HPP_INCLUDED
