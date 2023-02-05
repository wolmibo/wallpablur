#ifndef GL_OBJECT_NAME_HPP_INCLUDED
#define GL_OBJECT_NAME_HPP_INCLUDED

#include <functional>
#include <utility>

#include <epoxy/gl.h>



namespace gl {

template<typename FNC>
class object_name {
  public:
    explicit object_name(GLuint name = 0, FNC deleter = {}) :
      name_   {name},
      deleter_{deleter}
    {}



    object_name(const object_name&) = delete;

    object_name(object_name&& rhs) noexcept :
      name_   {std::exchange(rhs.name_, 0)},
      deleter_{std::exchange(rhs.deleter_, {})}
    {}



    object_name& operator=(const object_name&) = delete;

    object_name& operator=(object_name&& rhs) noexcept {
      std::swap(name_,    rhs.name_);
      std::swap(deleter_, rhs.deleter_);

      return *this;
    }



    ~object_name() {
      std::invoke(deleter_, name_);
    }



    [[nodiscard]] GLuint get() const { return name_; }



  private:
    GLuint name_;
    FNC    deleter_;
};

}

#endif // GL_OBJECT_NAME_HPP_INCLUDED
