#ifndef WALLPABLUR_RECT_HPP_INCLUDED
#define WALLPABLUR_RECT_HPP_INCLUDED

struct rect {
  int          x     {0};
  int          y     {0};
  unsigned int width {0};
  unsigned int height{0};



  bool operator==(const rect&) const = default;
};

#endif // WALLPABLUR_RECT_HPP_INCLUDED
