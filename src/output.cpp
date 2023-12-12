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

    config_.emplace(config::global_config().output_config_for(wl_output_->name()));
    painter_.emplace(*config_);

    update(false);

    create_surfaces(config_->clipping);
    setup_surfaces();
  });

  wl_output_->set_size_cb([this](vec2<uint32_t> size) {
    if (wallpaper_surface_) {
      wallpaper_surface_->update_screen_size(size);
    }

    if (clipping_surface_) {
      clipping_surface_->update_screen_size(size);
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





void output::update(bool force) {
  if (layout_token_.changed() || force) {
    {
      auto ref = layout_token_.get();
      last_layout_ = *ref;

      for (const auto& [surface, expression]: fixed_panels_) {
        if (expression.evaluate(*ref)) {
          last_layout_.emplace_surface(surface);
        }
      }
    }

    last_layout_id_++;
    surface_updated_.reset();

    auto round_corners = painter_->update_rounded_corners(last_layout_, last_layout_id_);

    if (clipping_surface_) {
      if (round_corners) {
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
    wallpaper_surface_->set_geometry_cb([this](const wayland::geometry& geo) {
        update_geometry(geo);
    });


    wallpaper_surface_->set_context_cb([this](std::shared_ptr<egl::context> ctx) {
      painter_->set_wallpaper_context(std::move(ctx));
    });


    wallpaper_surface_->set_update_cb([this]() {
      update(false);
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
    clipping_surface_->set_geometry_cb([this](const wayland::geometry& geo) {
        if (clipping_surface_->visible()) {
          update_geometry(geo);
        }
    });


    clipping_surface_->set_context_cb([this](std::shared_ptr<egl::context> ctx) {
      painter_->set_clipping_context(std::move(ctx));
    });


    clipping_surface_->set_update_cb([this]() {
      update(false);
      return !surface_updated_[1] || alpha_changed(last_clipping_alpha_, app().alpha());
    });


    clipping_surface_->set_render_cb([this]() {
      surface_updated_[1] = true;
      last_clipping_alpha_ = app().alpha();

      if (!clipping_surface_->visible()) {
        return;
      }

      painter_.value().render_clipping(last_layout_, last_clipping_alpha_,
          last_layout_id_);
    });
  }
}



void output::update_geometry(const wayland::geometry& geometry) {
  if (!config_ || !painter_) {
    return;
  }

  painter_->update_geometry(geometry);

  if (geometry == geometry_) {
    return;
  }

  geometry_ = geometry;

  fixed_panels_.clear();

  for (const auto& panel: config_->fixed_panels) {
    fixed_panels_.emplace_back(surface {
        panel.to_rect(geometry_.logical_size()),
        panel.app_id,
        panel.mask,
        panel.radius
      },
      panel.condition
    );
  }

  update(true);
}
