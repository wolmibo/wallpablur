#ifndef WALLPABLUR_APPLICATION_ARGS_HPP_INCLUDED
#define WALLPABLUR_APPLICATION_ARGS_HPP_INCLUDED

#include <chrono>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>



struct blur_args {
  unsigned int radius    {96};
  unsigned int iterations{2};

  [[nodiscard]] std::string to_config(const std::filesystem::path&) const;
};



struct application_args {
  bool help    {false};
  bool version {false};
  int  verbose {0};

  std::optional<std::filesystem::path>     config_path;
  std::optional<std::string_view>          config_string;

  std::optional<std::filesystem::path>     socket_path;
  bool                                     disable_i3ipc{false};

  std::optional<std::chrono::milliseconds> poll_rate;
  std::optional<std::chrono::milliseconds> fade_out;
  std::optional<std::chrono::milliseconds> fade_in;

  blur_args                                blur;
  std::optional<std::filesystem::path>     image;



  [[nodiscard]] static std::optional<application_args> parse(std::span<char*>);
};

#endif // WALLPABLUR_APPLICATION_ARGS_HPP_INCLUDED
