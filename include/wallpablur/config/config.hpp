#ifndef WALLPABLUR_CONFIG_HPP_INCLUDED
#define WALLPABLUR_CONFIG_HPP_INCLUDED

#include "wallpablur/config/output.hpp"
#include "wallpablur/config/panel.hpp"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>



namespace config {

class config {
  public:
    config() = default;

    explicit config(std::string_view);


    [[nodiscard]] output output_config_for(std::string_view) const;



    [[nodiscard]] std::chrono::milliseconds poll_rate() const { return poll_rate_; }
    [[nodiscard]] std::chrono::milliseconds fade_out()  const { return fade_out_;  }
    [[nodiscard]] std::chrono::milliseconds fade_in()   const { return fade_in_;   }

    [[nodiscard]] bool disable_i3ipc() const { return disable_i3ipc_; }



    void poll_rate(std::chrono::milliseconds ms) { poll_rate_ = ms; }
    void fade_out (std::chrono::milliseconds ms) { fade_out_  = ms; }
    void fade_in  (std::chrono::milliseconds ms) { fade_in_   = ms; }

    void disable_i3ipc(bool disable) { disable_i3ipc_ = disable || disable_i3ipc_; }



  private:
    std::chrono::milliseconds poll_rate_    {500};
    std::chrono::milliseconds fade_out_     {0};
    std::chrono::milliseconds fade_in_      {0};
    bool                      disable_i3ipc_{false};
    std::vector<output>       outputs_;
    output                    default_output_;
};



[[nodiscard]] std::optional<std::filesystem::path> find_config_file();

}

#endif // WALLPABLUR_CONFIG_HPP_INCLUDED
