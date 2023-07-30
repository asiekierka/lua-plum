# lua-plum

Lua bindings for the [libplum](https://github.com/aaaaaa123456789/libplum/) image handling library.

Licensed under the Unlicense, same as the parent library.

## Constants

| Name | Description |
| ---- | ----------- |
| plum.COLOR_32 | |
| plum.COLOR_16 | |
| plum.COLOR_64 | |
| plum.COLOR_32X | |
| plum.COLOR_MASK | |
| plum.ALPHA_INVERT | |
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

| Name | Description |
| ---- | ----------- |
| libplum.new(width, height, frames, flags...) | |
| libplum.load(buffer, flags...) | Load image file from string. |
| libplum.loadfile(filename, flags...) | Load image file from filename. |
| libplum.error_text(code) | Show the error string for a given return code. |
| libplum.file_format_name(type) | Show the file format for a given image type. |
| libplum.version() | Parent library version. |
| libplum.check_valid_image_size(width, height[, frames]) | |
| image:get(x, y, [z, [width, height]]) | |
| image:set(x, y, [z, [width, height]], table or pixel) | |
| image:copy() | |
| image:store() | |
| image:storefile(filename) | |
| image:validate() | |
| image:rotate(count, flip) | |
| image:convert_colors(target) | |
| image:remove_alpha() | |
| image:sort_palette([sorting_function, ] flags...) | |
| image:reduce_palette() | |
| image:highest_used_palette_index() | |
| image:to_indexed() | |
| image:to_rgba() | |
| color:unpack(value) | |
| color:pack({r, g, b, a}) | |
| color:convert(value, target) | |

## TODO

* Support reading metadata (looping information, etc).
* Support writing metadata.
