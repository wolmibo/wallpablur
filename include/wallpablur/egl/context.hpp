#ifndef WALLPABLUR_EGL_CONTEXT_HPP_INCLUDED
#define WALLPABLUR_EGL_CONTEXT_HPP_INCLUDED

#include "wallpablur/exception.hpp"

#include <source_location>
#include <string>

#include <epoxy/gl.h>

#include <EGL/egl.h>



namespace egl {

class error : public exception {
  public:
    error(
      const std::string&,
      EGLint               = eglGetError(),
      std::source_location = std::source_location::current()
    );
};



class context {
  public:
    context(const context&) = delete;
    context(context&&) noexcept;
    context& operator=(const context&) = delete;
    context& operator=(context&&) noexcept;

    ~context();

    explicit context(NativeDisplayType);



    [[nodiscard]] context share(NativeWindowType) const;



    void make_current() const;
    void swap_buffers() const;



  private:
    class display_wrapper;
    std::shared_ptr<display_wrapper> display_;

    EGLConfig  config_;
    EGLSurface surface_{EGL_NO_SURFACE};
    EGLContext context_{EGL_NO_CONTEXT};

    context(std::shared_ptr<display_wrapper>, EGLConfig, EGLSurface, EGLContext);

    void enable_debugging() const;
};

}

#endif // WALLPABLUR_EGL_CONTEXT_HPP_INCLUDED
