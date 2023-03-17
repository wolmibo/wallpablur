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

    [[nodiscard]] bool     disable_i3ipc() const { return disable_i3ipc_; }
    [[nodiscard]] bool     as_overlay()    const { return as_overlay_;    }
    [[nodiscard]] float    opacity()       const { return opacity_;       }
    [[nodiscard]] uint16_t gl_samples()    const { return gl_samples_;    }



    void poll_rate(std::chrono::milliseconds ms) { poll_rate_ = ms; }
    void fade_out (std::chrono::milliseconds ms) { fade_out_  = ms; }
    void fade_in  (std::chrono::milliseconds ms) { fade_in_   = ms; }

    void disable_i3ipc(bool  disable) { disable_i3ipc_ = disable || disable_i3ipc_; }
    void as_overlay   (bool  overlay) { as_overlay_    = overlay; }
    void opacity      (float opacity) { opacity_       = opacity; }



  private:
    std::chrono::milliseconds poll_rate_    {500};
    std::chrono::milliseconds fade_out_     {0};
    std::chrono::milliseconds fade_in_      {0};
    bool                      disable_i3ipc_{false};
    bool                      as_overlay_   {false};
    float                     opacity_      {1.f};
    uint16_t                  gl_samples_   {0};



    std::vector<output>       outputs_;
    output                    default_output_;
};

[[nodiscard]] config& global_config();
void global_config(config&&);



[[nodiscard]] std::optional<std::filesystem::path> find_config_file();

}

#endif // WALLPABLUR_CONFIG_HPP_INCLUDED
