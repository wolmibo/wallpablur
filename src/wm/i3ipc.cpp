#include "wallpablur/config/config.hpp"
#include "wallpablur/json/utils.hpp"
#include "wallpablur/surface.hpp"
#include "wallpablur/wm/change-token.hpp"
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



  void load_surface_from_json(
      wm::layout&             workspace,
      const rapidjson::Value& value,
      surface_type            type
  ) {
    auto json_rect = json::find_member(value, "rect");
    if (!json_rect) {
      return;
    }

    std::string app_id {json::member_to_str (value, "app_id" ).value_or("")};
    bool        focused{json::member_to_bool(value, "focused").value_or(false)};
    bool        urgent {json::member_to_bool(value, "urgent" ).value_or(false)};

    auto base_rect{rectangle_from_json(*json_rect)};

    if (json::member_to_bool(value, "visible").value_or(false)) {
      workspace.emplace_surface(base_rect, type, app_id, focused, urgent);
    }


    json_rect = json::find_member(value, "deco_rect");
    if (!json_rect) {
      return;
    }

    auto deco_rect{rectangle_from_json(*json_rect)};

    if (deco_rect.empty()) {
      return;
    }

    deco_rect.translate(base_rect.x(), base_rect.y() - deco_rect.height());

    workspace.emplace_surface(deco_rect, surface_type::decoration,
                                 app_id, focused, urgent);
  }



  void load_node_leaves(
      wm::layout&                         surfaces,
      const rapidjson::Value::ConstArray& list,
      surface_type                        type
  ) {

    for (const auto& container: list) {
      auto nodes    = json::member_to_array(container, "nodes");
      auto floating = json::member_to_array(container, "floating_nodes");

      if ((!nodes || nodes->Empty()) && (!floating || floating->Empty())) {
        load_surface_from_json(surfaces, container, type);
      } else {
        if (nodes) {
          load_node_leaves(surfaces, *nodes, surface_type::tiled);
        }

        if (floating) {
          load_node_leaves(surfaces, *floating, surface_type::floating);
        }
      }
    }
  }



  void parse_workspace_layout(wm::layout& workspace, const rapidjson::Value& value) {
    wm::layout surfaces{
      std::string{json::member_to_str(value, "name").value_or("")},
      std::string{json::member_to_str(value, "output").value_or("")},
      {}
    };

    if (auto nodes = json::member_to_array(value, "nodes")) {
      load_node_leaves(workspace, *nodes, surface_type::tiled);
    }

    if (auto floating = json::member_to_array(value, "floating_nodes")) {
      load_node_leaves(workspace, *floating, surface_type::floating);
    }
  }



  [[nodiscard]] wm::layout parse_output_layout(const rapidjson::Value& value) {
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

      wm::layout workspace{
        std::string{name.value_or("")},
        std::string{json::member_to_str(node, "output").value_or("")},
        {}
      };

      parse_workspace_layout(workspace, node);

      if (auto offset = json::find_member(value, "rect")) {
        translate_surfaces(workspace.surfaces(),
          -json::member_to_int(*offset, "x").value_or(0),
          -json::member_to_int(*offset, "y").value_or(0)
        );
      }

      return workspace;
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
