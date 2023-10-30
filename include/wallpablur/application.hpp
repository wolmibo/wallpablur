#ifndef WALLPABLUR_APPLICATION_HPP_INCLUDED
#define WALLPABLUR_APPLICATION_HPP_INCLUDED

#include "wallpablur/flat-map.hpp"
#include "wallpablur/output.hpp"
#include "wallpablur/texture-provider.hpp"
#include "wallpablur/wayland/client.hpp"
#include "wallpablur/wm/change-token.hpp"
#include "wallpablur/wm/i3ipc.hpp"

#include <chrono>



struct application_args;

class output;

class application {
  public:
    application(const application_args&);

    [[nodiscard]] int run();



    [[nodiscard]] float alpha() const;

    [[nodiscard]] std::shared_ptr<::texture_provider> texture_provider();

    [[nodiscard]] change_token<workspace> layout_token(std::string_view);



  private:
    using time_point = std::chrono::high_resolution_clock::time_point;

    std::optional<wm::i3ipc>                    i3ipc_;
    wayland::client                             wayland_client_;
    std::shared_ptr<::texture_provider>         texture_provider_;

    flat_map<uint32_t, std::unique_ptr<output>> outputs_;

    time_point                                  app_start_;
    std::optional<time_point>                   exit_start_;
};



[[nodiscard]] application& app();

#endif // WALLPABLUR_APPLICATION_HPP_INCLUDED
