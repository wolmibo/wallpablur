#include "wallpablur/config/config.hpp"
#include "wallpablur/wayland/client.hpp"
#include "wallpablur/wayland/output.hpp"

#include <stdexcept>

#include <logging/log.hpp>



wayland::output::output(wl_ptr<wl_output> op, client& parent) :
  client_{&parent},
  output_{std::move(op)},

  ready_{false}
{
  wl_output_add_listener(output_.get(), &output_listener_, this);
}



void wayland::output::render() {
  if (!context_) {
    throw std::runtime_error{"attempting to render without active egl context"};
  }

  context_->make_current();

  if (render_cb_ && !current_geometry_.empty()) {
    render_cb_(current_geometry_);
  }

  context_->swap_buffers();
}



void wayland::output::reset_frame_listener() {
  frame_callback_.reset(wl_surface_frame(surface_.get()));
  if (!frame_callback_) {
    throw std::runtime_error{"unable to set frame callback"};
  }

  wl_callback_add_listener(frame_callback_.get(), &callback_listener_, this);

  wl_surface_commit(surface_.get());
}



void wayland::output::callback_done_(void* data, wl_callback* /*cb*/, uint32_t /*ser*/) {
  auto* self = static_cast<output*>(data);

  if (self->update_cb_ && self->update_cb_(self->current_geometry_)) {
    self->render();
  }

  self->reset_frame_listener();
}




namespace {
  constexpr uint32_t anchor_all =
    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT  |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;



  [[nodiscard]] wl_ptr<zwlr_layer_surface_v1> create_layer_surface(
      zwlr_layer_shell_v1* layer_shell,
      wl_output*           output,
      wl_surface*          surface
  ) {
    const uint32_t layer = config::global_config().as_overlay() ?
      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY : ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;

    wl_ptr<zwlr_layer_surface_v1> layer_surface(zwlr_layer_shell_v1_get_layer_surface(
                                                  layer_shell, surface, output, layer,
                                                  "wallpablur-wallpaper"));

    if (!layer_surface) {
      throw std::runtime_error{"unable to create layer surface"};
    }

    zwlr_layer_surface_v1_set_size          (layer_surface.get(), 0, 0);
    zwlr_layer_surface_v1_set_anchor        (layer_surface.get(), anchor_all);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface.get(), -1);

    return layer_surface;
  }
}



void wayland::output::create_layer_surface() {
  logging::verbose("{}: creating surface", name());
  surface_.reset(wl_compositor_create_surface(client_->compositor()));

  if (!surface_) {
    throw std::runtime_error{"unable to create surface"};
  }



  logging::debug("{}: creating viewport", name());
  viewport_.reset(wp_viewporter_get_viewport(client_->viewporter(), surface_.get()));
  if (!viewport_) {
    logging::warn("{}: unable to create viewport", name());
  }



  logging::debug("{}: creating layer surface", name());
  layer_surface_ =
    ::create_layer_surface(client_->layer_shell(), output_.get(), surface_.get());

  zwlr_layer_surface_v1_add_listener(layer_surface_.get(),
      &layer_surface_listener_, this);



  wl_surface_commit(surface_.get());
}



void wayland::output::create_context() {
  logging::debug("{}: creating egl context", name());

  egl_window_.reset(wl_egl_window_create(surface_.get(),
        current_geometry_.pixel_width(), current_geometry_.pixel_height()));

  if (!egl_window_) {
    throw std::runtime_error{"unable to create egl window"};
  }

  context_ = std::make_shared<egl::context>(client_->context().share(egl_window_.get()));

  reset_frame_listener();
}



void wayland::output::output_mode_(
    void*      data,
    wl_output* /*output*/,
    uint32_t   /*flags*/,
    int32_t    width,
    int32_t    height,
    int32_t    /*refresh*/
) {
  auto& geometry = static_cast<output*>(data)->current_geometry_;
  geometry.pixel_width(width);
  geometry.pixel_height(height);
}



void wayland::output::output_done_(void* data, wl_output* /*output*/) {
  auto* self = static_cast<output*>(data);

  if (!self->layer_surface_) {
    self->create_layer_surface();
  }
}



void wayland::output::layer_surface_configure_(
  void*                   data,
  zwlr_layer_surface_v1*  zwlr_layer_surface,
  uint32_t                serial,
  uint32_t                width,
  uint32_t                /*height*/
) {
  auto* self = static_cast<output*>(data);

  self->current_geometry_.scale(static_cast<float>(self->current_geometry_.pixel_width())
      / static_cast<float>(width));

  if (self->first_configuration_) {
    self->create_context();
  } else {
    wl_egl_window_resize(
        self->egl_window_.get(),
        self->current_geometry_.pixel_width(),
        self->current_geometry_.pixel_height(),
        0, 0);
  }

  self->update_viewport();

  zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface, serial);

  if (self->ready_cb_) {
    self->ready_ = true;
    self->ready_cb_(*self);
    self->ready_cb_ = {};
  }

  if (self->first_configuration_) {
    self->render();
    self->first_configuration_ = false;
  }
}



void wayland::output::update_viewport() const {
  wp_viewport_set_destination(viewport_.get(),
      current_geometry_.logical_width(), current_geometry_.logical_height());

  wp_viewport_set_source(viewport_.get(),
      wl_fixed_from_int(0),
      wl_fixed_from_int(0),
      wl_fixed_from_int(current_geometry_.pixel_width()),
      wl_fixed_from_int(current_geometry_.pixel_height()));
}
