#ifndef WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED
#define WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED

#include "wallpablur/flat-map.hpp"
#include "wallpablur/texture-generator.hpp"
#include "wallpablur/wayland/geometry.hpp"

#include <memory>
#include <vector>

#include <gl/texture.hpp>



class texture_provider {
  public:
    using key = std::pair<wayland::geometry, config::brush>;



    explicit texture_provider(std::shared_ptr<egl::context> context) :
      texture_generator_{std::move(context)}
    {}



    [[nodiscard]] std::shared_ptr<gl::texture> get(const wayland::geometry&,
        const config::brush&);

    void cleanup();



  private:
    texture_generator                         texture_generator_;
    flat_map<key, std::weak_ptr<gl::texture>> cache_;

    [[nodiscard]] std::shared_ptr<gl::texture> create_texture(
        const wayland::geometry&, const config::brush&);
};

#endif // WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED
