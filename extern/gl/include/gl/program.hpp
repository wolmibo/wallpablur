#ifndef GL_PROGRAM_HPP_INCLUDED
#define GL_PROGRAM_HPP_INCLUDED

#include "gl/object-name.hpp"

#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>



namespace gl {

enum class shader_type : GLenum {
  vertex   = GL_VERTEX_SHADER,
  fragment = GL_FRAGMENT_SHADER,

  linking  = 0
};



class program_error : public std::runtime_error {
  public:
    program_error(GLint, shader_type, std::string);

    struct message {
      std::string complete_message;

      size_t      row;
      size_t      column;
      std::string message;
    };

    [[nodiscard]] shader_type                     type()     const { return type_;     }
    [[nodiscard]] std::optional<std::string_view> source()   const;
    [[nodiscard]] std::span<const message>        messages() const { return messages_; }



  private:
    shader_type          type_;
    std::string          source_;
    std::vector<message> messages_;
};





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
