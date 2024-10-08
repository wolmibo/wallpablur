#include <wayland-egl.h>
#include "wallpablur/egl/context.hpp"

#include <array>
#include <string_view>
#include <utility>

#include <logcerr/log.hpp>



namespace {
  [[nodiscard]] std::string_view gl_message_source(GLenum source) {
    switch (source) {
      case GL_DEBUG_SOURCE_API:             return "API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "window system";
      case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
      case GL_DEBUG_SOURCE_THIRD_PARTY:     return "3rd party";
      case GL_DEBUG_SOURCE_APPLICATION:     return "application";

      case GL_DEBUG_SOURCE_OTHER:
      default:
        return "unknown source";
    }
  }



  [[nodiscard]] std::string_view gl_message_type(GLenum type) {
    switch (type) {
      case GL_DEBUG_TYPE_ERROR:               return "an error";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "undefined behavior";
      case GL_DEBUG_TYPE_PORTABILITY:         return "a portability issue";
      case GL_DEBUG_TYPE_PERFORMANCE:         return "a performance issue";
      case GL_DEBUG_TYPE_MARKER:              return "an annotation";
      case GL_DEBUG_TYPE_PUSH_GROUP:          return "group pushing";
      case GL_DEBUG_TYPE_POP_GROUP:           return "group popping";

      case GL_DEBUG_TYPE_OTHER:
      default:                                return "something";
    }
  }



  [[nodiscard]] logcerr::severity gl_severity(GLenum severity) {
    switch (severity) {
      case GL_DEBUG_SEVERITY_HIGH:         return logcerr::severity::error;
      case GL_DEBUG_SEVERITY_MEDIUM:       return logcerr::severity::warning;

      case GL_DEBUG_SEVERITY_LOW:
      case GL_DEBUG_SEVERITY_NOTIFICATION:
      default:                             return logcerr::severity::debug;
    }
  }



  void gl_debug_cb(GLenum source, GLenum type, GLuint id, GLenum severity,
      GLsizei /*length*/, const GLchar* message, const void* /*userptr*/) {

    if constexpr (logcerr::debugging_enabled()) {
      logcerr::print(gl_severity(severity), "GL {} reports {} ({}):\n{}",
          gl_message_source(source), gl_message_type(type), id, message);
    }
  }





  [[nodiscard]] constexpr std::string_view egl_error_to_string(EGLint code) {
    switch (code) {
      case EGL_SUCCESS:             return "success";
      case EGL_NOT_INITIALIZED:     return "not initialized";
      case EGL_BAD_ACCESS:          return "bad access";
      case EGL_BAD_ALLOC:           return "bad alloc";
      case EGL_BAD_ATTRIBUTE:       return "bad attribute";
      case EGL_BAD_CONTEXT:         return "bad context";
      case EGL_BAD_CONFIG:          return "bad config";
      case EGL_BAD_CURRENT_SURFACE: return "bad current surface";
      case EGL_BAD_DISPLAY:         return "bad display";
      case EGL_BAD_SURFACE:         return "bad surface";
      case EGL_BAD_MATCH:           return "bad match";
      case EGL_BAD_PARAMETER:       return "bad parameter";
      case EGL_BAD_NATIVE_PIXMAP:   return "bad native pixmap";
      case EGL_BAD_NATIVE_WINDOW:   return "bad native window";
      case EGL_CONTEXT_LOST:        return "context lost";

      default:                      return "<unknown>";
    }
  }



  [[nodiscard]] std::string format_egl_error_message(
    const std::string&   message,
    EGLint               code
  ) {
    return logcerr::format("{}: eglError({}, {:#X})",
      message,
      egl_error_to_string(code),
      code
    );
  }
}



egl::error::error(const std::string& message, EGLint code, std::source_location loc) :
  exception{format_egl_error_message(message, code), false, loc}
{}





void egl::context::enable_debugging() const {
  if constexpr (logcerr::debugging_enabled()) {
    make_current();

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_debug_cb, nullptr);
  }
}



class egl::context::display_wrapper {
  public:
    display_wrapper(const display_wrapper&) = delete;
    display_wrapper(display_wrapper&&)      = delete;
    display_wrapper& operator=(const display_wrapper&) = delete;
    display_wrapper& operator=(display_wrapper&&)      = delete;


    display_wrapper(NativeDisplayType display)
        : display_{eglGetDisplay(display)}
    {
      if (display_ == EGL_NO_DISPLAY) {
        throw egl::error{"unable to obtain egl display"};
      }

      EGLint major{0};
      EGLint minor{0};
      if (eglInitialize(display_, &major, &minor) == EGL_FALSE) {
        throw egl::error{"unable to initialize egl"};
      }
      logcerr::verbose("egl version {}.{}", major, minor);


      eglBindAPI(EGL_OPENGL_API);

      eglSwapInterval(display_, 0);
    }

    ~display_wrapper() {
      if (eglTerminate(display_) == EGL_FALSE) {
        logcerr::error(error{"unable to terminate egl display connection"}.what());
      }
    }

    EGLDisplay operator*() { return display_; }

  private:
    EGLDisplay display_;
};





void egl::context::make_current() const {
  if (eglMakeCurrent(**display_, surface_, surface_, context_) == EGL_FALSE) {
    throw error{"unable to make context current"};
  }
}



void egl::context::swap_buffers() const {
  if (eglSwapBuffers(**display_, surface_) == EGL_FALSE) {
    throw error{"unable to swap buffers"};
  }
}





namespace {
  [[nodiscard]] EGLConfig create_egl_config(EGLDisplay display) {
    EGLint count{0};
    if (eglGetConfigs(display, nullptr, 0, &count) != EGL_TRUE || count == 0) {
      throw egl::error{"unable to find egl configuration"};
    }

    const std::array<EGLint, 18> attrib = {
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_NONE,            EGL_NONE,
    };

    EGLConfig config{};
    if (eglChooseConfig(display, attrib.data(), &config, 1, &count) != EGL_TRUE
        || count != 1) {
      throw egl::error{"unable to choose egl configuration"};
    }

    return config;
  }



  [[nodiscard]] EGLSurface create_egl_surface(
      EGLDisplay       display,
      EGLConfig        config,
      NativeWindowType window
  ) {
    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);

    if (surface == EGL_NO_SURFACE) {
      throw egl::error{"unable to create egl surface"};
    }

    return surface;
  }



  [[nodiscard]] EGLContext create_egl_context(
      EGLDisplay display,
      EGLConfig  config,
      EGLContext shared
  ) {
    static constexpr std::array<EGLint, 6> cx = {
      EGL_CONTEXT_MAJOR_VERSION, 3,
      EGL_CONTEXT_MINOR_VERSION, 2,
      EGL_NONE,                  EGL_NONE,
    };

    EGLContext context = eglCreateContext(display, config, shared, cx.data());
    if (context == EGL_NO_CONTEXT) {
      throw egl::error{"unable to create egl context"};
    }

    return context;
  }
}



egl::context::context(NativeDisplayType display) :
  display_{std::make_shared<display_wrapper>(display)},
  config_ {create_egl_config(**display_)},
  surface_{EGL_NO_SURFACE},
  context_{create_egl_context(**display_, config_, EGL_NO_CONTEXT)}
{
  enable_debugging();
}



egl::context::context(
    std::shared_ptr<display_wrapper> display,
    EGLConfig                        config,
    EGLSurface                       surface,
    EGLContext                       context
) :
  display_{std::move(display)},
  config_ {config},
  surface_{surface},
  context_{context}
{
  enable_debugging();
}



egl::context::context(context&& rhs) noexcept :
  display_{std::exchange(rhs.display_, nullptr)},
  config_ {rhs.config_},
  surface_{std::exchange(rhs.surface_, EGL_NO_SURFACE)},
  context_{std::exchange(rhs.context_, EGL_NO_CONTEXT)}
{}



egl::context& egl::context::operator=(context&& rhs) noexcept {
  std::swap(display_, rhs.display_);
  std::swap(config_,  rhs.config_);
  std::swap(surface_, rhs.surface_);
  std::swap(context_, rhs.context_);

  return *this;
}



egl::context egl::context::share(NativeWindowType window) const {
  return context {
    display_,
    config_,
    create_egl_surface(**display_, config_, window),
    create_egl_context(**display_, config_, context_)
  };
}



egl::context::~context() {
  if (!display_) {
    return;
  }

  if (eglMakeCurrent(**display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
      == EGL_FALSE) {
    logcerr::error(error{"unable to unbind egl surface and context"}.what());
  }

  if (context_ != EGL_NO_CONTEXT) {
    if (eglDestroyContext(**display_, context_) == 0) {
      logcerr::error(error{"unable to destroy egl context"}.what());
    }
  }

  if (surface_ != EGL_NO_SURFACE) {
    if (eglDestroySurface(**display_, surface_) == 0) {
      logcerr::error(error{"unable to destroy egl surface"}.what());
    }
  }
}
