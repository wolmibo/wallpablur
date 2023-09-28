#ifndef WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED
#define WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED

#include "wallpablur/flat-map.hpp"
#include "wallpablur/texture-generator.hpp"

#include <memory>
#include <vector>



class texture_provider {
  public:
    explicit texture_provider(std::shared_ptr<egl::context>);



    [[nodiscard]] std::shared_ptr<gl::texture> get(const wayland::geometry&,
        const config::brush&);

    void cleanup();





  private:
    using key = std::pair<wayland::geometry, config::brush>;

    texture_generator                         texture_generator_;
    flat_map<key, std::weak_ptr<gl::texture>> cache_;
};

#endif // WALLPABLUR_TEXTURE_PROVIDER_HPP_INCLUDED
