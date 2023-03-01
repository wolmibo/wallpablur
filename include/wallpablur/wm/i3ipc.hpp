#ifndef WALLPABLUR_WM_I3IPC_HPP_INCLUDED
#define WALLPABLUR_WM_I3IPC_HPP_INCLUDED

#include "wallpablur/wm/change-token.hpp"
#include "wallpablur/wm/i3ipc-socket.hpp"
#include "wallpablur/wm/layout-manager.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace wm {

class i3ipc {
  public:
    i3ipc(const i3ipc&) = delete;
    i3ipc(i3ipc&&)      = delete;
    i3ipc& operator=(const i3ipc&) = delete;
    i3ipc& operator=(i3ipc&&)      = delete;

    ~i3ipc();

    i3ipc(const std::filesystem::path&);



    [[nodiscard]] change_token<wm::layout> layout_token(std::string_view name) {
      return manager_.subscribe(name);
    }



  private:
    layout_manager            manager_;

    i3ipc_socket              poll_socket_;
    i3ipc_socket              event_socket_;

    std::chrono::milliseconds poll_rate_;

    std::string               layouts_json_;
    std::mutex                layouts_mutex_;

    std::mutex                exit_requested_mutex_;
    std::condition_variable   exit_cv_;

    std::jthread              event_loop_thread_;
    std::jthread              timer_loop_thread_;



    void event_loop(const std::stop_token&);
    void timer_loop(const std::stop_token&);

    void update_layouts();
};

}

#endif // WALLPABLUR_WM_I3IPC_HPP_INCLUDED
