#include "wallpablur/wm/unix-socket.hpp"

#include <array>
#include <stdexcept>
#include <utility>

#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>



wm::unix_socket::pipe_fd::pipe_fd() {
  std::array<int, 2> rw{};
  if (pipe(rw.data()) == 0) {
    read  = rw[0];
    write = rw[1];
  }
}



wm::unix_socket::pipe_fd::~pipe_fd() {
  if (read  >= 0) { close(read);  }
  if (write >= 0) { close(write); }
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
    close(fd_);
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



wm::unix_socket::unix_socket(const std::filesystem::path& path) :
  fd_{socket(AF_UNIX, SOCK_STREAM, 0)}
{
  if (fd_ < 0) {
    throw std::runtime_error{"unable to open unix socket stream"};
  }

  fcntl(fd_, F_SETFD, FD_CLOEXEC); // NOLINT(*vararg)

  sockaddr_un addr {
    .sun_family = AF_UNIX,
    .sun_path   = {}
  };

  auto str = path.string();
  if (str.size() >= sizeof(addr.sun_path)) {
    throw std::runtime_error{"unix_socket{path}: path is too long"};
  }

  std::strncpy(static_cast<char*>(addr.sun_path), str.c_str(), sizeof(addr.sun_path)-1);

  // NOLINTNEXTLINE(*reinterpret-cast)
  if (connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    throw std::runtime_error{"unable to connect to socket \"" + path.string() + "\""};
  }
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
    throw std::runtime_error{"trying to read on deleted socket"};
  }

  size_t count{0};

  auto events = create_poll_pair(fd_, watch_pipe_.read);

  while (!buffer.empty()) {
    if (poll(events.data(), events.size(), -1) < 0) {
      throw std::runtime_error{"unable to poll on fd"};
    }

    if ((events[1].revents & POLLIN) != 0) {
      std::array<std::byte, 64> discard{};
      read(events[1].fd, discard.data(), discard.size());
      return {};
    }

    if ((events[0].revents & POLLIN) != 0) {
      if (ssize_t res = read(events[0].fd, buffer.data(), buffer.size()); res > 0) {
        buffer  =  buffer.subspan(res);
        count  +=  res;
      } else if (errno != EINTR && errno != EAGAIN) {
        throw std::runtime_error{"unable to read from socket"};
      }
    }
  }

  return count;
}



void wm::unix_socket::unblock_recv() const {
  if (watch_pipe_.write >= 0) {
    write(watch_pipe_.write, "done", 4);
  }
}



void wm::unix_socket::send(std::span<const std::byte> data) const {
  if (fd_ < 0) {
    throw std::runtime_error{"trying to write to deleted socket"};
  }

  if (::send(fd_, data.data(), data.size(), 0) < 0) {
    throw std::runtime_error{"unable to send data to socket"};
  }
}
