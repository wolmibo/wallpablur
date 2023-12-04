#include "wallpablur/config/config.hpp"
#include "wallpablur/json/utils.hpp"
#include "wallpablur/surface.hpp"
#include "wallpablur/wm/i3ipc.hpp"
#include "wallpablur/wm/layout-manager.hpp"

#include <logcerr/log.hpp>



wm::i3ipc::i3ipc(
  const std::filesystem::path& path
) :
  poll_socket_ {path},
  event_socket_{path},

  poll_rate_   {config::global_config().poll_rate()},

  event_loop_thread_{[this] (const std::stop_token& stoken){
    logcerr::thread_name("event");
    logcerr::debug("entering event loop");
    event_loop(stoken);
    logcerr::debug("exiting event loop");
  }},

  timer_loop_thread_{[this] (const std::stop_token& stoken) {
    logcerr::thread_name("timer");
    logcerr::debug("entering timer loop");
    timer_loop(stoken);
    logcerr::debug("exiting timer loop");
  }}
{}



wm::i3ipc::~i3ipc() {
  event_loop_thread_.request_stop();
  timer_loop_thread_.request_stop();

  event_socket_.unblock();

  exit_cv_.notify_all();
}



void wm::i3ipc::timer_loop(const std::stop_token& stoken) {
  try {
    while (!stoken.stop_requested()) {
      update_layouts();

      {
        std::unique_lock<std::mutex> lock(exit_requested_mutex_);
        exit_cv_.wait_for(lock, poll_rate_);
      }
    }
  } catch (std::exception& ex) {
    logcerr::error(ex.what());
  } catch (...) {
    logcerr::error("unhandled exception");
  }
}



void wm::i3ipc::event_loop(const std::stop_token& stoken) {
  try {
    event_socket_.subscribe(i3ipc_socket::event::workspace);
    event_socket_.subscribe(i3ipc_socket::event::window);

    while (!stoken.stop_requested()) {
      if (event_socket_.next_message()) {
        update_layouts();
      }
    }
  } catch (std::exception& ex) {
    logcerr::error(ex.what());
  } catch (...) {
    logcerr::error("unhandled exception");
  }
}





namespace {
  void translate_surfaces(std::span<surface> surf, int dx, int dy) {
    for (auto& r: surf) {
      r.rect().translate(dx, dy);
    }
  }



  [[nodiscard]] rectangle rectangle_from_json(const rapidjson::Value& value) {
    return {
      static_cast<float>(json::member_to_int (value, "x"     ).value_or(0)),
      static_cast<float>(json::member_to_int (value, "y"     ).value_or(0)),
      static_cast<float>(json::member_to_uint(value, "width" ).value_or(0)),
      static_cast<float>(json::member_to_uint(value, "height").value_or(0))
    };
  }



  [[nodiscard]] layout_orientation orientation_from_json(const rapidjson::Value& parent) {
    if (auto value = json::member_to_str(parent, "layout")) {
      if (*value == "splitv") { return layout_orientation::vertical;   }
      if (*value == "splith") { return layout_orientation::horizontal; }
    }

    return layout_orientation::none;
  }



  void load_surface_from_json(
      workspace&              ws,
      const rapidjson::Value& value,
      bool                    floating,
      layout_orientation      orientation,
      rectangle               parent_rect
  ) {
    auto json_rect = json::find_member(value, "rect");
    if (!json_rect) {
      return;
    }

    std::string app_id {json::member_to_str (value, "app_id" ).value_or("")};

    surface_flag_mask mask;

    if (json::member_to_bool(value, "focused").value_or(false)) {
      set_surface_flag(mask, surface_flag::focused);
    }

    if (json::member_to_bool(value, "urgent").value_or(false)) {
      set_surface_flag(mask, surface_flag::urgent);
    }

    if (json::member_to_int(value, "fullscreen_mode").value_or(0) > 0) {
      set_surface_flag(mask, surface_flag::fullscreen);
    }

    if (floating) {
      set_surface_flag(mask, surface_flag::floating);
    } else {
      set_surface_flag(mask, surface_flag::tiled);
    }

    auto base_rect{rectangle_from_json(*json_rect)};

    if (json::member_to_bool(value, "visible").value_or(false)) {
      ws.emplace_surface(base_rect, app_id, mask, 0.f, orientation);
    }


    json_rect = json::find_member(value, "deco_rect");
    if (!json_rect) {
      return;
    }

    auto deco_rect{rectangle_from_json(*json_rect)};

    if (deco_rect.empty()) {
      return;
    }

    if (floating) {
      deco_rect.translate(parent_rect.x(), parent_rect.y());
    } else {
      deco_rect.translate(base_rect.x(), base_rect.y() - deco_rect.height());
    }

    set_surface_flag(mask, surface_flag::decoration);

    ws.emplace_surface(deco_rect, app_id, mask, 0.f, orientation);
  }



  void load_node_leaves(
      workspace&                          ws,
      const rapidjson::Value::ConstArray& list,
      bool                                floating,
      layout_orientation                  parent_orientation,
      rectangle                           parent_rect
  ) {

    for (const auto& container: list) {
      auto nodes  = json::member_to_array(container, "nodes");
      auto floats = json::member_to_array(container, "floating_nodes");

      if ((!nodes || nodes->Empty()) && (!floats || floats->Empty())) {
        load_surface_from_json(ws, container, floating, parent_orientation, parent_rect);
      } else {
        auto orientation = orientation_from_json(container);

        rectangle parent_rect{};
        if (auto rect = json::find_member(container, "rect")) {
          parent_rect = rectangle_from_json(*rect);
        }

        if (nodes) {
          load_node_leaves(ws, *nodes, false, orientation, parent_rect);
        }

        if (floats) {
          load_node_leaves(ws, *floats, true, orientation, parent_rect);
        }
      }
    }
  }



  void parse_workspace_layout(workspace& ws, const rapidjson::Value& value) {
    auto orientation = orientation_from_json(value);

    rectangle parent_rect{};
    if (auto rect = json::find_member(value, "rect")) {
      parent_rect = rectangle_from_json(*rect);
    }

    if (auto nodes = json::member_to_array(value, "nodes")) {
      load_node_leaves(ws, *nodes, false, orientation, parent_rect);
    }

    if (auto floating = json::member_to_array(value, "floating_nodes")) {
      load_node_leaves(ws, *floating, true, orientation, parent_rect);
    }
  }



  [[nodiscard]] workspace parse_output_layout(const rapidjson::Value& value) {
    auto current = json::member_to_str(value, "current_workspace");
    if (!current) {
      logcerr::warn("no active workspace on output");
      return {};
    }

    auto nodes = json::member_to_array(value, "nodes");
    if (!nodes) {
      return {};
    }


    for (const auto& node: *nodes) {
      if (json::member_to_str(node, "type") != "workspace") {
        continue;
      }

      auto name = json::member_to_str(node, "name");

      if (name != *current) {
        continue;
      }

      workspace ws{
        std::string{name.value_or("")},
        std::string{json::member_to_str(node, "output").value_or("")},
        {}
      };

      parse_workspace_layout(ws, node);

      if (auto offset = json::find_member(value, "rect")) {
        translate_surfaces(ws.surfaces(),
          -json::member_to_int(*offset, "x").value_or(0),
          -json::member_to_int(*offset, "y").value_or(0)
        );
      }

      return ws;
    }

    logcerr::warn("active workspace not found");
    return {};
  }



  void parse_layout(wm::layout_manager& manager, std::string_view json) {
    rapidjson::Document document;
    json::assert_parse_success(document.Parse(json.data(), json.size()));

    auto nodes = json::member_to_array(document, "nodes");
    if (!nodes) {
      return;
    }

    for (const auto& output: *nodes) {
      if (json::member_to_str(output, "type") != "output") {
        continue;
      }
      if (json::member_to_bool(output, "dpms") != true) {
        continue;
      }

      if (auto name = json::member_to_str(output, "name")) {
        manager.update_layout(*name, parse_output_layout(output));
      } else {
        logcerr::warn("found active output without name");
      }
    }
  }
}



void wm::i3ipc::update_layouts() {
  auto result = poll_socket_.request(i3ipc_socket::action::get_tree);

  std::lock_guard<std::mutex> lock{layouts_mutex_};

  if (!result || *result == layouts_json_) {
    return;
  }

  layouts_json_ = std::move(*result);

  try {
    parse_layout(manager_, layouts_json_);
  } catch (std::exception& ex) {
    logcerr::error("unable to parse json: {}", ex.what());
  }
}
