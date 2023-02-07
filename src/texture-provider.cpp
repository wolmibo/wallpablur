#include "wallpablur/config/filter.hpp"
#include "wallpablur/texture-provider.hpp"
#include "wallpablur/wayland/geometry.hpp"

#include <algorithm>
#include <tuple>

#include <logging/log.hpp>



void texture_provider::cleanup() {
  for (size_t i = 0; i < cache_.size();) {
    if (cache_.value(i).expired()) {
      cache_.erase(i);
    } else {
      ++i;
    }
  }
}



std::shared_ptr<gl::texture> texture_provider::get(
  const wayland::geometry& geometry,
  const config::brush&     brush
) {
  if (!brush.fgraph) {
    logging::warn("trying to retrieve texture from brush without filter_graph");
    return {};
  }

  cleanup();

  size_t ix = std::ranges::find_if(cache_.keys(), [&brush, &geometry](const auto& tp){
                  return tp.first.same_pixel_size(geometry) && tp.second == brush; })
                - cache_.keys().begin();

  if (ix < cache_.size()) {
    return cache_.value(ix).lock();
  }

  auto texture = create_texture(geometry, brush);
  if (texture) {
    cache_.emplace({geometry, brush}, texture);
  }

  return texture;
}





namespace {
  [[nodiscard]] bool starts_with(
      const config::filter_graph& fg,
      const config::filter_graph& start
  ) {
    if (fg.path != start.path ||
        fg.distribution != start.distribution ||
        fg.filters.size() < start.filters.size()) {
      return false;
    }

    return std::equal(start.filters.begin(), start.filters.end(), fg.filters.begin());
  }



  [[nodiscard]] size_t find_best_fit(
      std::span<texture_provider::key> list,
      const wayland::geometry&         geometry,
      const config::brush&             brush
  ) {
    size_t best{list.size()};
    size_t match_length{0};

    for (size_t i = 0; i < list.size(); ++i) {
      if (list[i].first != geometry || list[i].second.solid != brush.solid) {
        continue;
      }

      if (starts_with(*brush.fgraph, *list[i].second.fgraph) &&
          list[i].second.fgraph->filters.size() + 1 > match_length) {
        match_length = list[i].second.fgraph->filters.size() + 1;
        best = i;
      }
    }

    return best;
  }
}





std::shared_ptr<gl::texture> texture_provider::create_texture(
  const wayland::geometry& geometry,
  const config::brush&     brush
) {
  try {
    auto index = find_best_fit(cache_.keys(), geometry, brush);

    if (index == cache_.size()) {
      return std::make_shared<gl::texture>(texture_generator_.generate(geometry, brush));
    }

    return std::make_shared<gl::texture>(
        texture_generator_.generate_from_existing(
          *cache_.value(index).lock(),
          geometry,
          std::span{brush.fgraph->filters}
            .subspan(cache_.key(index).second.fgraph->filters.size())
        )
      );

  } catch (std::exception& ex) {
    logging::error("unable to create texture:\n{}", ex.what());
    return {};
  }
}
