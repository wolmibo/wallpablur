#include "wallpablur/application.hpp"
#include "wallpablur/application-args.hpp"
#include "wallpablur/exception.hpp"

#include <cstdlib>
#include <iostream>

#include <iconfigp/format.hpp>

#include <logcerr/log.hpp>

#include <gl/program.hpp>




namespace {
  void print_exception(const exception& ex) {
    if (auto location = ex.location()) {
      logcerr::error("{}\n{}:{} in {}",
          ex.what(),
          location->file_name(),
          location->line(),
          location->function_name()
      );
    } else {
      logcerr::error(ex.what());
    }
  }


  void print_gl_error(const gl::program_error& err) {
    logcerr::error(err.what());
    for (const auto& message: err.messages()) {
      logcerr::print_raw_sync(std::cerr, message.message + "\n");

      if (auto src = err.source()) {
        auto offset = iconfigp::line_offset(*src, message.row).value_or(0)
                        + message.column;

        logcerr::print_raw_sync(std::cerr,
            iconfigp::highlight_text_segment(*src, offset, 0,
              logcerr::is_colored() ? iconfigp::message_color::error
                                    : iconfigp::message_color::none));
      }
    }
  }
}



int main(int argc, char** argv) {
  std::span arg{argv, static_cast<size_t>(argc)};

  try {
    if (auto args = application_args::parse(arg)) {
      application app{*args};
      return app.run();
    }
  } catch (const gl::program_error& err) {
    print_gl_error(err);
    return EXIT_FAILURE;
  } catch (const exception& ex) {
    print_exception(ex);
    return EXIT_FAILURE;
  } catch (std::exception& ex) {
    logcerr::error(ex.what());
    return EXIT_FAILURE;
  } catch (bool) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
