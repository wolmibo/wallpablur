#include "wallpablur/wm/i3ipc-socket.hpp"

#include <algorithm>
#include <array>
#include <expected>
#include <filesystem>
#include <limits>
#include <span>
#include <stdexcept>

#include <memory.h>
#include <unistd.h>
#include <sys/socket.h>

#include <logcerr/log.hpp>

#include <rapidjson/document.h>



namespace {
  constexpr std::string_view header_signature {"i3-ipc"};

  struct header {
    uint32_t size{0};
    uint32_t type{0};



    header() = default;
    header(uint32_t s, uint32_t t) : size{s}, type{t} {}

    [[nodiscard]] auto serialize() const {
      return std::bit_cast<std::array<std::byte, sizeof(header)>>(*this);
    }

    [[nodiscard]] std::span<std::byte> as_writable_bytes() {
      return std::as_writable_bytes(std::span{this, 1});
    }
  };

  static_assert(sizeof(header) == 8);



  [[nodiscard]] std::span<const std::byte> to_bytes(std::string_view content) {
    return std::as_bytes(std::span{content.data(), content.size()});
  }





  void send_i3ipc(const wm::unix_socket& sock, uint32_t type, std::string_view request) {
    assert(request.size() < std::numeric_limits<uint32_t>::max());

    sock.send(to_bytes(header_signature));
    sock.send(header{static_cast<uint32_t>(request.size()), type}.serialize());
    sock.send(to_bytes(request));
  }





  enum class receive_status {
    canceled,
    error
  };



  [[nodiscard]] std::optional<receive_status> recv_maybe(
      const wm::unix_socket& socket,
      std::span<std::byte>   buffer
  ) {
    auto res = socket.recv(buffer);

    if (!res)                 { return receive_status::canceled; }
    if (res != buffer.size()) { return receive_status::error;    }

    return {};
  }



  [[nodiscard]] std::expected<std::pair<uint32_t, std::string>, receive_status>
  recv_i3ipc(const wm::unix_socket& socket) {

    std::array<std::byte, header_signature.size()> buffer{};

    if (auto err = recv_maybe(socket, buffer)) {
      return std::unexpected(*err);
    }

    if (!std::ranges::equal(buffer, to_bytes(header_signature))) {
      throw std::runtime_error{"invalid i3ipc header"};
    }

    header head;
    if (auto err = recv_maybe(socket, head.as_writable_bytes())) {
      return std::unexpected(*err);
    }

    std::string str_buffer(head.size, ' ');
    if (auto err = recv_maybe(socket, std::as_writable_bytes(std::span{str_buffer}))) {
      return std::unexpected(*err);
    }

    return std::make_pair(head.type, str_buffer);
  }





  [[nodiscard]] std::string_view subscription_code(wm::i3ipc_socket::event ev) {
    using enum wm::i3ipc_socket::event;

    switch (ev) {
      case workspace: return R"(["workspace"])";
      case window:    return R"(["window"])";
    }

    throw std::runtime_error{"unknown i3ipc event: "
      + std::to_string(std::to_underlying(ev))};
  }



  [[nodiscard]] bool is_success(std::string_view json) {
    rapidjson::Document document;

    if (document.Parse(json.data(), json.size()).HasParseError()) {
      return false;
    }

    auto it = document.FindMember("success");
    if (it == document.MemberEnd()) {
      return false;
    }

    return it->value.IsBool() && it->value.GetBool();
  }
}



void wm::i3ipc_socket::subscribe(event ev) const {
  std::lock_guard lock{socket_mutex_};

  send_i3ipc(socket_, std::to_underlying(action::subscribe), subscription_code(ev));

  auto result = recv_i3ipc(socket_);

  if ((result && !is_success(result->second))
      || (!result && result.error() == receive_status::error)) {

    throw std::runtime_error{"unable to subscribe to i3ipc event "
      + std::string{subscription_code(ev)}};
  }
}





std::optional<std::string> wm::i3ipc_socket::request(action act) const {
  std::lock_guard lock{socket_mutex_};

  send_i3ipc(socket_, static_cast<uint32_t>(act), "");
  if (auto result = recv_i3ipc(socket_)) {
    return result->second;
  }

  return {};
}





std::optional<wm::i3ipc_socket::message> wm::i3ipc_socket::next_message() const {
  std::lock_guard lock{socket_mutex_};

  if (auto result = recv_i3ipc(socket_)) {
    const auto& [type, payload] = *result;

    switch (type) {
      case std::to_underlying(event::workspace):
      case std::to_underlying(event::window):
        return make_pair(static_cast<event>(type), std::move(payload));
    }

    throw std::runtime_error{"next message has unsupported event"};
  }

  return {};
}





namespace {
  std::optional<std::filesystem::path> guess_socket() {
    const char* env = getenv("XDG_RUNTIME_DIR");

    if (env == nullptr || *env == 0 || !std::filesystem::is_directory(env)) {
      return {};
    }

    std::optional<std::filesystem::path> candidate;

    for (const auto& path: std::filesystem::directory_iterator{env}) {
      auto fname = path.path().filename().string();
      if (fname.starts_with("sway-ipc.") && fname.ends_with(".sock")) {
        if (candidate) {
          logcerr::verbose("not guessing socket due to multiple candidates");
          return {};
        }

        candidate = path.path();
      }
    }

    return candidate;
  }
}



std::optional<std::filesystem::path> wm::find_i3_socket() {
  if (const char* env = getenv("SWAYSOCK"); env != nullptr && *env != 0) {
    logcerr::verbose("found socket SWAYSOCK=\"{}\"", env);
    return env;
  }

  if (const char* env = getenv("I3SOCK"); env != nullptr && *env != 0) {
    logcerr::verbose("found socket I3SOCK=\"{}\"", env);
    return env;
  }

  if (auto socket = guess_socket()) {
    logcerr::warn("guessed socket \"{}\";\n"
        "make sure SWAYSOCK or I3SOCK is set correctly or use --socket",
        socket->string());

    return *socket;
  }

  return {};
}
