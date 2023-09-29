#ifndef WALLPABLUR_WM_LAYOUT_MANAGER_HPP_INCLUDED
#define WALLPABLUR_WM_LAYOUT_MANAGER_HPP_INCLUDED

#include "wallpablur/flat-map.hpp"
#include "wallpablur/workspace.hpp"
#include "wallpablur/wm/change-token.hpp"

#include <mutex>



namespace wm {

class layout_manager {
  public:
    [[nodiscard]] change_token<workspace> subscribe(std::string_view key) {
      std::lock_guard lock{layouts_mutex_};
      return layouts_.find_or_create(key).create_token();
    }

    void update_layout(std::string_view key, workspace&& lay) {
      std::lock_guard lock{layouts_mutex_};
      layouts_.find_or_create(key).set(std::move(lay));
    }



  private:
    flat_map<std::string, change_source<workspace>> layouts_;
    std::mutex                                      layouts_mutex_;
};

}

#endif // WALLPABLUR_WM_LAYOUT_MANAGER_HPP_INCLUDED
