#ifndef ICONFIGP_LOCATED_STRING_HPP_INCLUDED
#define ICONFIGP_LOCATED_STRING_HPP_INCLUDED

#include <string>
#include <string_view>

namespace iconfigp {

class located_string {
  public:
    located_string(std::string content, size_t offset, size_t size) :
      content_{std::move(content)},
      offset_ {offset},
      size_   {size}
    {}

    explicit located_string(std::string content) :
      content_{std::move(content)},
      offset_ {0},
      size_   {content_.size()}
    {}



    [[nodiscard]] bool operator==(const located_string&) const = default;



    [[nodiscard]] std::string_view content()     const { return content_; }
    [[nodiscard]] size_t           offset()      const { return offset_;  }
    [[nodiscard]] size_t           size()        const { return size_;    }

    [[nodiscard]] std::string take_string() && { return std::move(content_); }



  private:
    std::string content_;
    size_t      offset_;
    size_t      size_;
};

}

#endif // ICONFIGP_LOCATED_STRING_HPP_INCLUDED
