#include <algorithm>
#include <filesystem>

#include <gl/texture.hpp>

#include <gdk-pixbuf/gdk-pixbuf.h>



namespace {
  struct gerror_destructor {
    void operator()(GError* error) { g_error_free(error); }
  };

  struct pixbuf_destructor {
    void operator()(GdkPixbuf* pixbuf) { g_object_unref(pixbuf); }
  };



  [[nodiscard]] gl::texture::format pixel_format(GdkPixbuf* pixbuf) {
    if (gdk_pixbuf_get_has_alpha(pixbuf) == TRUE) {
      return gl::texture::format::rgba8;
    }
    return gl::texture::format::rgb8;
  }
}



gl::texture load_to_gl_texture(const std::filesystem::path& path) {
  GError* error_unsafe = nullptr;

  std::unique_ptr<GdkPixbuf, pixbuf_destructor> pixbuf
    {gdk_pixbuf_new_from_file(path.string().c_str(), &error_unsafe)};

  std::unique_ptr<GError, gerror_destructor> error{error_unsafe};

  if (error) {
    throw std::runtime_error{"failed to decode " + path.string()
      + " using gdk-pixbuf:\n" + error->message};
  }



  int width  = gdk_pixbuf_get_width (pixbuf.get());
  int height = gdk_pixbuf_get_height(pixbuf.get());

  if (gdk_pixbuf_get_bits_per_sample(pixbuf.get()) != 8) {
    throw std::runtime_error{"failed to use " + path.string() + ":\n"
      "only images with 8 bits per color channel are supported"};
  }

  std::span<const guint8> pixels {
    gdk_pixbuf_read_pixels    (pixbuf.get()),
    gdk_pixbuf_get_byte_length(pixbuf.get())
  };

  std::vector<guint8> pixels_save_range;

  if (auto size = static_cast<size_t>(height)
                * static_cast<size_t>(gdk_pixbuf_get_rowstride(pixbuf.get()));
      size != pixels.size()) {

    pixels_save_range.resize(size);
    std::ranges::copy(pixels, pixels_save_range.begin());
    pixels = pixels_save_range;
  }

  return {width, height, std::as_bytes(pixels), pixel_format(pixbuf.get())};
}
