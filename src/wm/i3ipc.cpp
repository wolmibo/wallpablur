#include "wallpablur/json/utils.hpp"
#include "wallpablur/surface.hpp"
#include "wallpablur/wm/change-token.hpp"
#include "wallpablur/wm/i3ipc.hpp"
#include "wallpablur/wm/layout-manager.hpp"

#include <logging/log.hpp>



wm::i3ipc::i3ipc(
  const std::filesystem::path& path,
  std::chrono::milliseconds    poll_rate
) :
  poll_socket_ {path},
  event_socket_{path},

  poll_rate_   {poll_rate},

  event_loop_thread_{[this] (const std::stop_token& stoken){
    logging::thread_name("event");
    logging::debug("entering event loop");
    event_loop(stoken);
    logging::debug("exiting event loop");
  }},

  timer_loop_thread_{[this] (const std::stop_token& stoken) {
    logging::thread_name("timer");
    logging::debug("entering timer loop");
    timer_loop(stoken);
    logging::debug("exiting timer loop");
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
    logging::error(ex.what());
  } catch (...) {
    logging::error("unhandled exception");
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
    logging::error(ex.what());
  } catch (...) {
    logging::error("unhandled exception");
  }
}





namespace {
  void translate_surfaces(std::span<surface> surf, int dx, int dy) {
    for (auto& r: surf) {
      r.rect().x += dx;
      r.rect().y += dy;
    }
  }



  [[nodiscard]] rectangle rectangle_from_json(const rapidjson::Value& value) {
    return rectangle {
      .x      = json::member_to_int (value, "x"     ).value_or(0),
      .y      = json::member_to_int (value, "y"     ).value_or(0),
      .width  = json::member_to_uint(value, "width" ).value_or(0),
      .height = json::member_to_uint(value, "height").value_or(0)
    };
  }



  [[nodiscard]] std::optional<surface> surface_from_json(
      const rapidjson::Value& value,
      surface_type            type
  ) {
    if (!json::member_to_bool(value, "visible").value_or(false)) {
      return {};
    }

    auto json_rect = json::find_member(value, "rect");
    if (!json_rect) {
      return {};
    }

    surface surf{rectangle_from_json(*json_rect), type};

    return surf;
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
        if (auto surf = surface_from_json(container, type)) {
          surfaces.emplace_back(*surf);
        }
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



  [[nodiscard]] wm::layout parse_workspace_layout(const rapidjson::Value& value) {
    wm::layout surfaces;

    if (auto nodes = json::member_to_array(value, "nodes")) {
      load_node_leaves(surfaces, *nodes, surface_type::tiled);
    }

    if (auto floating = json::member_to_array(value, "floating_nodes")) {
      load_node_leaves(surfaces, *floating, surface_type::floating);
    }

    return surfaces;
  }



  [[nodiscard]] wm::layout parse_output_layout(const rapidjson::Value& value) {
    auto current = json::member_to_str(value, "current_workspace");
    if (!current) {
      logging::warn("no active workspace on output");
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
      if (json::member_to_str(node, "name") != *current) {
        continue;
      }

      auto layout = parse_workspace_layout(node);

      if (auto offset = json::find_member(value, "rect")) {
        translate_surfaces(layout,
          -json::member_to_int(*offset, "x").value_or(0),
          -json::member_to_int(*offset, "y").value_or(0)
        );
      }

      return layout;
    }

    logging::warn("active workspace not found");
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
        logging::warn("found active output without name");
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
    logging::error("unable to parse json: {}", ex.what());
  }
}
