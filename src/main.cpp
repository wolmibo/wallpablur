#include "wallpablur/application.hpp"
#include "wallpablur/application-args.hpp"

#include <cstdlib>

#include <logcerr/log.hpp>



int main(int argc, char** argv) {
  std::span arg{argv, static_cast<size_t>(argc)};

  try {
    if (auto args = application_args::parse(arg)) {
      application app{*args};
      return app.run();
    }
  } catch (std::exception& ex) {
    logcerr::error(ex.what());
    return EXIT_FAILURE;
  } catch (bool) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
