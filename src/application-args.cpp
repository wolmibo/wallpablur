#include "wallpablur/application-args.hpp"
#include "wallpablur/application.hpp"
#include "wallpablur/config/config.hpp"

#include <array>
#include <iostream>
#include <fstream>

#include <iconfigp/find-config.hpp>
#include <iconfigp/path.hpp>

#include <logcerr/log.hpp>

#include <getopt.h>
#include <utility>



namespace {
  constexpr std::string_view help_text =
R"(Usage: {0} [OPTIONS...] [<image>]

Available options:
  -h, --help                  show this help and exit
  -v, --version               show version information and exit
  -V, --verbose               enable verbose logcerr (use twice for debug output)

  -s, --socket <path>         connect to <socket> instead of $SWAYSOCK or $I3SOCK
  -W, --disable-i3ipc         disable i3ipc and only draw the wallpaper

  -p, --poll-rate <ms>        how often to poll the window manager for updates
  -f, --fade-out <ms>         how long to fade out after receiving SIGTERM or SIGINT
  -F, --fade-in <ms>          how long to fade in

  -c, --config <path>         load configuration from file at <path>
  -i, --config-string <conf>  load configuration from string <conf>

If an image path is provided, all outputs will use this image as wallpaper and a blurred
version as background. Additional options in this case:

  -b, --blur <rad>[:<iter>]   perform <iter> blur steps with radius <rad> (default: 96:2)
)";



  enum class flags : int {
    as_overlay = 1000,
    opacity    = 1001,
  };


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

    option{"as-overlay", no_argument,       nullptr, std::to_underlying(flags::as_overlay)},
    option{"opacity",    required_argument, nullptr, std::to_underlying(flags::opacity)},

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



  void overwrite_global_config_from_args(std::span<char*> arg) {
    int c{0};
    auto& cfg = config::global_config();
    optind = 1;

    while ((c = getopt_long(arg.size(), arg.data(),
            short_options, long_options.data(), nullptr)) != -1) {

      switch (c) {
        case 'W': cfg.disable_i3ipc(true);   break;

        case 'p': cfg.poll_rate(std::chrono::milliseconds{std::stoi(optarg)}); break;
        case 'f': cfg.fade_out (std::chrono::milliseconds{std::stoi(optarg)}); break;
        case 'F': cfg.fade_in  (std::chrono::milliseconds{std::stoi(optarg)}); break;


        case std::to_underlying(flags::as_overlay):
          cfg.as_overlay(true);
          break;

        case std::to_underlying(flags::opacity):
          cfg.opacity(std::stof(optarg));
          break;

        default: break;
      }
    }
  }



  void init_logcerr(int verbose) {
    logcerr::merge_after(0);

    if (verbose == 0) {
      logcerr::output_level(logcerr::severity::log);
    } else if (verbose == 1) {
      logcerr::output_level(logcerr::severity::verbose);
    } else {
      logcerr::output_level(logcerr::severity::debug);
    }
  }




  [[nodiscard]] std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::ate);
    if (!input) {
      throw std::runtime_error{logcerr::format("unable to read file at \"{}\"",
          path.string())};
    }

    auto size = input.tellg();
    std::string buffer;
    buffer.resize(size);

    input.seekg(0);

    input.read(buffer.data(), buffer.size());

    return buffer;
  }



  [[nodiscard]] config::config config_from_args(const application_args& arg) {
    if (arg.image) {
      logcerr::verbose("using generic config");
      return config::config{arg.blur.to_config(*arg.image)};
    }

    if (arg.config_string) {
      logcerr::verbose("using config string from arguments");
      return config::config(*arg.config_string);
    }

    if (auto path = arg.config_path.or_else([](){
          return iconfigp::find_config_file("WallpaBlur");
        })) {
      logcerr::verbose("using config located at \"{}\"", path->string());
      iconfigp::preferred_root_path(path->parent_path());
      return config::config(read_file(*path));
    }

    logcerr::verbose("using default config");
    return config::config{};
  }
}



[[nodiscard]] std::string blur_args::to_config(const std::filesystem::path& path) const {
  return logcerr::format(
      R"([wallpaper]path="{}"[background]filter=blur;radius={};iterations={})",
      path.string(), radius, iterations);
}




std::string_view version_info();

std::optional<application_args> application_args::parse(std::span<char*> arg) {
  auto args = parse_args(arg);

  if (args.help) {
    logcerr::print_raw_sync(std::cout, logcerr::format(help_text,
          std::filesystem::path{arg[0]}.filename().string()));
    return {};
  }

  if (args.version) {
    logcerr::print_raw_sync(std::cout, version_info());
    return {};
  }

  init_logcerr(args.verbose);

  config::global_config(config_from_args(args));

  overwrite_global_config_from_args(arg);

  return args;
}
