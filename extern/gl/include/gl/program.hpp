#ifndef GL_PROGRAM_HPP_INCLUDED
#define GL_PROGRAM_HPP_INCLUDED

#include "gl/object-name.hpp"

#include <string>
#include <string_view>



namespace gl {

class program {
  public:
    program(std::string_view, std::string_view);



    void use() const { glUseProgram(program_.get()); }

    [[nodiscard]] GLint uniform  (const std::string&) const;
    [[nodiscard]] GLint attribute(const std::string&) const;



  private:
    struct deleter {
      void operator()(GLuint p) { glDeleteProgram(p); }
    };

    object_name<deleter> program_;
};

}

#endif // GL_PROGRAM_HPP_INCLUDED
