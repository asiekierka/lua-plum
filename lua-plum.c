#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "libplum/libplum.h"

#define LUAPLUM_COLOR_MT "luaplum_color"
#define LUAPLUM_COLOR_ARRAY_MT "luaplum_color_array"
#define LIBPLUM_IMAGE_MT "plum_image"
#define LUAPLUM_IMAGE_PALETTE_MT "luaplum_image_palette"

static int __libplumL_index_funcs_fallback(lua_State *L, const char *key, const luaL_Reg *func) {
    while (func->name != NULL) {
        if (!strcmp(func->name, key)) {
            lua_pushcfunction(L, func->func);
            return 1;
        }
        func++;
    }

    lua_pushnil(L);
    return 1;
}

// libplum color abstraction

struct luaplum_color {
    uint32_t id;
    const char *name;
    uint64_t mask[4];
    uint64_t shift[4];
    uint64_t width[4];
};

static const struct luaplum_color luaplum_colors[] = {
    {
        PLUM_COLOR_32, "COLOR_32",
        { PLUM_RED_MASK_32, PLUM_GREEN_MASK_32, PLUM_BLUE_MASK_32, PLUM_ALPHA_MASK_32 },
        { 0, 8, 16, 24 },
        { 8, 8, 8, 8 }
    },
    {
        PLUM_COLOR_64, "COLOR_64",
        { PLUM_RED_MASK_64, PLUM_GREEN_MASK_64, PLUM_BLUE_MASK_64, PLUM_ALPHA_MASK_64 },
        { 0, 16, 32, 48 },
        { 16, 16, 16, 16 }
    },
    {
        PLUM_COLOR_16, "COLOR_16",
        { PLUM_RED_MASK_16, PLUM_GREEN_MASK_16, PLUM_BLUE_MASK_16, PLUM_ALPHA_MASK_16 },
        { 0, 5, 10, 15 },
        { 5, 5, 5, 1 }
    },
    {
        PLUM_COLOR_32X, "COLOR_32X",
        { PLUM_RED_MASK_32X, PLUM_GREEN_MASK_32X, PLUM_BLUE_MASK_32X, PLUM_ALPHA_MASK_32X },
        { 0, 10, 20, 30 },
        { 10, 10, 10, 2 }
    }
};
#define LUAPLUM_COLOR_MT_COUNT 4

static inline lua_Integer luaplum_color_extract(struct luaplum_color *color, uint64_t value, int component) {
    return (value & color->mask[component]) >> color->shift[component];
}

static inline lua_Number luaplum_color_normalize(struct luaplum_color *color, lua_Integer value, int component) {
    return ((lua_Number) value) / ((1 << color->width[component]) - 1);
}

static inline lua_Number luaplum_color_extract_normalize(struct luaplum_color *color, uint64_t value, int component) {
    return luaplum_color_normalize(color, luaplum_color_extract(color, value, component), component);
}

static inline uint64_t luaplum_color_insert(struct luaplum_color *color, uint64_t value, lua_Integer element, int component) {
    uint64_t mask = color->mask[component];
    return (value & ~mask) | ((element << color->shift[component]) & mask);
}

static inline lua_Integer luaplum_color_denormalize(struct luaplum_color *color, lua_Number value, int component) {
    return (lua_Integer) (value * ((1 << color->width[component]) - 1));
}

static inline uint64_t luaplum_color_insert_denormalize(struct luaplum_color *color, uint64_t value, lua_Number element, int component) {
    return luaplum_color_insert(color, value, luaplum_color_denormalize(color, element, component), component);
}

static struct luaplum_color *libplumL_checkcolor(lua_State *L, int i) {
    void *ptr = luaL_checkudata(L, i, LUAPLUM_COLOR_MT);
    luaL_argcheck(L, ptr != NULL, i, "`color' expected");
    return *((struct luaplum_color **) ptr);
}

static unsigned libplumL_checkcolorid(lua_State *L, int i) {
    if (lua_isuserdata(L, i)) {
        return libplumL_checkcolor(L, i)->id;
    } else {
        return luaL_checkinteger(L, i);
    }
}

static uint64_t libplumL_checkcolorvalue(lua_State *L, int i, struct luaplum_color *color) {
    if (lua_istable(L, i)) {
        uint64_t value = 0;
        for (int i = 0; i < 4; i++) {
            lua_geti(L, 2, i + 1);
            if (lua_isnumber(L, i)) {
                value = luaplum_color_insert_denormalize(color, value, luaL_checknumber(L, -1), i);
            } else {
                value = luaplum_color_insert(color, value, luaL_checkinteger(L, -1), i);
            }
        }
        return value;
    } else {
        return luaL_checkinteger(L, i);
    }
}

static int libplumL_color_unpack(lua_State *L) {
    struct luaplum_color *color = libplumL_checkcolor(L, 1);
    lua_Integer value = luaL_checkinteger(L, 2);
    bool normalized = luaL_optinteger(L, 3, 0);
    for (int i = 0; i < 4; i++) {
        if (normalized) {
            lua_pushnumber(L, luaplum_color_extract_normalize(color, value, i));
        } else {
            lua_pushinteger(L, luaplum_color_extract(color, value, i));
        }
    }
    return 4;
}

static int libplumL_color_pack(lua_State *L) {
    struct luaplum_color *color = libplumL_checkcolor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    bool normalized = luaL_optinteger(L, 3, 0);

    uint64_t value = 0;
    for (int i = 0; i < 4; i++) {
        lua_geti(L, 2, i + 1);
        if (normalized) {
            value = luaplum_color_insert_denormalize(color, value, luaL_checknumber(L, -1), i);
        } else {
            value = luaplum_color_insert(color, value, luaL_checkinteger(L, -1), i);
        }
    }

    lua_pushinteger(L, value);
    return 1;
}

static int libplumL_color_convert(lua_State *L) {
    struct luaplum_color *from = libplumL_checkcolor(L, 1);
    uint64_t value = libplumL_checkcolorvalue(L, 1, from);
    unsigned to = libplumL_checkcolorid(L, 2);
    lua_pushinteger(L, plum_convert_color(value, from->id, to));
    return 1;
}

static const luaL_Reg plum_color_funcs[] = {
    { "unpack", libplumL_color_unpack },
    { "pack", libplumL_color_pack },
    { "convert", libplumL_color_convert },
    { NULL, NULL }
};

static void libplumL_pushcolorarray(lua_State *L, const uint64_t *array) {
    const uint64_t ** container = lua_newuserdata(L, sizeof(const uint64_t *));
    *container = array;

    luaL_getmetatable(L, LUAPLUM_COLOR_ARRAY_MT);
    lua_setmetatable(L, -2);
}

static int libplumL_color_index(lua_State *L) {
    struct luaplum_color *color = libplumL_checkcolor(L, 1);
    const char *key = luaL_checklstring(L, 2, NULL);

    if (!strcmp(key, "id")) {
        lua_pushinteger(L, color->id);
        return 1;
    }
    if (!strcmp(key, "mask")) {
        libplumL_pushcolorarray(L, color->mask);
        return 1;
    }
    if (!strcmp(key, "shift")) {
        libplumL_pushcolorarray(L, color->shift);
        return 1;
    }
    if (!strcmp(key, "width")) {
        libplumL_pushcolorarray(L, color->width);
        return 1;
    }

    return __libplumL_index_funcs_fallback(L, key, plum_color_funcs);
}

static int libplumL_color_array_index(lua_State *L) {
    void *ptr = luaL_checkudata(L, 1, LUAPLUM_COLOR_ARRAY_MT);
    luaL_argcheck(L, ptr != NULL, 1, "`color array' expected");
    uint64_t *table = *((uint64_t **) ptr);
    if (lua_isnumber(L, 2)) {
        lua_Integer key = lua_tonumber(L, 2);
        if (key >= 1 && key <= 4) lua_pushinteger(L, table[key - 1]);
        else lua_pushnil(L);
    } else {
        const char *key = lua_tostring(L, 2);
        if (!strcmp(key, "red") || !strcmp(key, "r")) lua_pushinteger(L, table[0]);
        else if (!strcmp(key, "green") || !strcmp(key, "g")) lua_pushinteger(L, table[1]);
        else if (!strcmp(key, "blue") || !strcmp(key, "b")) lua_pushinteger(L, table[2]);
        else if (!strcmp(key, "alpha") || !strcmp(key, "a")) lua_pushinteger(L, table[3]);
        else lua_pushnil(L);
    }
    return 1;
}

static int libplumL_color_array_eq(lua_State *L) {
    void *ap = luaL_testudata(L, 1, LUAPLUM_COLOR_ARRAY_MT);
    void *bp = luaL_testudata(L, 2, LUAPLUM_COLOR_ARRAY_MT);
    lua_pushboolean(L, ap != NULL && bp != NULL && *((uint64_t **) ap) == *((uint64_t **) bp));
    return 1;
}

static int libplumL_color_array_len(lua_State *L) {
    lua_pushinteger(L, 4);
    return 1;
}

// libplum wrappers

static unsigned __libplumL_or_flags(lua_State *L, int n) {
    unsigned flags = 0;
    for (int i = n; i <= lua_gettop(L); i++) {
        if (lua_isuserdata(L, i)) {
            flags |= libplumL_checkcolorid(L, i);
        } else {
            flags |= luaL_checkinteger(L, i);
        }
    }
    return flags;
}

static int __libplumL_return_code(lua_State *L, int error) {
    if (error) {
        lua_pushnil(L);
        lua_pushinteger(L, error);
        return 2;
    } else {
        lua_pushboolean(L, true);
        return 1;
    }
}

static uint64_t __libplumL_read_pixel(void *data, unsigned color_format, size_t position) {
    color_format &= PLUM_COLOR_MASK;
    if (color_format == PLUM_COLOR_16) {
        return ((uint16_t *) data)[position];
    } else if (color_format == PLUM_COLOR_64) {
        return ((uint64_t *) data)[position];
    } else if (color_format == PLUM_COLOR_32) {
        return ((uint32_t *) data)[position];
    } else if (color_format == PLUM_COLOR_32X) {
        return ((uint32_t *) data)[position];
    } else {
        return 0;
    }
}

static void __libplumL_write_pixel(void *data, unsigned color_format, size_t position, uint64_t value) {
    color_format &= PLUM_COLOR_MASK;
    if (color_format == PLUM_COLOR_16) {
        ((uint16_t *) data)[position] = value;
    } else if (color_format == PLUM_COLOR_64) {
        ((uint16_t *) data)[position] = value;
    } else if (color_format == PLUM_COLOR_32) {
        ((uint16_t *) data)[position] = value;
    } else if (color_format == PLUM_COLOR_32X) {
        ((uint16_t *) data)[position] = value;
    }
}

static struct plum_image *libplumL_checkimage(lua_State *L, int i, const char *mt) {
    void *ptr = luaL_checkudata(L, i, mt);
    luaL_argcheck(L, ptr != NULL, i, "`image' expected");
    return *((struct plum_image **) ptr);
}

static void __libplumL_image_inc_refcount(lua_State *L, struct plum_image *image) {
    if (image->userdata == NULL) {
        image->userdata = malloc(sizeof(uint64_t));
        if (!image->userdata) {
            luaL_error(L, "out of memory");
        }
        *((uint64_t*) image->userdata) = 0;
    }
    (*((uint64_t *) image->userdata))++;
}

static void __libplumL_image_dec_refcount(lua_State *L, struct plum_image *image) {
    uint64_t *counter = (uint64_t*) image->userdata;
    if (*counter == 1) {
      plum_destroy_image(image);
      free(image->userdata);
    } else {
        (*counter)--;
    }
}

static int __libplumL_image_destroy(lua_State *L, const char *mt) {
    struct plum_image *image = libplumL_checkimage(L, 1, mt);
    __libplumL_image_dec_refcount(L, image);
    lua_pushboolean(L, true);
    return 1;
}

static void libplumL_pushimage(lua_State *L, struct plum_image *image, const char *mt) {
    struct plum_image ** image_container = lua_newuserdata(L, sizeof(struct plum_image *));
    *image_container = image;
    __libplumL_image_inc_refcount(L, image);

    luaL_getmetatable(L, mt);
    lua_setmetatable(L, -2);
}

static int libplumL_image_destroy(lua_State *L) {
    return __libplumL_image_destroy(L, LIBPLUM_IMAGE_MT);
}

static int libplumL_image_palette_destroy(lua_State *L) {
    return __libplumL_image_destroy(L, LUAPLUM_IMAGE_PALETTE_MT);
}

static int libplumL_image_new(lua_State *L) {
    struct plum_image *image = plum_new_image();
    if (!image) {
        luaL_error(L, "out of memory");
    }
    unsigned flags = __libplumL_or_flags(L, 4);
    image->width = luaL_checkinteger(L, 1);
    image->height = luaL_checkinteger(L, 2);
    image->frames = luaL_checkinteger(L, 3);
    image->color_format = flags & (PLUM_COLOR_MASK | PLUM_ALPHA_INVERT);
    image->type = PLUM_IMAGE_NONE;
    size_t size = image->width * image->height * image->frames;
    if (flags & PLUM_PALETTE_MASK) {
        // create indexed image
        image->data = plum_malloc(image, size);
        image->palette = plum_malloc(image, plum_color_buffer_size(1, image->color_format));
        if (!image->data || !image->palette) {
            plum_destroy_image(image);
            luaL_error(L, "out of memory");
        }
        image->max_palette_index = 0;

        memset(image->data, 0, size);
        memset(image->palette, 0, plum_color_buffer_size(1, image->color_format));
    } else {
        // create RGBA image
        image->data = plum_malloc(image, plum_color_buffer_size(size, image->color_format));
        if (!image->data) {
            plum_destroy_image(image);
            luaL_error(L, "out of memory");
        }
        image->palette = NULL;

        memset(image->data, 0, plum_color_buffer_size(size, image->color_format));
    }
    image->userdata = NULL;
    libplumL_pushimage(L, image, LIBPLUM_IMAGE_MT);
    return 1;
}

static int libplumL_image_copy(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    struct plum_image *new_image = plum_copy_image(image);
    new_image->userdata = NULL;
    libplumL_pushimage(L, new_image, LIBPLUM_IMAGE_MT);
    return 1;
}

static int __libplumL_image_load(lua_State *L, size_t mode) {
    size_t length = 0;
    const char *buffer = luaL_checklstring(L, 1, &length);
    unsigned flags = __libplumL_or_flags(L, 2);
    unsigned int error = 0;
    struct plum_image *image = plum_load_image(buffer, mode == 0 ? length : mode, flags, &error);
    if (error) {
        lua_pushnil(L);
        lua_pushinteger(L, error);
        return 2;
    } else {
        libplumL_pushimage(L, image, LIBPLUM_IMAGE_MT);
        return 1;
    }
}

static int libplumL_image_load(lua_State *L) {
    return __libplumL_image_load(L, 0);
}

static int libplumL_image_loadfile(lua_State *L) {
    return __libplumL_image_load(L, PLUM_MODE_FILENAME);
}

static int __libplumL_image_store(lua_State *L, size_t mode) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    struct plum_buffer buffer = { 0, NULL };
    void *source;
    if (mode == PLUM_MODE_FILENAME) {
        source = (void*) luaL_checklstring(L, 2, NULL);
    } else {
        source = &buffer;
    }
    unsigned int error = 0;
    plum_store_image(image, source, mode, &error);
    if (error) {
        lua_pushnil(L);
        lua_pushinteger(L, error);
        return 2;
    } else if (mode == PLUM_MODE_BUFFER) {
        lua_pushlstring(L, buffer.data, buffer.size);
        return 1;
    } else {
        lua_pushboolean(L, true);
        return 1;
    }
}

static int libplumL_image_store(lua_State *L) {
    return __libplumL_image_store(L, PLUM_MODE_BUFFER);
}

static int libplumL_image_storefile(lua_State *L) {
    return __libplumL_image_store(L, PLUM_MODE_FILENAME);
}

static int libplumL_image_validate(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    lua_pushinteger(L, plum_validate_image(image));
    return 1;
}

static int libplumL_error_text(lua_State *L) {
    lua_pushstring(L, plum_get_error_text(luaL_checkinteger(L, 1)));
    return 1;
}

static int libplumL_file_format_name(lua_State *L) {
    lua_pushstring(L, plum_get_file_format_name(luaL_checkinteger(L, 1)));
    return 1;
}

static int libplumL_version(lua_State *L) {
    lua_pushinteger(L, plum_get_version_number());
    return 1;
}

static int libplumL_check_valid_image_size(lua_State *L) {
    lua_Integer width = luaL_checkinteger(L, 1);
    lua_Integer height = luaL_checkinteger(L, 2);
    lua_Integer frames = luaL_optinteger(L, 3, 1);
    if (width <= 0 || height <= 0 || frames <= 0 || width > UINT32_MAX || height > UINT32_MAX || frames > UINT32_MAX) {
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, plum_check_valid_image_size(width, height, frames));
    }
    return 1;
}

static int libplumL_image_remove_alpha(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    plum_remove_alpha(image);
    return 0;
}

static int libplumL_image_convert_colors(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    unsigned to = __libplumL_or_flags(L, 2);
    if (image->palette) {
        // paletted image
        size_t size = image->max_palette_index + 1;
        void *new_colors = plum_malloc(image, plum_color_buffer_size(size, to));
        if (!new_colors) luaL_error(L, "out of memory");
        plum_convert_colors(new_colors, image->palette, size, to, image->color_format);
        plum_free(image, image->palette);
        image->palette = new_colors;
    } else {
        // non-paletted image
        size_t size = image->width * image->height * image->frames;
        void *new_colors = plum_malloc(image, plum_color_buffer_size(size, to));
        if (!new_colors) luaL_error(L, "out of memory");
        plum_convert_colors(new_colors, image->data, size, to, image->color_format);
        plum_free(image, image->data);
        image->data = new_colors;
    }
    image->color_format = to;
    return 0;
}


static int libplumL_image_rotate(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    int count = luaL_optinteger(L, 2, 1);
    int flip = luaL_optinteger(L, 3, 0);
    plum_rotate_image(image, count, flip);
    return 0;
}

static uint64_t libplumL_image_sort_palette_custom(void *userdata, uint64_t value) {
    lua_State *L = (lua_State *) userdata;
    lua_pushinteger(L, value);
    lua_call(L, 1, 1);
    uint64_t result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return result;
}

static int libplumL_image_sort_palette(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    unsigned flags;
    int error;
    if (lua_isfunction(L, 2)) {
        flags = __libplumL_or_flags(L, 3);
        error = plum_sort_palette_custom(image, libplumL_image_sort_palette_custom, L, flags);
    } else {
        flags = __libplumL_or_flags(L, 2);
        error = plum_sort_palette(image, flags);
    }
    return __libplumL_return_code(L, error);
}

static int libplumL_image_reduce_palette(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    return __libplumL_return_code(L, plum_reduce_palette(image));
}

static int libplumL_image_get_highest_palette_index(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    lua_pushinteger(L, plum_get_highest_palette_index(image));
    return 1;
}

static int libplumL_image_to_indexed(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    size_t size = image->width * image->height * image->frames;
    unsigned flags = lua_gettop(L) >= 2 ? __libplumL_or_flags(L, 2) : image->color_format;

    if (image->palette) {
        lua_pushinteger(L, 0);
        return 1;
    }

    uint8_t *new_pixels = plum_malloc(image, size);
    if (!new_pixels) luaL_error(L, "out of memory");
    void *new_palette = plum_malloc(image, plum_color_buffer_size(256, flags & PLUM_COLOR_MASK));
    if (!new_palette) {
        plum_free(image, new_pixels);
        luaL_error(L, "out of memory");
    }

    int error = plum_convert_colors_to_indexes(new_pixels, image->data, new_palette, size, flags);
    if (error >= 0) {
        plum_free(image, image->data);
        image->data = new_pixels;
        image->max_palette_index = error;
        image->palette = new_palette;
        lua_pushinteger(L, error);
        return 1;
    } else {
        return __libplumL_return_code(L, error);
    }
}

static int libplumL_image_to_rgba(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    size_t size = image->width * image->height * image->frames;
    unsigned flags = lua_gettop(L) >= 2 ? __libplumL_or_flags(L, 2) : image->color_format;

    if (!image->palette) {
        lua_pushboolean(L, false);
        return 1;
    }

    uint8_t *new_pixels = plum_malloc(image, plum_color_buffer_size(size, flags & PLUM_COLOR_MASK));
    if (!new_pixels) luaL_error(L, "out of memory");

    plum_convert_indexes_to_colors(new_pixels, image->data, image->palette, size, flags);
    plum_free(image, image->data);
    plum_free(image, image->palette);
    image->data = new_pixels;
    image->max_palette_index = 0;
    image->palette = NULL;
    return __libplumL_return_code(L, 0);
}

static int libplumL_image_get_index(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    lua_Integer x1 = luaL_checkinteger(L, 2);
    lua_Integer y1 = luaL_checkinteger(L, 3);
    lua_Integer z = luaL_optinteger(L, 4, 0);
    lua_Integer width = lua_gettop(L) >= 5 ? luaL_checkinteger(L, 5) : 1;
    lua_Integer height = lua_gettop(L) >= 5 ? luaL_checkinteger(L, 6) : 1;
    if (width <= 0 || height <= 0) {
        return 0;
    }

    for (lua_Integer yo = 0; yo < height; yo++) {
        lua_Integer y = y1 + yo;
        for (lua_Integer xo = 0; xo < width; xo++) {
            lua_Integer x = x1 + xo;

            if (x < 0 || y < 0 || z < 0 || x >= image->width || y >= image->height || z >= image->frames) {
                lua_pushnil(L);
            } else {
                size_t position = ((z * image->height) + y * image->width) + x;
                if (image->palette != NULL) {
                    lua_pushinteger(L, ((uint8_t *) image->data)[position]);
                } else {
                    lua_pushinteger(L, __libplumL_read_pixel(image->data, image->color_format, position));
                }
            }
        }
    }
    return width * height;
}

static int libplumL_image_set_index(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    lua_Integer x1 = luaL_checkinteger(L, 2);
    lua_Integer y1 = luaL_checkinteger(L, 3);
    lua_Integer z = lua_gettop(L) > 4 ? luaL_checkinteger(L, 4) : 0;
    lua_Integer width = lua_gettop(L) > 5 ? luaL_checkinteger(L, 5) : 1;
    lua_Integer height = lua_gettop(L) > 5 ? luaL_checkinteger(L, 6) : 1;
    if (width <= 0 || height <= 0) {
        return 0;
    }
    for (lua_Integer yo = 0; yo < height; yo++) {
        lua_Integer y = y1 + yo;
        for (lua_Integer xo = 0; xo < width; xo++) {
            lua_Integer x = x1 + xo;

            if (x < 0 || y < 0 || z < 0 || x >= image->width || y >= image->height || z >= image->frames) {
                lua_pushnil(L);
            } else {
                size_t position = ((z * image->height) + y * image->width) + x;
                size_t local_position = yo * width + xo;
                uint64_t value;
                if (lua_istable(L, -1)) {
                    lua_geti(L, -1, local_position + 1);
                    value = lua_tointeger(L, -1);
                    lua_pop(L, 1);
                } else {
                    value = lua_tointeger(L, -1);
                }

                if (image->palette != NULL) {
                    ((uint8_t *) image->data)[position] = value;
                } else {
                    __libplumL_write_pixel(image->data, image->color_format, position, value);
                }
            }
        }
    }
    return 0;
}

static const luaL_Reg plum_image_funcs[] = {
    { "get", libplumL_image_get_index },
    { "set", libplumL_image_set_index },
    { "copy", libplumL_image_copy },
    { "store", libplumL_image_store },
    { "storefile", libplumL_image_storefile },
    { "validate", libplumL_image_validate },
    { "rotate", libplumL_image_rotate },
    { "convert_colors", libplumL_image_convert_colors },
    { "remove_alpha", libplumL_image_remove_alpha },
    { "sort_palette", libplumL_image_sort_palette },
    { "reduce_palette", libplumL_image_reduce_palette },
// const uint8_t * plum_validate_palette_indexes(const struct plum_image * image);
    { "highest_used_palette_index", libplumL_image_get_highest_palette_index },
    { "to_indexed", libplumL_image_to_indexed },
    { "to_rgba", libplumL_image_to_rgba },
// void plum_sort_colors(const void * restrict colors, uint8_t max_index, unsigned flags, uint8_t * restrict result);
// struct plum_metadata * plum_allocate_metadata(struct plum_image * image, size_t size);
// unsigned plum_append_metadata(struct plum_image * image, int type, const void * data, size_t size);
// struct plum_metadata * plum_find_metadata(const struct plum_image * image, int type);
    { NULL, NULL }
};

static const luaL_Reg plum_funcs[] = {
// struct plum_image * plum_new_image(void);
    { "new", libplumL_image_new },
    { "load", libplumL_image_load },
    { "loadfile", libplumL_image_loadfile },
// struct plum_image * plum_load_image_limited(const void * restrict buffer, size_t size_mode, unsigned flags, size_t limit, unsigned * restrict error);
    { "error_text", libplumL_error_text },
    { "file_format_name", libplumL_file_format_name },
    { "version", libplumL_version },
    { "check_valid_image_size", libplumL_check_valid_image_size },
// int plum_check_limited_image_size(uint32_t width, uint32_t height, uint32_t frames, size_t limit);
    { NULL, NULL }
};

static int libplumL_image_index(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    const char *key = luaL_checklstring(L, 2, NULL);

    if (!strcmp(key, "width")) {
        lua_pushinteger(L, image->width);
        return 1;
    }
    if (!strcmp(key, "height")) {
        lua_pushinteger(L, image->height);
        return 1;
    }
    if (!strcmp(key, "frames")) {
        lua_pushinteger(L, image->frames);
        return 1;
    }
    if (!strcmp(key, "type")) {
        lua_pushinteger(L, image->type);
        return 1;
    }
    if (!strcmp(key, "color")) {
        const struct luaplum_color ** color_container = lua_newuserdata(L, sizeof(const struct plum_color *));
        *color_container = &luaplum_colors[image->color_format & PLUM_COLOR_MASK];

        luaL_getmetatable(L, LUAPLUM_COLOR_MT);
        lua_setmetatable(L, -2);
        return 1;
    }
    if (!strcmp(key, "alpha_invert")) {
        lua_pushboolean(L, (image->color_format & PLUM_ALPHA_INVERT) ? true : false);
        return 1;
    }
    if (!strcmp(key, "palette")) {
        if (image->palette != NULL) {
            libplumL_pushimage(L, image, LUAPLUM_IMAGE_PALETTE_MT);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    return __libplumL_index_funcs_fallback(L, key, plum_image_funcs);
}

static int libplumL_image_newindex(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LIBPLUM_IMAGE_MT);
    const char *key = luaL_checklstring(L, 2, NULL);

    if (!strcmp(key, "type")) {
        image->type = luaL_checkinteger(L, 3);
        return 0;
    }
    if (!strcmp(key, "palette")) {
        luaL_error(L, "unsupported assignment");
        return 0;
    }

    return 0;
}

static int libplumL_image_palette_len(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LUAPLUM_IMAGE_PALETTE_MT);

    lua_pushinteger(L, image->palette != NULL ? (lua_Integer) image->max_palette_index + 1 : 0);
    return 1;
}

static int libplumL_image_palette_index(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LUAPLUM_IMAGE_PALETTE_MT);
    lua_Integer key = luaL_checknumber(L, 2);

    if (image->palette != NULL && key >= 0 && key < image->max_palette_index) {
        lua_pushinteger(L, __libplumL_read_pixel(image->palette, image->color_format, key));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int libplumL_image_palette_newindex(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1, LUAPLUM_IMAGE_PALETTE_MT);
    lua_Integer key = luaL_checknumber(L, 2);
    uint64_t value = luaL_checkinteger(L, 3);

    if (image->palette != NULL && key >= 0 && key < 256) {
        if (key >= image->max_palette_index) {
            void *new_palette = plum_realloc(image, image->palette, plum_color_buffer_size(key + 1, image->color_format));
            if (!new_palette) {
                luaL_error(L, "out of memory");
            }
            image->palette = new_palette;
            image->max_palette_index = key + 1;
        }
        __libplumL_write_pixel(image->palette, image->color_format, key, value);
    }

    return 0;
}

static inline void __libplum_pushconst(lua_State *L, const char *name, int value) {
    lua_pushstring(L, name);
    lua_pushinteger(L, value);
    lua_settable(L, -3);
}
#define libplum_pushconst(L, c) __libplum_pushconst(L, (#c) + 5, c)

int luaopen_libplum(lua_State* L) {
    luaL_newlib(L, plum_funcs);
    luaL_setfuncs(L, plum_color_funcs, 0);
    luaL_setfuncs(L, plum_image_funcs, 0);

    libplum_pushconst(L, PLUM_COLOR_MASK);
    libplum_pushconst(L, PLUM_ALPHA_INVERT);
    libplum_pushconst(L, PLUM_PALETTE_LOAD);
    libplum_pushconst(L, PLUM_PALETTE_GENERATE);
    libplum_pushconst(L, PLUM_PALETTE_FORCE);
    libplum_pushconst(L, PLUM_PALETTE_MASK);
    libplum_pushconst(L, PLUM_SORT_LIGHT_FIRST);
    libplum_pushconst(L, PLUM_SORT_DARK_FIRST);
    libplum_pushconst(L, PLUM_ALPHA_REMOVE);
    libplum_pushconst(L, PLUM_SORT_EXISTING);
    libplum_pushconst(L, PLUM_PALETTE_REDUCE);

    libplum_pushconst(L, PLUM_IMAGE_NONE);
    libplum_pushconst(L, PLUM_IMAGE_BMP);
    libplum_pushconst(L, PLUM_IMAGE_GIF);
    libplum_pushconst(L, PLUM_IMAGE_PNG);
    libplum_pushconst(L, PLUM_IMAGE_APNG);
    libplum_pushconst(L, PLUM_IMAGE_JPEG);
    libplum_pushconst(L, PLUM_IMAGE_PNM);
    libplum_pushconst(L, PLUM_NUM_IMAGE_TYPES);

    libplum_pushconst(L, PLUM_METADATA_NONE);
    libplum_pushconst(L, PLUM_METADATA_COLOR_DEPTH);
    libplum_pushconst(L, PLUM_METADATA_BACKGROUND);
    libplum_pushconst(L, PLUM_METADATA_LOOP_COUNT);
    libplum_pushconst(L, PLUM_METADATA_FRAME_DURATION);
    libplum_pushconst(L, PLUM_METADATA_FRAME_DISPOSAL);
    libplum_pushconst(L, PLUM_METADATA_FRAME_AREA);
    libplum_pushconst(L, PLUM_NUM_METADATA_TYPES);

    libplum_pushconst(L, PLUM_DISPOSAL_NONE);
    libplum_pushconst(L, PLUM_DISPOSAL_BACKGROUND);
    libplum_pushconst(L, PLUM_DISPOSAL_PREVIOUS);
    libplum_pushconst(L, PLUM_DISPOSAL_REPLACE);
    libplum_pushconst(L, PLUM_DISPOSAL_BACKGROUND_REPLACE);
    libplum_pushconst(L, PLUM_DISPOSAL_PREVIOUS_REPLACE);
    libplum_pushconst(L, PLUM_NUM_DISPOSAL_METHODS);

    libplum_pushconst(L, PLUM_OK);
    libplum_pushconst(L, PLUM_ERR_INVALID_ARGUMENTS);
    libplum_pushconst(L, PLUM_ERR_INVALID_FILE_FORMAT);
    libplum_pushconst(L, PLUM_ERR_INVALID_COLOR_INDEX);
    libplum_pushconst(L, PLUM_ERR_TOO_MANY_COLORS);
    libplum_pushconst(L, PLUM_ERR_UNDEFINED_PALETTE);
    libplum_pushconst(L, PLUM_ERR_IMAGE_TOO_LARGE);
    libplum_pushconst(L, PLUM_ERR_NO_DATA);
    libplum_pushconst(L, PLUM_ERR_NO_MULTI_FRAME);
    libplum_pushconst(L, PLUM_ERR_FILE_INACCESSIBLE);
    libplum_pushconst(L, PLUM_ERR_FILE_ERROR);
    libplum_pushconst(L, PLUM_ERR_OUT_OF_MEMORY);
    libplum_pushconst(L, PLUM_NUM_ERRORS);

    luaL_newmetatable(L, LUAPLUM_COLOR_ARRAY_MT);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, libplumL_color_array_index);
    lua_settable(L, -3);

    lua_pushliteral(L, "__eq");
    lua_pushcfunction(L, libplumL_color_array_eq);
    lua_settable(L, -3);

    lua_pushliteral(L, "__len");
    lua_pushcfunction(L, libplumL_color_array_len);
    lua_settable(L, -3);

    lua_pop(L, 1);

    luaL_newmetatable(L, LUAPLUM_COLOR_MT);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, libplumL_color_index);
    lua_settable(L, -3);

    lua_pop(L, 1);

    luaL_newmetatable(L, LUAPLUM_IMAGE_PALETTE_MT);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, libplumL_image_palette_index);
    lua_settable(L, -3);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, libplumL_image_palette_newindex);
    lua_settable(L, -3);

    lua_pushliteral(L, "__len");
    lua_pushcfunction(L, libplumL_image_palette_len);
    lua_settable(L, -3);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, libplumL_image_palette_destroy);
    lua_settable(L, -3);
    
    lua_pop(L, 1);

    for (int i = 0; i < LUAPLUM_COLOR_MT_COUNT; i++) {
        lua_pushstring(L, luaplum_colors[i].name);

        const struct luaplum_color ** color_container = lua_newuserdata(L, sizeof(const struct plum_color *));
        *color_container = &luaplum_colors[i];

        luaL_getmetatable(L, LUAPLUM_COLOR_MT);
        lua_setmetatable(L, -2);

        lua_settable(L, -3);
    }
    
    luaL_newmetatable(L, LIBPLUM_IMAGE_MT);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, libplumL_image_index);
    lua_settable(L, -3);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, libplumL_image_newindex);
    lua_settable(L, -3);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, libplumL_image_destroy);
    lua_settable(L, -3);
    
    lua_pop(L, 1);

    return 1;
}
