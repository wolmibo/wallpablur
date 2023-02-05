#include "gl/program.hpp"

#include <stdexcept>



namespace {
  [[nodiscard]] bool program_good(GLint program) {
    GLint status{GL_FALSE};

    glGetProgramiv(program, GL_LINK_STATUS, &status);

    return status != GL_FALSE && program != 0;
  }



  [[nodiscard]] std::string get_program_message(GLint program) {
    GLint length{0};
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    if (length < 0) {
      throw std::runtime_error("unable to obtain program message");
    }

    std::string buffer(static_cast<size_t>(length), ' ');
    glGetProgramInfoLog(program, buffer.size(), nullptr, buffer.data());

    return buffer;
  }





  [[nodiscard]] bool shader_good(GLint shader) {
    GLint status{GL_FALSE};

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    return status != GL_FALSE && shader != 0;
  }



  [[nodiscard]] std::string get_shader_message(GLint shader) {
    GLint length{0};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length < 0) {
      throw std::runtime_error("unable to obtain shader message");
    }

    std::string buffer(static_cast<size_t>(length), ' ');
    glGetShaderInfoLog(shader, buffer.size(), nullptr, buffer.data());

    return buffer;
  }





  [[nodiscard]] constexpr std::string shader_type_to_string(GLenum type) {
    switch (type) {
      case GL_VERTEX_SHADER:   return "vertex shader";
      case GL_FRAGMENT_SHADER: return "fragment shader";
      default:                 return "unknown shader type";
    }
  }



  class shader {
    public:
      shader(GLenum type, std::string_view source) :
        shader_{glCreateShader(type)}
      {
        GLint length       = source.length();
        const GLchar* data = source.data();

        glShaderSource(shader_.get(), 1, &data, &length);
        glCompileShader(shader_.get());

        if (!shader_good(shader_.get())) {
          auto message = get_shader_message(shader_.get());
          glDeleteShader(shader_.get());

          throw std::runtime_error{"failed to compile "
            + shader_type_to_string(type) + ":\n" + message};
        }
      }



      [[nodiscard]] GLuint get() const { return shader_.get(); }



    private:
      struct deleter {
        void operator()(GLuint s) { glDeleteShader(s); }
      };

      gl::object_name<deleter> shader_;
  };
}



gl::program::program(std::string_view vs, std::string_view fs) :
  program_{glCreateProgram()}
{
  shader vertex  {GL_VERTEX_SHADER,   vs};
  glAttachShader(program_.get(), vertex.get());

  shader fragment{GL_FRAGMENT_SHADER, fs};
  glAttachShader(program_.get(), fragment.get());


  glLinkProgram(program_.get());

  if (!program_good(program_.get())) {
    auto message = get_program_message(program_.get());
    glDeleteProgram(program_.get());

    throw std::runtime_error{"failed to compile program:\n" + message};
  }

  glDetachShader(program_.get(), vertex.get());
  glDetachShader(program_.get(), fragment.get());
}





GLint gl::program::uniform(const std::string& id) const {
  auto uniform = glGetUniformLocation(program_.get(), id.c_str());

  if (uniform == -1) {
    throw std::runtime_error{"unable to find uniform \"" + id + "\""};
  }

  return uniform;
}



GLint gl::program::attribute(const std::string& id) const {
  auto attribute = glGetAttribLocation(program_.get(), id.c_str());

  if (attribute == -1) {
    throw std::runtime_error{"unable to find attribute \"" + id + "\""};
  }

  return attribute;
}
