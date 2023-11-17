#include "wallpablur/wayland/surface.hpp"
#include "wallpablur/wayland/output.hpp"

#include "wallpablur/wayland/client.hpp"

#include <logcerr/log.hpp>




wayland::output::~output() = default;



wayland::output::output(wl_ptr<wl_output> op, client& parent) :
  client_           {&parent},
  output_           {std::move(op)}
{
  wl_output_add_listener(output_.get(), &output_listener_, this);
}





void wayland::output::output_name_(void* data, wl_output* /*output*/, const char* name) {
  static_cast<output*>(data)->name_ = name;
}





void wayland::output::output_mode_(
    void*      data,
    wl_output* /*output*/,
    uint32_t   /*flags*/,
    int32_t    width,
    int32_t    height,
    int32_t    /*refresh*/
) {
  auto* self = static_cast<output*>(data);

  self->current_size_.x = width;
  self->current_size_.y = height;

  if (self->size_cb_) {
    self->size_cb_(self->current_size_);
  }
}





void wayland::output::output_done_(void* data, wl_output* /*output*/) {
  auto* self = static_cast<output*>(data);
  logcerr::debug("{}: output done {}x{}", self->name(),
      self->current_size_.x,
      self->current_size_.y);

  if (self->done_cb_) {
    self->done_cb_();
  }
}





std::unique_ptr<wayland::surface> wayland::output::create_surface(
    std::string name,
    bool        overlay
) {
  return std::make_unique<wayland::surface>(std::move(name), *client_, *this, overlay);
}
