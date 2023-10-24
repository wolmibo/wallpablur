#include "wallpablur/wayland/surface.hpp"
#include "wallpablur/wayland/output.hpp"

#include "wallpablur/config/config.hpp"
#include "wallpablur/wayland/client.hpp"

#include <stdexcept>

#include <logcerr/log.hpp>




wayland::output::~output() = default;



wayland::output::output(wl_ptr<wl_output> op, client& parent) :
  client_{&parent},
  output_{std::move(op)}
{
  wl_output_add_listener(output_.get(), &output_listener_, this);
}





bool wayland::output::ready() const {
  if (config::global_config().clipping()) {
    return wallpaper_surface_ && wallpaper_surface_->ready() &&
      clipping_surface_ && clipping_surface_->ready();
  }

  return wallpaper_surface_ && wallpaper_surface_->ready();
}





const wayland::surface& wayland::output::wallpaper_surface() const {
  if (!wallpaper_surface_) {
    throw std::runtime_error{"wallpaper surface has not been created yet"};
  }
  return *wallpaper_surface_;
}



const wayland::surface& wayland::output::clipping_surface() const {
  if (!clipping_surface_) {
    throw std::runtime_error{"clipping surface has not been created yet"};
  }
  return *clipping_surface_;
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

  self->current_geometry_.physical_width(width);
  self->current_geometry_.physical_height(height);

  if (self->wallpaper_surface_) {
    self->wallpaper_surface_->update_geometry(self->current_geometry_);
  }

  if (self->clipping_surface_) {
    self->clipping_surface_->update_geometry(self->current_geometry_);
  }
}





void wayland::output::output_done_(void* data, wl_output* /*output*/) {
  auto* self = static_cast<output*>(data);
  logcerr::debug("{}: output done {}x{}", self->name(),
      self->current_geometry_.physical_width(),
      self->current_geometry_.physical_height());

  if (!self->wallpaper_surface_) {
    self->wallpaper_surface_ = std::make_unique<surface>(
        "wallpablur-wallpaper." + std::string{self->name()}, *self->client_, *self,
        config::global_config().as_overlay());
  }

  if (!self->clipping_surface_ && config::global_config().clipping()) {
    self->clipping_surface_ = std::make_unique<surface>(
        "wallpablur-clipping." + std::string{self->name()}, *self->client_, *self, true);
  }
}
