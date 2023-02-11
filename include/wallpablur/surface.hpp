#ifndef WALLPABLUR_SURFACE_HPP_INCLUDED
#define WALLPABLUR_SURFACE_HPP_INCLUDED


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
    surface(rectangle rect, surface_type type) :
      rect_{rect},
      type_{type}
    {}



    bool operator==(const surface&) const = default;

    [[nodiscard]] rectangle&       rect()       { return rect_; }
    [[nodiscard]] const rectangle& rect() const { return rect_; }

    [[nodiscard]] surface_type     type() const { return type_; }



  private:
    rectangle    rect_;
    surface_type type_;
};

#endif // WALLPABLUR_SURFACE_HPP_INCLUDED
