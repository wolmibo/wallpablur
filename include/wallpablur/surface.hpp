#ifndef WALLPABLUR_SURFACE_HPP_INCLUDED
#define WALLPABLUR_SURFACE_HPP_INCLUDED

#include <string>
#include <string_view>



struct rectangle {
  int          x     {0};
  int          y     {0};
  unsigned int width {0};
  unsigned int height{0};



  bool operator==(const rectangle&) const = default;
};





enum class surface_type {
  panel,
  floating,
  tiled
};



class surface {
  public:
    surface(
        rectangle    rect,
        surface_type type,
        std::string  app_id,
        bool         focused = false,
        bool         urgent  = false
    ) :
      rect_{rect},
      type_{type},

      app_id_ {std::move(app_id)},
      focused_{focused},
      urgent_ {urgent}
    {}



    bool operator==(const surface&) const = default;

    [[nodiscard]] rectangle&       rect()          { return rect_; }
    [[nodiscard]] const rectangle& rect()    const { return rect_; }

    [[nodiscard]] surface_type     type()    const { return type_; }

    [[nodiscard]] std::string_view app_id()  const { return app_id_;  }

    [[nodiscard]] bool             focused() const { return focused_; }
    [[nodiscard]] bool             urgent()  const { return urgent_;  }



  private:
    rectangle    rect_;
    surface_type type_;

    std::string  app_id_;

    bool         focused_;
    bool         urgent_;
};

#endif // WALLPABLUR_SURFACE_HPP_INCLUDED
