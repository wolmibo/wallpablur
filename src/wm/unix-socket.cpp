#include "wallpablur/wm/unix-socket.hpp"
#include "wallpablur/exception.hpp"

#include <array>
#include <utility>

#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>



wm::unix_socket::pipe_fd::pipe_fd() {
  std::array<int, 2> rw{};

  check_errno("unable to initialize pipe", [&]{
    return pipe(rw.data()) == 0;
  });

  read  = rw[0];
  write = rw[1];
}



wm::unix_socket::pipe_fd::~pipe_fd() {
  if (read >= 0) {
    check_errno_nothrow("unable to close read pipe", [&] {
      return close(read) == 0;
    });
  }

  if (write >= 0) {
    check_errno_nothrow("unable to close write pipe", [&] {
      return close(write) == 0;
    });
  }
}



wm::unix_socket::pipe_fd::pipe_fd(pipe_fd&& rhs) noexcept :
  read {std::exchange(rhs.read,  -1)},
  write{std::exchange(rhs.write, -1)}
{}



wm::unix_socket::pipe_fd& wm::unix_socket::pipe_fd::operator=(pipe_fd&& rhs) noexcept {
  std::swap(read,   rhs.read);
  std::swap(write, rhs.write);

  return *this;
}





wm::unix_socket::~unix_socket() {
  if (fd_ >= 0) {
    check_errno_nothrow("unable to close socket", [&] {
      return close(fd_) == 0;
    });
  }
}



wm::unix_socket::unix_socket(unix_socket&& rhs) noexcept :
  fd_{std::exchange(rhs.fd_, -1)},
  watch_pipe_{std::move(rhs.watch_pipe_)}
{}



wm::unix_socket& wm::unix_socket::operator=(unix_socket&& rhs) noexcept {
  std::swap(fd_, rhs.fd_);
  watch_pipe_ = std::move(rhs.watch_pipe_);

  return *this;
}




namespace {
  [[nodiscard]] int init_socket() {
    int soc{};
    check_errno("unable to create socket stream", [&] {
      soc = socket(AF_UNIX, SOCK_STREAM, 0);
      return soc != -1;
    });
    return soc;
  }
}



wm::unix_socket::unix_socket(const std::filesystem::path& path) :
  fd_{init_socket()}
{
  check_errno("unable to set socket attributes", [&] {
    return fcntl(fd_, F_SETFD, FD_CLOEXEC) == 0; // NOLINT(*vararg)
  });

  sockaddr_un addr {
    .sun_family = AF_UNIX,
    .sun_path   = {}
  };

  auto str = path.string();
  if (str.size() >= sizeof(addr.sun_path)) {
    throw exception{"unix_socket: path is too long"};
  }

  std::strncpy(static_cast<char*>(addr.sun_path), str.c_str(), sizeof(addr.sun_path)-1);

  check_errno("unable to connect to socket", [&] {
    // NOLINTNEXTLINE(*reinterpret-cast)
    return connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
  });
}





namespace {
  [[nodiscard]] std::array<pollfd, 2> create_poll_pair(int fd1, int fd2) {
    return {
      pollfd {
        .fd      = fd1,
        .events  = POLLIN,
        .revents = 0,
      },
      pollfd {
        .fd      = fd2,
        .events  = POLLIN,
        .revents = 0,
      }
    };
  }
}



std::optional<size_t> wm::unix_socket::recv(std::span<std::byte> buffer) const {
  if (fd_ < 0) {
    throw exception{"trying to read on deleted socket"};
  }

  size_t count{0};

  auto events = create_poll_pair(fd_, watch_pipe_.read);

  while (!buffer.empty()) {
    check_errno("unable to poll on fd", [&] {
      return poll(events.data(), events.size(), -1) >= 0;
    });

    if ((events[1].revents & POLLIN) != 0) {
      std::array<std::byte, 64> discard{};
      check_errno("unable to read from socket", [&] {
        return read(events[1].fd, discard.data(), discard.size()) >= 0;
      });
      return {};
    }

    if ((events[0].revents & POLLIN) != 0) {
      check_errno("unable to read from socket", [&] {
        if (ssize_t res = read(events[0].fd, buffer.data(), buffer.size()); res > 0) {
          buffer  =  buffer.subspan(res);
          count  +=  res;
        } else if (errno != EINTR && errno != EAGAIN) {
          return false;
        }
        return true;
      });
    }
  }

  return count;
}



void wm::unix_socket::unblock_recv() const {
  if (watch_pipe_.write >= 0) {
    check_errno("unable to write to pipe", [&] {
      return write(watch_pipe_.write, "done", 4) == 4;
    });
  }
}



void wm::unix_socket::send(std::span<const std::byte> data) const {
  if (fd_ < 0) {
    throw exception{"trying to write to deleted socket"};
  }

  check_errno("unable to send data to socket", [&] {
      return ::send(fd_, data.data(), data.size(), 0) == std::ssize(data);
  });
}
