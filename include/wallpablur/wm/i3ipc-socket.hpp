#ifndef WALLPABLUR_WM_I3IPC_SOCKET_HPP_INCLUDED
#define WALLPABLUR_WM_I3IPC_SOCKET_HPP_INCLUDED

#include "wallpablur/wm/unix-socket.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace wm {

class i3ipc_socket {
  public:
    enum class event : uint32_t {
      workspace = (1u << 31) | 0u,
      window    = (1u << 31) | 3u,
    };

    enum class action : uint32_t {
      subscribe = 2,
      get_tree  = 4,
    };

    using message = std::pair<event, std::string>;



    explicit i3ipc_socket(const std::filesystem::path& path) : socket_{path} {}



    [[nodiscard]] std::optional<std::string> request(action) const;
    [[nodiscard]] std::optional<message>     next_message()  const;

    void subscribe(event) const;
    void unblock()        const { socket_.unblock_recv(); }



  private:
    mutable std::mutex socket_mutex_;
    unix_socket        socket_;
};



[[nodiscard]] std::optional<std::filesystem::path> find_i3_socket();

}

#endif // WALLPABLUR_WM_I3IPC_SOCKET_HPP_INCLUDED
