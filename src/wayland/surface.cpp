#include "wallpablur/wayland/surface.hpp"

#include "wallpablur/config/config.hpp"
#include "wallpablur/wayland/client.hpp"
#include "wallpablur/wayland/output.hpp"

#include <logcerr/log.hpp>





namespace {
  constexpr uint32_t anchor_all =
    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT  |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;



  [[nodiscard]] wl_ptr<zwlr_layer_surface_v1> create_layer_surface(
      zwlr_layer_shell_v1* layer_shell,
      wl_output*           output,
      wl_surface*          surface,
      const char*          name
  ) {
    const uint32_t layer = config::global_config().as_overlay() ?
      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY : ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;

    wl_ptr<zwlr_layer_surface_v1> layer_surface(zwlr_layer_shell_v1_get_layer_surface(
                                                  layer_shell, surface, output, layer,
                                                  name));

    if (!layer_surface) {
      throw std::runtime_error{"unable to create layer surface"};
    }

    zwlr_layer_surface_v1_set_size          (layer_surface.get(), 0, 0);
    zwlr_layer_surface_v1_set_anchor        (layer_surface.get(), anchor_all);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface.get(), -1);

    return layer_surface;
  }
}





wayland::surface::surface(std::string name, client& cl, output& op) :
  name_            {std::move(name)},
  client_          {&cl},
  output_          {&op},
  current_geometry_{output_->current_geometry_}
{
  logcerr::verbose("{}: creating surface", name_);
  surface_.reset(wl_compositor_create_surface(client_->compositor()));

  if (!surface_) {
    throw std::runtime_error{"unable to create surface"};
  }



  if (wl_ptr<wl_region> input{wl_compositor_create_region(client_->compositor())}) {
    wl_surface_set_input_region(surface_.get(), input.get());
  } else {
    logcerr::warn("unable to clear input region");
  }



  logcerr::debug("{}: creating viewport", name_);
  viewport_.reset(wp_viewporter_get_viewport(client_->viewporter(), surface_.get()));
  if (!viewport_) {
    logcerr::warn("{}: unable to create viewport", name_);
  }



  logcerr::debug("{}: creating layer surface", name_);
  layer_surface_ =
    ::create_layer_surface(client_->layer_shell(),
                           output_->output_.get(), surface_.get(), name_.c_str());

  zwlr_layer_surface_v1_add_listener(layer_surface_.get(),
      &layer_surface_listener_, this);



  wl_surface_commit(surface_.get());
}





void wayland::surface::create_context() {
  logcerr::debug("{}: creating egl context", name_);

  egl_window_.reset(wl_egl_window_create(surface_.get(),
        current_geometry_.physical_width(), current_geometry_.physical_height()));

  if (!egl_window_) {
    throw std::runtime_error{"unable to create egl window"};
  }

  context_ = std::make_shared<egl::context>(client_->context().share(egl_window_.get()));

  reset_frame_listener();
}





void wayland::surface::reset_frame_listener() {
  frame_callback_.reset(wl_surface_frame(surface_.get()));
  if (!frame_callback_) {
    throw std::runtime_error{"unable to set frame callback"};
  }

  wl_callback_add_listener(frame_callback_.get(), &callback_listener_, this);

  wl_surface_commit(surface_.get());
}





void wayland::surface::render() {
  if (!context_) {
    throw std::runtime_error{"attempting to render without active egl context"};
  }

  context_->make_current();

  if (output_->render_cb_ && !current_geometry_.empty()) {
    output_->render_cb_(current_geometry_);
  }

  context_->swap_buffers();
}





void wayland::surface::callback_done_(void* data, wl_callback* /*cb*/, uint32_t /*ser*/) {
  auto* self = static_cast<surface*>(data);

  if (self->output_->update_cb_ && self->output_->update_cb_(self->current_geometry_)) {
    self->render();
  }

  self->reset_frame_listener();
}





void wayland::surface::layer_surface_configure_(
  void*                   data,
  zwlr_layer_surface_v1*  zwlr_layer_surface,
  uint32_t                serial,
  uint32_t                width,
  uint32_t                height
) {
  auto* self = static_cast<surface*>(data);

  logcerr::debug("{}: configuring layer surface {}x{}", self->name_, width, height);

  self->current_geometry_.scale(
      static_cast<float>(self->current_geometry_.physical_width())
      / static_cast<float>(width));

  if (self->first_configuration_) {
    self->create_context();
  } else {
    wl_egl_window_resize(
        self->egl_window_.get(),
        self->current_geometry_.physical_width(),
        self->current_geometry_.physical_height(),
        0, 0);
  }

  self->update_viewport();

  zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface, serial);

  if (self->output_->ready_cb_) {
    self->ready_ = true;
    self->output_->ready_cb_(*(self->output_));
    self->output_->ready_cb_ = {};
  }

  if (self->first_configuration_) {
    self->render();
    self->first_configuration_ = false;
  }
}





void wayland::surface::update_viewport() const {
  logcerr::debug("{}: viewport transform {}x{}->{}x{}", name_,
      current_geometry_.physical_width(), current_geometry_.physical_height(),
      current_geometry_.logical_width(),  current_geometry_.logical_height());

  wp_viewport_set_destination(viewport_.get(),
      current_geometry_.logical_width(), current_geometry_.logical_height());

  wp_viewport_set_source(viewport_.get(),
      wl_fixed_from_int(0),
      wl_fixed_from_int(0),
      wl_fixed_from_int(current_geometry_.physical_width()),
      wl_fixed_from_int(current_geometry_.physical_height()));
}





void wayland::surface::update_geometry(const geometry& geo) {
  current_geometry_ = geo;
}