#ifndef ICONFIGP_KEY_VALUE_HPP_INCLUDED
#define ICONFIGP_KEY_VALUE_HPP_INCLUDED

#include "iconfigp/located-string.hpp"



namespace iconfigp {

class key_value {
  public:
    key_value(located_string key, located_string value) :
      key_        {std::move(key)},
      value_      {std::move(value)}
    {}



    [[nodiscard]] size_t           key_offset()   const { return key_.offset();   }
    [[nodiscard]] size_t           key_size()     const { return key_.size();     }
    [[nodiscard]] size_t           value_offset() const { return value_.offset(); }
    [[nodiscard]] size_t           value_size()   const { return value_.size();   }

    [[nodiscard]] bool             used()         const { return used_;           }

    [[nodiscard]] std::string_view key()          const { return key_.content();  }

    [[nodiscard]] std::string_view value() const {
      used_ = true;
      return value_.content();
    }



  private:
    located_string key_;
    located_string value_;

    mutable bool   used_{false};
};


}

#endif // ICONFIGP_KEY_VALUE_HPP_INCLUDED
