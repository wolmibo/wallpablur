#include "wallpablur/exception.hpp"

#include <iostream>

#include <logcerr/log.hpp>

#include <gl/program.hpp>

#include <iconfigp/exception.hpp>
#include <iconfigp/format.hpp>



namespace {
  void print_exception(const gl::program_error& err) {
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



void print_exception(std::exception& ex) {
  if (const auto* excp = dynamic_cast<const gl::program_error*>(&ex); excp != nullptr) {
    print_exception(*excp);
  } else if (const auto* excp = dynamic_cast<const exception*>(&ex); excp != nullptr) {
    print_exception(*excp);
  } else {
    logcerr::error(ex.what());
  }
}

