#include "wallpablur/application-args.hpp"
#include "wallpablur/application.hpp"

#include <array>

#include <iostream>
#include <logging/log.hpp>

#include <getopt.h>



namespace {
  constexpr std::string_view help_text =
R"(Usage: {0} [OPTIONS...] [<image>]

Available options:
  -h, --help                  show this help and exit
  -v, --version               show version information and exit
  -V, --verbose               enable verbose logging (use twice for debug output)

  -s, --socket <path>         connect to <socket> instead of $SWAYSOCK or $I3SOCK
  -W, --disable-i3ipc         disable i3ipc and only draw the wallpaper

  -p, --poll-rate <ms>        how often to poll the window manager for updates
  -f, --fade-out <ms>         how long to fade out after receiving SIGTERM or SIGINT
  -F, --fade-in <ms>          how long to fade in


If an image path is provided, all outputs will use this image as wallpaper and a blurred
version as background. Additional options in this case:

  -b, --blur <rad>[:<iter>]   perform <iter> blur steps with radius <rad> (default: 96:2)


If no image path is provided, a config file is searched in the following locations:
  1. $WALLPABLUR_CONFIG
  2. $XDG_CONFIG_HOME/wallpablur/config.ini
  3. $HOME/.config/wallpablur/config.ini

Alternatively, you can set a configuration via the following options:

  -c, --config <path>         load configuration from file at <path>
  -i, --config-string <conf>  load configuration from string <conf>
)";



  const std::array long_options = {
    option{"help",          no_argument,       nullptr, 'h'},
    option{"version",       no_argument,       nullptr, 'v'},
    option{"verbose",       no_argument,       nullptr, 'V'},

    option{"config",        required_argument, nullptr, 'c'},
    option{"config-string", required_argument, nullptr, 'i'},

    option{"socket",        required_argument, nullptr, 's'},
    option{"disable-i3ipc", no_argument,       nullptr, 'W'},

    option{"poll-rate",     required_argument, nullptr, 'p'},
    option{"fade-out",      required_argument, nullptr, 'f'},
    option{"fade-in",       required_argument, nullptr, 'F'},

    option{"blur",          required_argument, nullptr, 'b'},

    option{nullptr,         0,                 nullptr,   0}
  };

  const char* short_options = "hvVc:i:s:Wp:f:F:b:";





  [[nodiscard]] blur_args blur_args_from_string(std::string_view input) {
    blur_args blur;
    if (auto pos = input.find(':'); pos != std::string_view::npos) {
      blur.radius     = std::stoi(std::string{input.substr(0, pos)});
      blur.iterations = std::stoi(std::string{input.substr(pos+1)});
    } else {
      blur.radius = std::stoi(std::string{input});
    }
    return blur;
  }



  [[nodiscard]] application_args parse_args(std::span<char*> arg) {
    application_args args;
    int c{0};

    while ((c = getopt_long(arg.size(), arg.data(),
            short_options, long_options.data(), nullptr)) != -1) {

      switch (c) {
        case 'h': args.help    = true; break;
        case 'v': args.version = true; break;
        case 'V': args.verbose++;      break;

        case 'c': args.config_path   = optarg; break;
        case 'i': args.config_string = optarg; break;

        case 's': args.socket_path   = optarg; break;
        case 'W': args.disable_i3ipc = true;   break;

        case 'p': args.poll_rate = std::chrono::milliseconds{std::stoi(optarg)}; break;
        case 'f': args.fade_out  = std::chrono::milliseconds{std::stoi(optarg)}; break;
        case 'F': args.fade_in   = std::chrono::milliseconds{std::stoi(optarg)}; break;

        case 'b': args.blur = blur_args_from_string(optarg); break;

        default: break;
      }
    }

    if (static_cast<size_t>(optind) < arg.size()
        && arg[optind] != nullptr && *arg[optind] != 0) {
      args.image = arg[optind];
    }

    return args;
  }



  void init_logging(int verbose) {
    logging::merge_after(0);

    if (verbose == 0) {
      logging::output_level(logging::severity::log);
    } else if (verbose == 1) {
      logging::output_level(logging::severity::verbose);
    } else {
      logging::output_level(logging::severity::debug);
    }
  }
}



[[nodiscard]] std::string blur_args::to_config(const std::filesystem::path& path) const {
  return logging::format(
      R"([wallpaper]path="{}"[background]filter=blur;radius={};iterations={})",
      path.string(), radius, iterations);
}




std::string_view version_info();

std::optional<application_args> application_args::parse(std::span<char*> arg) {
  auto args = parse_args(arg);

  if (args.help) {
    logging::print_raw_sync(std::cout, logging::format(help_text,
          std::filesystem::path{arg[0]}.filename().string()));
    return {};
  }

  if (args.version) {
    logging::print_raw_sync(std::cout, version_info());
    return {};
  }

  init_logging(args.verbose);

  return args;
}
