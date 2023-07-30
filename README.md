# lua-plum

Lua bindings for the [libplum](https://github.com/aaaaaa123456789/libplum/) image handling library.

Licensed under the Unlicense, same as the parent library.

More complete documentation is provided in the parent project; relevant sections are linked below.

## Constants

[C library documentation](https://github.com/aaaaaa123456789/libplum/blob/master/docs/constants.md)

| Name | Description |
| ---- | ----------- |
| plum.COLOR_32 | ARGB8888 color type. |
| plum.COLOR_16 | ARGB1555 color type. |
| plum.COLOR_64 | ARGB16161616 color type. |
| plum.COLOR_32X | ARGB2101010 color type. |
| plum.COLOR_MASK | Color bitmask for color type flags. |
| plum.ALPHA_INVERT | Flag for color types which un-invert the alpha (by default, the alpha is inverted). |
| plum.PALETTE_LOAD | |
| plum.PALETTE_GENERATE | |
| plum.PALETTE_FORCE | |
| plum.PALETTE_MASK | |
| plum.SORT_LIGHT_FIRST | |
| plum.SORT_DARK_FIRST | |
| plum.ALPHA_REMOVE | |
| plum.SORT_EXISTING | |
| plum.PALETTE_REDUCE | |
| plum.IMAGE_NONE | |
| plum.IMAGE_BMP | |
| plum.IMAGE_GIF | |
| plum.IMAGE_PNG | |
| plum.IMAGE_APNG | |
| plum.IMAGE_JPEG | |
| plum.IMAGE_PNM | |
| plum.NUM_IMAGE_TYPES | |
| plum.METADATA_NONE | |
| plum.METADATA_COLOR_DEPTH | |
| plum.METADATA_BACKGROUND | |
| plum.METADATA_LOOP_COUNT | |
| plum.METADATA_FRAME_DURATION | |
| plum.METADATA_FRAME_DISPOSAL | |
| plum.METADATA_FRAME_AREA | |
| plum.NUM_METADATA_TYPES | |
| plum.DISPOSAL_NONE | |
| plum.DISPOSAL_BACKGROUND | |
| plum.DISPOSAL_PREVIOUS | |
| plum.DISPOSAL_REPLACE | |
| plum.DISPOSAL_BACKGROUND_REPLACE | |
| plum.DISPOSAL_PREVIOUS_REPLACE | |
| plum.NUM_DISPOSAL_METHODS | |

### Error codes

| Name | Description |
| ---- | ----------- |
| plum.OK | |
| plum.ERR_INVALID_ARGUMENTS | |
| plum.ERR_INVALID_FILE_FORMAT | |
| plum.ERR_INVALID_COLOR_INDEX | |
| plum.ERR_TOO_MANY_COLORS | |
| plum.ERR_UNDEFINED_PALETTE | |
| plum.ERR_IMAGE_TOO_LARGE | |
| plum.ERR_NO_DATA | |
| plum.ERR_NO_MULTI_FRAME | |
| plum.ERR_FILE_INACCESSIBLE | |
| plum.ERR_FILE_ERROR | |
| plum.ERR_OUT_OF_MEMORY | |
| plum.NUM_ERRORS | |

## Fields

| Name | Description |
| ---- | ----------- |
| image.type | |
| image.width | |
| image.height | |
| image.color | |
| image.alpha_invert | |
| image.frames | |
| image.palette | |
| color.id | |
| color.mask | |
| color.shift | |
| color.width | |

## Methods

[C library documentation](https://github.com/aaaaaa123456789/libplum/blob/master/docs/functions.md)

| Name | Description |
| ---- | ----------- |
| libplum.new(width, height, frames, flags...) | |
| libplum.load(buffer, flags...) | Load image file from string. |
| libplum.loadfile(filename, flags...) | Load image file from filename. |
| libplum.error_text(code) | Show the error string for a given return code. |
| libplum.file_format_name(type) | Show the file format for a given image type. |
| libplum.version() | Parent library version. |
| libplum.check_valid_image_size(width, height[, frames]) | |
| image:get(x, y, [z, [width, height]]) | Get a pixel or group of pixels, as color values. |
| image:set(x, y, [z, [width, height]], table or pixel) | Set a pixel or group of pixels, as color values. |
| image:copy() | Create copy of image. |
| image:store() | Store image to buffer; returns string of type specified in `image.type`. |
| image:storefile(filename) | Store image to filename. |
| image:validate() | Validate image. |
| image:rotate(count, flip) | Rotate image by `count` clockwise rotations, optionally flipping vertically - [see example](https://github.com/aaaaaa123456789/libplum/blob/master/docs/rotation.md). |
| image:convert_colors(target) | Convert to a target color space. |
| image:remove_alpha() | Remove transparency data from image.  |
| image:sort_palette([sorting_function, ] flags...) | |
| image:reduce_palette() | Remove duplicate palette entries. |
| image:highest_used_palette_index() | Return the highest palette index actually in use. |
| image:to_indexed() | Convert an RGBA image to an indexed image. |
| image:to_rgba() | Convert an indexed image to an RGBA image. |
| color:unpack(value[, normalized]) | Unpack a color value into a four-value table; `normalized` returns 0..1 floats |
| color:pack({r, g, b, a}[, normalized]) | Pack a four-value table into a color value. |
| color:convert(value, target) | Convert a color value from one color space to another. |

## TODO

* Support reading metadata (looping information, etc).
* Support writing metadata.
