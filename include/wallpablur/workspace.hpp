#ifndef WALLPABLUR_WORKSPACE_HPP_INCLUDED
#define WALLPABLUR_WORKSPACE_HPP_INCLUDED

#include "wallpablur/surface.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>



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



    [[nodiscard]] std::span<surface>       surfaces()       { return surfaces_; }
    [[nodiscard]] std::span<const surface> surfaces() const { return surfaces_; }

    [[nodiscard]] std::string_view         name()     const { return name_;     }
    [[nodiscard]] std::string_view         output()   const { return output_;   }



    template<typename... Args>
    void emplace_surface(Args&&... args) {
      surfaces_.emplace_back(std::forward<Args>(args)...);
    }


  private:
    std::string name_;
    std::string output_;
    vec2<float> size_{0.f};

    std::vector<surface> surfaces_;
};

#endif // WALLPABLUR_WORKSPACE_HPP_INCLUDED
