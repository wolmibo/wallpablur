#ifndef ICONFIGP_OPT_REF_HPP_INCLUDED
#define ICONFIGP_OPT_REF_HPP_INCLUDED

#include <optional>



namespace iconfigp {

template<typename T>
class opt_ref {
  public:
    opt_ref() = default;

    opt_ref(T& value) :
      ptr_{std::addressof(value)}
    {}



    [[nodiscard]] operator bool()  const { return ptr_ != nullptr; }
    [[nodiscard]] bool has_value() const { return ptr_ != nullptr; }

    [[nodiscard]]       T* operator->()       { return ptr_; }
    [[nodiscard]] const T* operator->() const { return ptr_; }

    [[nodiscard]] T&       operator*()        { return *ptr_; }
    [[nodiscard]] const T& operator*()  const { return *ptr_; }



    [[nodiscard]] T& value() {
      if (ptr_ != nullptr) {
        return *ptr_;
      }
      throw std::bad_optional_access{};
    }

    [[nodiscard]] const T& value() const {
      if (ptr_ != nullptr) {
        return *ptr_;
      }
      throw std::bad_optional_access{};
    }



  private:
    T* ptr_{nullptr};
};


}

#endif // ICONFIGP_OPT_REF_HPP_INCLUDED
