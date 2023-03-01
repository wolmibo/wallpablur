#ifndef WALLPABLUR_APPLICATION_HPP_INCLUDED
#define WALLPABLUR_APPLICATION_HPP_INCLUDED

#include "wallpablur/config/config.hpp"
#include "wallpablur/layout-painter.hpp"
#include "wallpablur/texture-provider.hpp"
#include "wallpablur/wayland/client.hpp"
#include "wallpablur/wm/change-token.hpp"
#include "wallpablur/wm/i3ipc.hpp"

#include <chrono>



struct application_args;

class application {
  public:
    application(const application_args&);

    [[nodiscard]] int run();



  private:
    std::optional<wm::i3ipc>          i3ipc_;
    wayland::client                   wayland_client_;
    std::shared_ptr<texture_provider> texture_provider_;

    struct output_data {
      std::string              name;
      layout_painter           painter;
      change_token<wm::layout> layout_source;
      float                    last_alpha;
    };

    std::vector<std::unique_ptr<output_data>> data_;

    std::chrono::high_resolution_clock::time_point                app_start_;
    std::optional<std::chrono::high_resolution_clock::time_point> exit_start_;



    [[nodiscard]] float alpha() const;
};

#endif // WALLPABLUR_APPLICATION_HPP_INCLUDED
