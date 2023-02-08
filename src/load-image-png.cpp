#include "wallpablur/config/filter.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gl/texture.hpp>

#include <logging/log.hpp>

#include <png.h>



namespace {
  [[noreturn]] void png_error(png_structp /*png*/, png_const_charp msg) {
    throw std::runtime_error{logging::format("unable to decode png: {}", msg)};
  }

  void png_warning(png_structp /*png*/, png_const_charp msg) {
    logging::warn("during decoding of png:\n{}", msg);
  }



  struct png_guard {
    png_structp ptr {nullptr};
    png_infop   info{nullptr};

    png_guard() :
      ptr{png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)}
    {
      if (ptr == nullptr) {
        throw std::runtime_error{"unable to create png read struct"};
      }

      png_set_error_fn(ptr, nullptr, png_error, png_warning);

      info = png_create_info_struct(ptr);

      if (info == nullptr) {
        png_destroy_read_struct(&ptr, nullptr, nullptr);
        throw std::runtime_error{"unable to create png info struct"};
      }
    }

    png_guard(const png_guard&) = delete;
    png_guard(png_guard&&)      = delete;
    png_guard&operator= (const png_guard&) = delete;
    png_guard&operator= (png_guard&&)      = delete;

    ~png_guard() {
      png_destroy_read_struct(&ptr, &info, nullptr);
    }
  };



  struct image {
    GLsizei width;
    GLsizei height;

    std::vector<png_byte> data;



    [[nodiscard]] std::span<png_byte> row(size_t i) {
      return std::span{data}.subspan(i * width * 4);
    }
  };



  struct file_destructor {
    void operator()(FILE* f) const {
      fclose(f); // NOLINT(*owning-memory)
    }
  };

  using file_ptr = std::unique_ptr<FILE, file_destructor>;

  [[nodiscard]] file_ptr open_file(const std::filesystem::path& path) {
    file_ptr fp{fopen(path.string().c_str(), "rb")};
    if (!fp) {
      throw std::runtime_error{
        logging::format("unable to open \"{}\" for reading", path.string())};
    }

    return fp;
  }



  void force_rgba8(png_structp png) {
    png_set_expand(png);
    png_set_gray_to_rgb(png);
    png_set_strip_16(png);
    png_set_alpha_mode(png, PNG_ALPHA_PREMULTIPLIED, PNG_GAMMA_LINEAR);
    png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
  }



  [[nodiscard]] image load_png(const std::filesystem::path& path) {
    logging::verbose("loading png file \"{}\"", path.string());
    png_guard png;

    auto fp = open_file(path);
    png_init_io(png.ptr, fp.get());

    png_read_info(png.ptr, png.info);

    image img{
      .width  = static_cast<GLsizei>(png_get_image_width(png.ptr, png.info)),
      .height = static_cast<GLsizei>(png_get_image_height(png.ptr, png.info)),
      .data   = std::vector<png_byte>(static_cast<size_t>(img.width) * img.height * 4)
    };

    force_rgba8(png.ptr);

    png_read_update_info(png.ptr, png.info);

    if (static_cast<GLsizei>(png_get_rowbytes(png.ptr, png.info)) != img.width * 4) {
      throw std::runtime_error{"error setting up png decoder: stride mismatch"};
    }

    int passes = png_set_interlace_handling(png.ptr);

    for (int p = 0; p < passes; ++p) {
      for (GLsizei y = 0; y < img.height; ++y) {
        png_read_row(png.ptr, img.row(y).data(), nullptr);
      }
    }

    return img;
  }
}



gl::texture load_to_gl_texture(const std::filesystem::path& path) {
  auto image = load_png(path);

  return {image.width, image.height, std::as_bytes(std::span{image.data}),
    gl::texture::format::rgba8};
}
