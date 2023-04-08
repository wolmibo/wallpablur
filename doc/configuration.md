# Configuration Of WallpaBlur

WallpaBlur uses an INI-derived configuration. It can be passed either as a string via the
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

[panels]
# - anchor =; size = 0:0; margin = 0:0:0:0; focused = false; urgent = false; app-id = ""

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
# - type = border; thickness = 2; position = outside; offset = 0,0;
#   color = #000000; blend = alpha; falloff = none; exponent = 1; enable-if = true
# - type = shadow; thickness = 30; position = centered; offset = 2,2;
#   color = #000000cc; blend = alpha; falloff = sinusoidal; exponent = 1.5;
#   enable-if = true
# - type = glow; thickness = 20; position = outside; offset = 0,0;
#   color = #ffffff; blend = add; falloff = linear; exponent = 3; enable-if = true

# [OUTPUT.panels]
# override panels for OUTPUT

# [OUTPUT.wallpaper]
# override wallpaper for OUTPUT

# [OUTPUT.background]
# override background for OUTPUT

# [OUTPUT.surface-effects]
# override surface-effects for OUTPUT


```

## Syntax
The configuration consists of the following syntactical elements:
* Comments: introduced by `#` at the beginning of a line until the end of that line
* (Named) sections: introduced by a section header `[name of section]` until the next
  section header or until cleared by returning to the global section via `[]`.
  Subsections are denoted by `[section.subsection]`.
* (Anonymous) groups: introduced by `-` inside a section or by a repeating the section
  header (they mimic lists and are described in `[panels]` below)
* Key-value pairs: introduced by a string, separated by `=` and ended at the end of line,
  at a section header, at a group divider `-`, or with a semicolon `;`.

All keys, values, and (sub)section names are syntactically strings and are subject to the
following rules:

* when started with a single quotation mark `'` everything after this character up until
  the next `'` is the content of the string
* when started with a double quotation mark `"` everything after this character up until
  the next `"` is the content of the string, escape sequences get resolved
* for unquoted strings leading and trailing spaces get trimmed and escape sequences
  get resolved
* the escape sequence `\n` is resolved as new line, while any other sequence `\<char>` is
  resolved as `<char>`

In particular, the following are equivalent:
```ini
number=10
number = 10
number = \1\0
number = "1\0"
number = '10'
```

Note that these rules (unless your using comments) allow the configuration file to be
written in a single line to be passed as argument via `--config-string`.

### File Paths
File paths can be specified as:
* absolute path
* $HOME-relative paths starting with `~/`
* relative paths:
  - relative to the configuration file if it exists
  - relative to the execution directory of wallpablur otherwise

## Options

### Global Options
The following options are defined in the global section, e.g. on top of the file.

#### `poll-rate-ms`
How frequently to poll the i3ipc for changes related to window resizing and window moving
(all other events are subscription based and unaffected by this setting).

Default value: `250`

#### `fade-in-ms`
How long to fade in on startup for a smoother experience.

Default value: `0`

#### `fade-out-ms`
How long to fade out on receiving a `SIGTERM` or `SIGINT` (e.g. `kill` or C-c in a
terminal).

Default value: `0`

#### `disable-i3ipc`
Disable i3ipc and only use the wallpaper.

Default value: `false`

### `[panels]`
Panels are used to mark regions of the screen as "always use the background", e.g. to
draw the background of your status bar.
All sizes are in pixels and refer to the respective size at output scale 1.
Each panel has the following properties (which are meant to reflect the corresponding
properties of the
[layer-shell protocol](https://wayland.app/protocols/wlr-layer-shell-unstable-v1):

#### `anchor`
Describes to which sides of the screen the panel is attached to.
It is specified as a (potentially empty) string consisting of 'l', 'r', 'b' and 't' to
indicated if the panel is attached to the **l**eft, **r**ight, **b**ottom or **t**op of
the screen.
The order (or multiplicity) does not matter.
For instance, a status bar on the bottom and spanning the width of the screen is attached
to the left, bottom, and right, i.e. `anchor = lbr`.

Default value: `""`

#### `size`
Specifies the size of the panel as `<width>:<height>`. If you want the panel to span the
width or height of the screen use `0` for the respective dimension and the appropriate
`anchor`.

Default value: `0:0`

#### `margin`
How much space to leave between the screen edges and the panel. It can be either
specified as a uniform margin `<int>` or explicitly for each side as
`<left>:<right>:<top>:<bottom>`.

Default value: `0`

#### `focused`
Sets the `focused` flag for this panel.

Default value: `false`

#### `urgent`
Sets the `urgent` flag for this panel.

Default value: `false`

#### `app-id`
Sets the `app-id` for this panel.

Default value: `""`

#### Multiple Panels

Each of the above properties is only allowed once per panel. To specify multiple panels
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
same keys)

### `[wallpaper]`
The wallpaper is created as follows:

If a *path* is specified and the file can be decoded:
* create a new image with the size of the output cleared with *color*
* draw the image from *path* on top using alpha-blending, *scale*, *scale-filter*,
  *wrap-x*, and *wrap-y*
* apply the filters in the specified order
Otherwise *color* is used as background color as-is.

#### `color`
The color with which the *wallpaper* is initialized. The color is specified as
case-insensitive hexadecimal string of the form `[#]rrggbb[aa]` or `[#]rgb[a]`.

Default value: `#00000000`

#### `path`
The file path of the image to use as *wallpaper*.

Default value: not specified

#### `scale`
How to scale the image if its resolution does not match the monitors. Possible values:

* `fit`: aspect ratio is preserved while the image has the largest possible size without
  being cropped
* `zoom`: aspect ratio is preserved while the image has the smallest possible size while
  covering the entire screen
* `stretch`: stretch the image to fit the screen
* `centered`: do not rescale the image

Default value: `zoom`

#### `scale-filter`
How to sample pixels when rescaling the image. Possible values:

* `linear`: use (bi)linear interpolation
* `nearest`: use nearest-neighbor interpolation

Default value: `linear`

#### `wrap` or `wrap-x` and `wrap-y`
What to do, when the rescaled image does not cover the whole screen. Possible values:

* `none`: use the background color
* `stretch-edge`: paint the remaining space using the pixels on the edge
* `tiled`: repeat the image
* `tiled-mirror`: repeat the image but flipped along the common edge

Use `wrap` to specify both the horizontal and vertical behavior at the same time or
`wrap-x` and `wrap-y` to specify them separately.

Default value: `none`

#### `filter`
Filters to apply to the *wallpaper*. Multiple filters can be specified when they are
separated into individual groups (compare `[panels]`).

##### `filter = invert`

Inverts the colors of the image.

##### `filter = box-blur`
Apply a box blur to the image. Additional properties:
* `radius`: how much to blur in horizontal or vertical direction. Default: `96`
* `width`: override how much to blur in the horizontal direction.
  Default: uses value of `radius`
* `height`: override how much to blur in the vertical direction.
  Default: uses value of `radius`
* `iterations`: how often to apply the filter. Default: `1`
* `dithering`: how much noise to add to reduce color banding. Default: `1.0`


##### `filter = blur`
Same as `filter = box-blur`, but with `iterations = 2` as default.


#### Example
To invert the colors and apply a rectangular blur:
```ini
[wallpaper]
path = <filepath>
- filter = invert
- filter = box-blur; width = 20; height = 10

```


### `[background]`
`[background]` is identical to `[wallpaper]` with the following exception:
If no `path` is specified, but one is specified in `[wallpaper]`,
all properties except the filters are ignored and
the filters are applied **on top** of the *wallpaper*
(compare the example for `wallpablur <image>` in the introduction).
If you explicitly do not want an image as *background* but as *wallpaper*,
set `path =` to be empty.

Additionally, `enable-if` can be used to restrict the background to specific surfaces
(See section about conditions below).


### `[surface-effects]`
Effects that are applied on a per surface (window or panel) basis
(currently rendered between wallpaper and background).
They are specified as list where each item requires a `type` property.

Furthermore, each item may contain an `enable-if` property (defaulted to `true`) to
restrict the effect to specific surfaces (See section about conditions below).

##### `type = border`
Draws a border effect around surfaces. Additional properties:
* `thickness`: thickness of the border in pixels. Default: `2`
* `position`: where to draw the border with respect to the surface border.
  Possible values: `outside`, `centered`. Default: `outside`
* `offset`: translate the border by the given 2-dim vector. Default: `0,0`
* `color`: color of the border. Default: `#000000`
* `blend`: how to mix the color of the border with the wallpaper. Possible values:
  `alpha`, `add`. Default: `alpha`
* `falloff`: How to fade the color accross the thickness. Possible values:
  `none`, `linear`, `sinusoidal`. Default: `none`
* `exponent`: Raise the falloff to this power. Default: `1`

##### `type = shadow`
Same as `type = border`, but with changed defaults:
`thickness = 30`, `position = centered`, `offset = 2,2`, `color = #000000cc`,
`falloff = sinusoidal`, `exponent = 1.5`.

##### `type = glow`
Same as `type = border`, but with changed defaults:
`thickness = 20`, `color = #ffffff`, `blend = add`, `falloff = linear`, `exponent = 3`.




#### Example
To enable shadows for all surfaces and highlight the focused window with a blue glow:
```ini
[surface-effects]
- type = shadow
- type = glow; color = #4488ff; enable-if = focused
```


### Surface Conditions
A surface condition is a statement that evaluates to true or false for any given surface.
It is formed from one of the following terms:

* `true`: always true
* `false`: always false
* `focused`: whether the surface is focused
* `urgent`: whether the surface is urgent
* `panel`: whether the surface is a panel (i.e. defined in `[panels]`)
* `floating`: whether the surface is a floating window
* `tiled`: whether the surface is a tiled window

Furthermore it can be a string comparison of the form:

* `var == <string>`: variable `var` equals `<string>`
* `var == <string>*`: variable `var` starts with `<string>`
* `var == *<string>`: variable `var` ends with `<string>`
* `var == *<string>*`: variable `var` contains `<string>`

where `<string>` is any string subject to the same quoting/escaping rules as every string
in this config. Note that `*`, `(`, `)`, `|`, `!`, and `=` in a string need to be escaped
(or the string put in quotation marks).
Available string variables:

* `app_id`: the app id of the current surface


These terms can be combined using the following operators (in descending precendence):

* parentheses `(<expression>)`
* logical not `!<expression>`
* logical and `<left expression> && <right expression>`
* logical or `<left expression> || <right expression>`

#### Examples

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


## Configuring Individual Outputs
The sections above can be overwritten for a specific output `NAME`
(e.g. `eDP-1` or `HDMI-A-1`) by providing a `[NAME.panels]`, `[NAME.wallpaper]` or
`[NAME.background]` section, respectively.
Without using quotation marks or escaping, `NAME` must not contain `.` or `]`.

Example:
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
The *wallpaper* on `eDP-1` will also be blurred (but based on `image1.png`),
since `[eDP-1.background]` is not specified and so `[background]` is used as fallback.
