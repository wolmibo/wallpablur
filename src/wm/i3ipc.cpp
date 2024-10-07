#include "wallpablur/wm/i3ipc.hpp"

#include "wallpablur/config/config.hpp"
#include "wallpablur/exception.hpp"
#include "wallpablur/json/utils.hpp"
#include "wallpablur/surface.hpp"
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
    print_exception(ex);
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
    print_exception(ex);
  } catch (...) {
    logcerr::error("unhandled exception");
  }
}





namespace {
  void translate_surfaces(std::span<surface> surf, vec2<float> delta) {
    for (auto& r: surf) {
      r.rect().pos() += delta;
    }
  }



  [[nodiscard]] rectangle rectangle_from_json(const rapidjson::Value& value) {
    return {
      {static_cast<float>(json::member_to_int (value, "x"     ).value_or(0)),
       static_cast<float>(json::member_to_int (value, "y"     ).value_or(0))},
      {static_cast<float>(json::member_to_uint(value, "width" ).value_or(0)),
       static_cast<float>(json::member_to_uint(value, "height").value_or(0))}
    };
  }



  [[nodiscard]] flag_mask<surface_flag> init_surface_flag_mask(
      const rapidjson::Value& parent,
      flag_mask<surface_flag> parent_flags
  ) {
    static constexpr auto conttype_mask =
      make_mask<surface_flag>(surface_flag::tiled, surface_flag::floating);

    auto flags = parent_flags & conttype_mask;

    if (auto value = json::member_to_str(parent, "layout")) {
      if (*value == "splitv") {
        set_flag(flags, surface_flag::splitv);
      }
      if (*value == "splith") {
        set_flag(flags, surface_flag::splith);
      }
      if (*value == "stacked") {
        set_flag(flags, surface_flag::stacked);
      }
      if (*value == "tabbed") {
        set_flag(flags, surface_flag::tabbed);
      }
    }

    return flags;
  }



  void load_surface_from_json(
      workspace&              ws,
      const rapidjson::Value& value,
      flag_mask<surface_flag> flags,
      rectangle               parent_rect,
      bool                    floating
  ) {
    auto json_rect = json::find_member(value, "rect");
    if (!json_rect) {
      return;
    }

    std::string app_id{json::member_to_str(value, "app_id").value_or("")};

    if (json::member_to_bool(value, "focused").value_or(false)) {
      set_flag(flags, surface_flag::focused);
    }

    if (json::member_to_bool(value, "urgent").value_or(false)) {
      set_flag(flags, surface_flag::urgent);
    }

    if (json::member_to_int(value, "fullscreen_mode").value_or(0) > 0) {
      set_flag(flags, surface_flag::fullscreen);
    }

    auto base_rect{rectangle_from_json(*json_rect)};

    if (json::member_to_bool(value, "visible").value_or(false)) {
      ws.emplace_surface(base_rect, app_id, flags, 0.f);
    } else {
      return;
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
      deco_rect.pos() += parent_rect.pos();
    } else {
      deco_rect.pos() += base_rect.pos() + vec2{0.f, - deco_rect.size().y()};
    }

    set_flag(flags, surface_flag::decoration);

    ws.emplace_surface(deco_rect, app_id, flags, 0.f);
  }



  bool parse_node_children(
      workspace&              ws,
      const rapidjson::Value& value,
      flag_mask<surface_flag> parent_flags
  ) {
    auto con_flags = init_surface_flag_mask(value, parent_flags);

    rectangle parent_rect{};
    if (auto rect = json::find_member(value, "rect")) {
      parent_rect = rectangle_from_json(*rect);
    }

    auto nodes    = json::member_to_array(value, "nodes");
    auto floating = json::member_to_array(value, "floating_nodes");

    if ((!nodes || nodes->Empty()) && (!floating || floating->Empty())) {
      return false;
    }

    auto handle_children = [&](const auto& nodes, surface_flag type) {
      auto flags = con_flags | make_mask<surface_flag>(type);
      bool floating = (type == surface_flag::floating);

      if (nodes) {
        for (const auto& node: *nodes) {
          if (!parse_node_children(ws, node, flags)) {
            load_surface_from_json(ws, node, flags, parent_rect, floating);
          }
        }
      }
    };

    handle_children(nodes,    surface_flag::tiled);
    handle_children(floating, surface_flag::floating);

    return true;
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

      rectangle output_rect{};
      if (auto json = json::find_member(value, "rect")) {
        output_rect = rectangle_from_json(*json);
      }

      workspace ws{
        std::string{name.value_or("")},
        std::string{json::member_to_str(node, "output").value_or("")},
        output_rect.size(),
        {}
      };

      parse_node_children(ws, node, make_mask<surface_flag>());

      if (auto offset = json::find_member(value, "rect")) {
        translate_surfaces(ws.surfaces(), -output_rect.pos());
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
    print_exception(ex);
  }
}
