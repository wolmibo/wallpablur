#include "wallpablur/output.hpp"

#include "wallpablur/application.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/wayland/output.hpp"
#include "wallpablur/wayland/surface.hpp"



output::~output() = default;



output::output(std::unique_ptr<wayland::output> wl_output) :
  wl_output_{std::move(wl_output)}
{
  wl_output_->set_done_cb([this](){
    if (painter_) {
      return;
    }

    layout_token_ = app().layout_token(wl_output_->name());

    auto config = config::global_config().output_config_for(wl_output_->name());

    bool clipping = config.clipping;

    painter_.emplace(std::move(config));
    painter_->update_geometry(wl_output_->current_geometry());

    create_surfaces(clipping);
    setup_surfaces();
  });

  wl_output_->set_geometry_cb([this](const wayland::geometry& geo) {
    if (wallpaper_surface_) {
      wallpaper_surface_->update_geometry(geo);
    }

    if (clipping_surface_) {
      clipping_surface_->update_geometry(geo);
    }

    if (painter_) {
      painter_->update_geometry(geo);
    }
  });
}





void output::create_surfaces(bool clipping) {
  if (!wallpaper_surface_) {
    wallpaper_surface_ = wl_output_->create_surface(
        "wallpablur-wallpaper." + std::string{wl_output_->name()},
        config::global_config().as_overlay()
    );
  }

  if (!clipping_surface_ && clipping && !config::global_config().as_overlay()) {
    clipping_surface_ = wl_output_->create_surface(
        "wallpablur-clipping." + std::string{wl_output_->name()},
        true
    );
  }
}





void output::update() {
  if (layout_token_.changed()) {
    last_layout_ = *layout_token_.get();
    last_layout_id_++;
    surface_updated_.reset();

    if (clipping_surface_) {
      if (painter_->update_rounded_corners(last_layout_, last_layout_id_)) {
        clipping_surface_->show();
      } else {
        clipping_surface_->hide();
      }
    }
  }
}





namespace {
  [[nodiscard]] bool alpha_changed(float last, float current) {
    return (current >= 1.f && last < 1.f) || std::abs(last - current) > 1.f / 255.f;
  }
}



void output::setup_surfaces() {
  if (wallpaper_surface_) {
    wallpaper_surface_->set_context_cb([this](std::shared_ptr<egl::context> ctx) {
      painter_->set_wallpaper_context(std::move(ctx));
    });


    wallpaper_surface_->set_update_cb([this]() {
      update();
      return !surface_updated_[0] || alpha_changed(last_wallpaper_alpha_, app().alpha());
    });


    wallpaper_surface_->set_render_cb([this]() {
      last_wallpaper_alpha_ = app().alpha();
      painter_.value().render_wallpaper(last_layout_, last_wallpaper_alpha_,
          last_layout_id_);
      surface_updated_[0] = true;
    });
  }



  if (clipping_surface_) {
    clipping_surface_->set_context_cb([this](std::shared_ptr<egl::context> ctx) {
      painter_->set_clipping_context(std::move(ctx));
    });


    clipping_surface_->set_update_cb([this]() {
      update();
      return !surface_updated_[1] || alpha_changed(last_clipping_alpha_, app().alpha());
    });


    clipping_surface_->set_render_cb([this]() {
      surface_updated_[1] = true;

      if (!clipping_surface_->visible()) {
        return;
      }

      last_clipping_alpha_ = app().alpha();
      painter_.value().render_clipping(last_layout_, last_clipping_alpha_,
          last_layout_id_);
    });
  }
}
