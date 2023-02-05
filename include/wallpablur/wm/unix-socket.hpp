#ifndef WALLPABLUR_WM_UNIX_SOCKET_HPP_INCLUDED
#define WALLPABLUR_WM_UNIX_SOCKET_HPP_INCLUDED

#include <filesystem>
#include <optional>
#include <span>



namespace wm {

class unix_socket {
  public:
    unix_socket(const unix_socket&) = delete;
    unix_socket(unix_socket&&) noexcept;

    unix_socket& operator=(const unix_socket&) = delete;
    unix_socket& operator=(unix_socket&&) noexcept;

    ~unix_socket();

    explicit unix_socket(const std::filesystem::path&);



    void unblock_recv() const;
    [[nodiscard]] std::optional<size_t> recv(std::span<std::byte>) const;

    void send(std::span<const std::byte>) const;



  private:
    struct pipe_fd {
      int read {-1};
      int write{-1};



      pipe_fd();

      pipe_fd(const pipe_fd&) = delete;
      pipe_fd(pipe_fd&&) noexcept;

      pipe_fd& operator=(const pipe_fd&) = delete;
      pipe_fd& operator=(pipe_fd&&) noexcept;

      ~pipe_fd();
    };

    int     fd_        {-1};
    pipe_fd watch_pipe_;
};

}

#endif // WALLPABLUR_WM_UNIX_SOCKET_HPP_INCLUDED
