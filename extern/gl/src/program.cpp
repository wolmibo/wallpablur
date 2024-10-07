#include "gl/program.hpp"

#include <charconv>
#include <stdexcept>




namespace {
  template<typename Fnc>
  [[nodiscard]] bool get_status(GLint id, Fnc&& f, GLenum query) {
    GLint status{GL_FALSE};
    std::forward<Fnc>(f)(id, query, &status);
    return status != GL_FALSE && id != 0;
  }



  template<typename FncIv, typename FncIL>
  [[nodiscard]] std::string get_message(GLint id, FncIv&& iv, FncIL&& il) {
    GLint length{0};
    std::forward<FncIv>(iv)(id, GL_INFO_LOG_LENGTH, &length);

    if (length < 0) {
      return "<unable to obtain message>";
    }

    std::string buffer(static_cast<size_t>(length), ' ');
    std::forward<FncIL>(il)(id, buffer.size(), nullptr, buffer.data());

    while (!buffer.empty() && buffer.back() == 0) {
      buffer.pop_back();
    }

    return buffer;
  }



  [[nodiscard]] std::string get_message(GLint id, gl::shader_type type) {
    switch (type) {
      case gl::shader_type::vertex:
      case gl::shader_type::fragment:
        return get_message(id, glGetShaderiv, glGetShaderInfoLog);
      case gl::shader_type::linking:
        return get_message(id, glGetProgramiv, glGetProgramInfoLog);
    }
    return "<unable to obtain message>";
  }



  [[nodiscard]] constexpr std::string shader_type_to_string(gl::shader_type type) {
    switch (type) {
      case gl::shader_type::vertex:   return "compile vertex shader";
      case gl::shader_type::fragment: return "compile fragment shader";
      case gl::shader_type::linking:  return "link program";
    }

    return "unknown shader action";
  }





  [[nodiscard]] size_t str_to_size_t(std::string_view str) {
    size_t value{0};
    std::from_chars(str.data(), str.data() + str.size(), value);
    return value;
  }





  [[nodiscard]] std::string_view trim_front(std::string_view message) {
    while (!message.empty() &&
           std::string_view{" \t\n\r"}.find(message.front()) != std::string_view::npos) {
      message.remove_prefix(1);
    }
    return message;
  }



  [[nodiscard]] gl::program_error::message message_from_string(std::string_view line) {
    auto end = line.find("):");
    auto start = line.find(':');

    if (end == std::string_view::npos || start > end) {
      return {
        .complete_message = std::string{line},
        .row              = 0,
        .column           = 0,
        .message          = std::string{line}
      };
    }

    auto split = line.find('(', start + 1);

    auto message = trim_front(line.substr(end + 2));

    if (message.starts_with("error:")) {
      message.remove_prefix(6);
      message = trim_front(message);
    }

    return {
      .complete_message = std::string{line},
      .row              = str_to_size_t(line.substr(start + 1, split - start - 1)) - 1,
      .column           = str_to_size_t(line.substr(split + 1, end   - split - 1)) - 1,
      .message          = std::string{message}
    };
  }





  [[nodiscard]] std::vector<gl::program_error::message> split_messages(
      std::string_view in
  ) {
    std::vector<gl::program_error::message> messages;

    for (auto pos = in.find('\n'); !in.empty(); pos = in.find('\n')) {
      if (auto line = trim_front(in.substr(0, pos)); !line.empty()) {
        messages.emplace_back(message_from_string(line));
      }
      if (pos == std::string_view::npos) {
        return messages;
      }
      in.remove_prefix(pos + 1);
    }

    return messages;
  }
}





gl::program_error::program_error(GLint id, shader_type type, std::string source) :
  std::runtime_error{"failed to " + shader_type_to_string(type)},
  type_    {type},
  source_  {std::move(source)},
  messages_{split_messages(get_message(id, type))}
{}



std::optional<std::string_view> gl::program_error::source() const {
  if (source_.empty()) {
    return {};
  }
  return source_;
}






namespace {
  void assert_object(GLint id, gl::shader_type type, std::string_view source) {
    switch (type) {
      case gl::shader_type::vertex:
      case gl::shader_type::fragment:
        if (!get_status(id, glGetShaderiv, GL_COMPILE_STATUS)) {
          throw gl::program_error{id, type, std::string{source}};
        }
        break;

      case gl::shader_type::linking:
        if (!get_status(id, glGetProgramiv, GL_LINK_STATUS)) {
          throw gl::program_error{id, type, std::string{source}};
        }
        break;
    }
  }





  class shader {
    public:
      shader(gl::shader_type type, std::string_view source) :
        shader_{glCreateShader(std::to_underlying(type))}
      {
        GLint length       = source.length();
        const GLchar* data = source.data();

        glShaderSource(shader_.get(), 1, &data, &length);
        glCompileShader(shader_.get());

        assert_object(shader_.get(), type, source);
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
  shader vertex  {gl::shader_type::vertex,   vs};
  glAttachShader(program_.get(), vertex.get());

  shader fragment{gl::shader_type::fragment, fs};
  glAttachShader(program_.get(), fragment.get());


  glLinkProgram(program_.get());

  assert_object(program_.get(), gl::shader_type::linking, "");

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
