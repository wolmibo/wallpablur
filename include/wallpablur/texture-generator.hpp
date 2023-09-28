#ifndef WALLPABLUR_TEXTURE_GENERATOR_HPP_INCLUDED
#define WALLPABLUR_TEXTURE_GENERATOR_HPP_INCLUDED

#include "wallpablur/egl/context.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/flat-map.hpp"
#include "wallpablur/wayland/geometry.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gl/mesh.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>



class texture_generator {
  public:
    texture_generator(const texture_generator&) = delete;
    texture_generator(texture_generator&&)      = default;
    texture_generator& operator=(const texture_generator&) = delete;
    texture_generator& operator=(texture_generator&&)      = default;

    ~texture_generator();

    explicit texture_generator(std::shared_ptr<egl::context>);



    [[nodiscard]] gl::texture generate(const wayland::geometry&,
        const config::brush&) const;

    [[nodiscard]] gl::texture generate_from_existing(const gl::texture&,
        const wayland::geometry&, std::span<const config::filter>) const;



  private:
    std::shared_ptr<egl::context>         context_;

    gl::mesh                              quad_;
    gl::program                           draw_texture_;

    enum class shader {
      box_blur,
      invert,
    };
    mutable flat_map<shader, gl::program> filter_shader_cache_;



    [[nodiscard]] gl::texture create_base_texture(
        const wayland::geometry&, const config::brush&) const;

    [[nodiscard]] gl::texture apply_filter(
        const gl::texture&, const config::filter&, const wayland::geometry&) const;
};

#endif // WALLPABLUR_TEXTURE_GENERATOR_HPP_INCLUDED
