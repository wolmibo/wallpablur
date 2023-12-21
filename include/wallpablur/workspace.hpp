#ifndef WALLPABLUR_WORKSPACE_HPP_INCLUDED
#define WALLPABLUR_WORKSPACE_HPP_INCLUDED

#include "wallpablur/surface.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <flags.hpp>



enum class workspace_flag : size_t {
  covered,

  eoec_marker
};



class workspace {
  public:
    workspace() = default;

    workspace(
        std::string&&          name,
        std::string&&          output,
        vec2<float>            size,
        std::vector<surface>&& surfaces
    ) :
      name_    {std::move(name)},
      output_  {std::move(output)},
      size_    {size},
      surfaces_{std::move(surfaces)}
    {}



    bool operator==(const workspace&) const = default;



    [[nodiscard]] std::span<surface> surfaces() {
      invalidate();
      return surfaces_;
    }

    [[nodiscard]] std::span<const surface> surfaces() const { return surfaces_; }

    [[nodiscard]] std::string_view         name()     const { return name_;     }
    [[nodiscard]] std::string_view         output()   const { return output_;   }

    [[nodiscard]] bool test_flag(workspace_flag) const;


    template<typename... Args>
    void emplace_surface(Args&&... args) {
      invalidate();
      surfaces_.emplace_back(std::forward<Args>(args)...);
    }


  private:
    std::string name_;
    std::string output_;
    vec2<float> size_{0.f};

    std::vector<surface> surfaces_;

    mutable flag_mask<workspace_flag> flags_;
    mutable flag_mask<workspace_flag> flags_set_;

    void invalidate() { flags_set_ = {}; }
};

#endif // WALLPABLUR_WORKSPACE_HPP_INCLUDED
