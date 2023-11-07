# Configuration Of WallpaBlur

WallpaBlur uses an INI-derived configuration (a detailed description of the file format
can be found [here](https://github.com/wolmibo/iconfigp/blob/main/doc/format.md)).
The configuration can be passed either as a string via the
`--config-string` flag or as a file via `--config`.
If no configuration is provided via parameters, the following places will be checked:

* `$WALLPABLUR_CONFIG`
* `$XDG_CONFIG_HOME/wallpablur/config.ini`
* `$HOME/.config/wallpablur/config.ini`

**Note**: When executing `wallpablur <imagefile>` a temporary configuration string is
created and all other configurations are ignored.
The equivalent configuration string in this case is:
```ini
[background]
path = <imagefile>

[wallpaper]
filter = blur
```




## Full Example Using Default Values
```ini
poll-rate-ms  = 250
fade-out-ms   = 0
fade-in-ms    = 0
disable-i3ipc = false

clipping      = false

[panels]
# - anchor =; size = 0:0; margin = 0:0:0:0; focused = false; urgent = false; app-id = ""
#   radius = 0; enable-if = true

[wallpaper]
color  = #00000000
# path =
scale  = zoom
wrap-x = none
wrap-y = none
scale-filter = linear
# - filter = invert
# - filter = blur;     iterations = 2; radius = 96; dithering = 1.0
# - filter = box-blur; iterations = 1; radius = 96; dithering = 1.0

[background]
# same as [wallpaper] and additionally:
enable-if = true

[surface-effects]
# - type = border; thickness = 2; position = outside; offset = 0,0; sides = lrtb;
#   color = #000000; blend = alpha; falloff = none; exponent = 1; enable-if = true
# - type = shadow; thickness = 30; position = centered; offset = 2,2; sides = lrtb;
#   color = #000000cc; blend = alpha; falloff = sinusoidal; exponent = 1.5;
#   enable-if = true
# - type = glow; thickness = 20; position = outside; offset = 0,0; sides = lrtb;
#   color = #ffffff; blend = add; falloff = linear; exponent = 3; enable-if = true
# - type = rounded-corners; radius = 10; enable-if = true

# [OUTPUT]
# clipping = false

# [OUTPUT.panels]
# override panels for OUTPUT

# [OUTPUT.wallpaper]
# override wallpaper for OUTPUT

# [OUTPUT.background]
# override background for OUTPUT

# [OUTPUT.surface-effects]
# override surface-effects for OUTPUT
```




## Global Options
The following options are defined in the global section, e.g. on top of the file.
* `poll-rate-ms`: How often to poll the i3ipc for unsubscribable changes
* `disable-i3ipc`: Do not try to connect to i3ipc and simply draw the wallpaper on the
  respective output
* `fade-in-ms`: How long to perform an alpha cross-fade on startup
* `fade-out-ms`: How long to perform an alpha cross-fade on receiving `SIGTERM` or
  `SIGINT` (e.g. `kill` or C-c in a terminal)
* `clipping`: Whether to spawn a layer surface *in front* of all windows to clip rounded
  corners.
  This setting can:
  - affect overall graphics performance
  - punch holes in surfaces above tiled windows (e.g. floating windows, dialogs)
  Can be overwritten per output.





## Defining `[panels]`
Panels are used to mark regions of the screen as "always use the background", e.g. to
draw the background of a status bar.
All sizes are in pixels and refer to the respective size at output scale 1.
Each panel has the following properties (which are meant to reflect the corresponding
properties of the layer-shell protocol:
* `anchor`:
  Describes to which sides of the screen the panel is attached to.
  It is specified as a (potentially empty) string consisting of 'l', 'r', 'b' and 't' to
  indicated if the panel is attached to the **l**eft, **r**ight, **b**ottom or **t**op of
  the screen.
  The order (or multiplicity) does not matter.
  For instance, a status bar on the bottom and spanning the width of the screen is attached
  to the left, bottom, and right, i.e. `anchor = lbr`.
* `size`:
  Specifies the size of the panel as `<width>:<height>`. If you want the panel to span the
  width or height of the screen use `0` for the respective dimension and the appropriate
  `anchor`.
* `margin`:
  How much space to leave between the screen edges and the panel. It can be either
  specified as a uniform margin `<int>` or explicitly for each side as
  `<left>:<right>:<top>:<bottom>`.
* `radius`:
  Sets the border radius of this panel. For not fully transparent panels, check out the
  `clipping` option (c.f. global options).

To show the panel conditionally (e.g. only on certain workspaces) you can use the
`enable-if` property set to a workspace expression (see below).

### Surface Properties
Surfaces have various properties which can be used in surface expressions (see below).
You can set them manually for panels using the following keys
* `focused`
* `urgent`
* `app-id`


### Multiple Panels

Each of the above keys is only allowed once per panel. To specify multiple panels
the key-value pairs need to be grouped. A new group is either started by creating another
section with the same name or by writing `-`.
For example:
```ini
[panels]
anchor = lbr
size = 0:22

[panels]
anchor = ltr
size = 0:22
```

This will create one panel on the top of the screen and one on the bottom, both of height
22.
Alternatively, you can write:
```ini
[panels]
anchor = lbr
size = 0:22
-
anchor = ltr
size = 0:22
```
or, more compact:
```ini
[panels]
- anchor = lbr; size = 0:22
- anchor = ltr; size = 0:22
```
(the first `-` is optional for readability, the second one is required to separate the
duplicate keys)




## Setting the `[wallpaper]`
The wallpaper is created as follows:

If a *path* is specified and the file can be decoded:
* create a new image with the size of the output cleared with *color*
* draw the image from *path* on top using alpha-blending, *scale*, *scale-filter*,
  *wrap-x*, and *wrap-y*
* apply the filters in the specified order
Otherwise *color* is used as background color as-is.


### Properties
* `color`:
  The color with which the *wallpaper* is initialized. The color is specified as
  case-insensitive hexadecimal string of the form `[#]rrggbb[aa]` or `[#]rgb[a]`
* `path`:
  The file path of the image to use as *wallpaper*
* `scale`:
  How to scale the image if its resolution does not match the monitors. Possible values:
    - `fit`: aspect ratio is preserved while the image has the largest possible size
      without being cropped
    - `zoom`: aspect ratio is preserved while the image has the smallest possible size
      while covering the entire screen
    - `stretch`: stretch the image to fit the screen
    - `centered`: do not rescale the image
* `scale-filter`:
  How to sample pixels when rescaling the image. Possible values:
    - `linear`: use (bi)linear interpolation
    - `nearest`: use nearest-neighbor interpolation
* `wrap` or `wrap-x` and `wrap-y`:
  What to do, when the rescaled image does not cover the whole screen.
  Use `wrap` to specify both the horizontal and vertical behavior at the same time or
  `wrap-x` and `wrap-y` to specify them separately.
  Possible values:
    - `none`: use the background color
    - `stretch-edge`: paint the remaining space using the pixels on the edge
    - `tiled`: repeat the image
    - `tiled-mirror`: repeat the image but flipped along the common edge


### Available Filter
Filters to apply to the *wallpaper*. Multiple filters can be specified when they are
separated into individual groups using `-` (compare `[panels]`).

* `filter = invert`:
  Inverts the colors of the image.
* `filter = box-blur`:
  Apply a box blur to the image. Additional properties:
    - `radius`: how much to blur in horizontal or vertical direction
    - `width`: override `radius` for how much to blur in the horizontal direction
    - `height`: override `radius` for how much to blur in the vertical direction
    - `iterations`: how often to apply the filter
    - `dithering`: how much noise to add to reduce color banding
* `filter = blur`:
  Same as `filter = box-blur`, but with `iterations = 2` as default.


### Example
To invert the colors and apply a rectangular blur:
```ini
[wallpaper]
path = <filepath>
- filter = invert
- filter = box-blur; width = 20; height = 10

```




## Setting the `[background]`
`[background]` is similar to `[wallpaper]` with the following exceptions:
* If no `path` is specified, but one is specified in `[wallpaper]`,
  all properties except the filters are ignored and
  the filters are applied **on top** of the *wallpaper*
  (compare the example for `wallpablur <image>` in the introduction).
  If *background* is supposed to not have an image, set `path =` to be empty.

* `enable-if` can be used to restrict the background to specific surfaces or workspaces
  (See section about conditions below).




## Configuring Individual Outputs
The sections above can be overwritten for a specific output `NAME`
(e.g. `eDP-1` or `HDMI-A-1`) by providing a `[NAME.panels]`, `[NAME.wallpaper]` or
`[NAME.background]` section, respectively.
Without using quotation marks or escaping, `NAME` must not contain `.` or `]`.


### Example
Suppose you have a primary output `eDP-1` with status bar and `image1.png` and
potentially multiple other outputs without status bar and `image2.png`.
Furthermore, you want a basic blur effect on all *backgrounds*. Then you could use:
```ini
[wallpaper]
path = image2.png

[background]
filter = blur

[eDP-1.panels]
anchor = lbr; size = 0:22

[eDP-1.wallpaper]
path = image1.png
```
The *background* on `eDP-1` will also be blurred (but based on `image1.png`),
since `[eDP-1.background]` is not specified and so `[background]` is used as fallback.




## Using `[surface-effects]`
Surface effects are effects applied on a per surface (window or panel) basis
(currently rendered between wallpaper and background).
They are specified as list where each item requires a `type` property.

Furthermore, each item may contain an `enable-if` property to
restrict the effect to specific surfaces or workspaces
(See section about conditions below).

* `type = border`:
  Draws a border effect around surfaces. Additional properties:
    - `thickness`: thickness of the border in pixels
    - `position`: where to draw the border with respect to the surface border.
      Possible values: `outside`, `centered`
    - `offset`: translate the border by the given 2-dim vector
    - `color`: color of the border
    - `blend`: how to mix the color of the border with the wallpaper. Possible values:
      `alpha`, `add`
    - `falloff`: How to fade the color accross the thickness. Possible values:
      `none`, `linear`, `sinusoidal`
    - `exponent`: Raise the falloff to this power
    - `sides`: String made up of `l`, `r`, `t`, `b`, `n`, `w`, `e`, and `s`,
       indicating on what sides of the surface to draw the border:
       `[l]eft`, `[r]ight`, `[t]op`, `[b]ottom` in absolute terms, and
       `[n]orth`, `[w]est`, `[e]ast`, `[s]outh = [s]plit in this direction` relative
       to the current split direction


* `type = shadow`:
  Same as `type = border`, but with changed defaults:
  `thickness = 30`, `position = centered`, `offset = 2,2`, `color = #000000cc`,
  `falloff = sinusoidal`, `exponent = 1.5`

* `type = glow`:
  Same as `type = border`, but with changed defaults:
  `thickness = 20`, `color = #ffffff`, `blend = add`, `falloff = linear`, `exponent = 3`

* `type = rounded-corners`:
  Set the border radius of affected surfaces. For not fully transparent surfaces, check
  out the `clipping` option (c.f. global options).
  Additional properties:
    - `radius`: The new border radius (Default: 10)


### Example
To enable shadows for all surfaces and highlight the focused window with a blue glow:
```ini
[surface-effects]
- type = shadow
- type = glow; color = #4488ff; enable-if = focused
- type = rounded-corners; radius = 12; enable-if = (tiled || decoration) && !unique(tiled)
```




## Conditions and Expressions
To selectively enable features depending on the current surface,
surfaces on the current workspace, or workspace, logical expressions that evaluate to
true or false can be used.

They are formed from one of the following terms:
* `true`: always true
* `false`: always false
* context specific terms listed below

Furthermore it can be a string comparison of the form:
* `var == <string>`: variable `var` equals `<string>`
* `var == <string>*`: variable `var` starts with `<string>`
* `var == *<string>`: variable `var` ends with `<string>`
* `var == *<string>*`: variable `var` contains `<string>`

where `<string>` is any string subject to the same quoting/escaping rules as every string
in this config. Note that `*`, `(`, `)`, `|`, `!`, and `=` in a string need to be escaped
(or the string put in quotation marks).

These terms can be combined using the following operators (in descending precendence):
* parentheses `(<expression>)`
* logical not `!<expression>`
* logical and `<left expression> && <right expression>`
* logical or `<left expression> || <right expression>`

### Surface Conditions
A surface condition is checked for an individual surface and has the following terms:
* `focused`, `urgent`: whether the surface is focused / urgent
* `panel`, `floating`, `tiled`: whether the surface is a panel
   (i.e., defined in `[panels]`), a floating window, or a tiled window, respectively

And string variables:
* `app_id`: the app id of the current surface

### Workspace Conditions
A workspace condition is checked for the current workspace and all its contained surfaces,
with the following terms:
* `any(S)`, `all(S)`, `none(S)`, `unique(S)`: whether any / all / none / a unique
  surface(s) on the current workspace satisfy the surface condition `S`

And string variables:
* `ws_name`: the name of the current workspace
* `output`: the name of the current output

### Combined Surface and Workspace Conditions
Surface conditions can generally be combined with workspace conditions, except inside the
aggregator functions `any(...)`, etc. .

Note that `app_id == ws_name` (currently) checks whether `app_id` is literally `"ws_name"`
and not whether these to variables are the same.

### Examples
```
enable-if = true
```
```
enable-if = !floating
```
```
enable-if = !panel && (urgent || focused)
```
```
enable-if = !(app_id == foot)
```
```
enable-if = (app_id == foot*) && unique(tiled)
```
