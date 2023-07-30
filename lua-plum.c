#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "libplum/libplum.h"

#define LIBPLUM_IMAGE "plum_image"

static struct plum_image *libplumL_checkimage(lua_State *L, int i) {
    void *ptr = luaL_checkudata(L, i, LIBPLUM_IMAGE);
    luaL_argcheck(L, ptr != NULL, i, "`image' expected");
    return *((struct plum_image **) ptr);
}

static void libplumL_pushimage(lua_State *L, struct plum_image *image) {
    struct plum_image ** image_container = lua_newuserdata(L, sizeof(struct plum_image *));
    *image_container = image;

    luaL_getmetatable(L, LIBPLUM_IMAGE);
    lua_setmetatable(L, -2);
}

static int libplumL_image_destroy(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    plum_destroy_image(image);
    lua_pushboolean(L, true);
    return 1;
}

static int libplumL_image_copy(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    libplumL_pushimage(L, image);
    return 1;
}

static unsigned __libplumL_or_flags(lua_State *L, int n) {
    unsigned flags = 0;
    for (int i = n; i <= lua_gettop(L); i++) {
        flags |= luaL_checkinteger(L, i);
    }
    return flags;
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
        libplumL_pushimage(L, image);
        return 1;
    }
}

static int libplumL_image_load(lua_State *L) {
    return __libplumL_image_load(L, PLUM_MODE_BUFFER);
}

static int libplumL_image_loadfile(lua_State *L) {
    return __libplumL_image_load(L, PLUM_MODE_FILENAME);
}

static int __libplumL_image_store(lua_State *L, size_t mode) {
    struct plum_image *image = libplumL_checkimage(L, 1);
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
    struct plum_image *image = libplumL_checkimage(L, 1);
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
    lua_Integer frames = lua_gettop(L) >= 3 ? luaL_checkinteger(L, 3) : 1;
    if (width <= 0 || height <= 0 || frames <= 0 || width > UINT32_MAX || height > UINT32_MAX || frames > UINT32_MAX) {
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, plum_check_valid_image_size(width, height, frames));
    }
    return 1;
}

static int libplumL_convert_color(lua_State *L) {
    uint64_t color = luaL_checkinteger(L, 1);
    unsigned from = luaL_checkinteger(L, 2);
    unsigned to = luaL_checkinteger(L, 3);
    lua_pushinteger(L, plum_convert_color(color, from, to));
    return 1;
}

static int libplumL_image_remove_alpha(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    plum_remove_alpha(image);
    return 0;
}

static int libplumL_image_rotate(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    int count = lua_gettop(L) >= 2 ? luaL_checkinteger(L, 2) : 1;
    int flip = lua_gettop(L) >= 3 ? luaL_checkinteger(L, 3) : 0;
    plum_rotate_image(image, count, flip);
    return 0;
}

static int libplumL_image_reduce_palette(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    unsigned error = plum_reduce_palette(image);
    if (error) {
        lua_pushinteger(L, error);
        return 1;
    } else {
        return 0;
    }
}

static int libplumL_image_sort_palette(lua_State *L) {
    struct plum_image *image = libplumL_checkimage(L, 1);
    unsigned flags = __libplumL_or_flags(L, 2);
    unsigned error = plum_sort_palette(image, flags);
    if (error) {
        lua_pushinteger(L, error);
        return 1;
    } else {
        return 0;
    }
}

static const luaL_Reg plum_funcs[] = {
// struct plum_image * plum_new_image(void);
    { "copy", libplumL_image_copy },
    { "load", libplumL_image_load },
    { "loadfile", libplumL_image_loadfile },
// struct plum_image * plum_load_image_limited(const void * restrict buffer, size_t size_mode, unsigned flags, size_t limit, unsigned * restrict error);
    { "store", libplumL_image_store },
    { "storefile", libplumL_image_storefile },
    { "validate", libplumL_image_validate },
    { "error_text", libplumL_error_text },
    { "file_format_name", libplumL_file_format_name },
    { "version", libplumL_version },
    { "check_valid_image_size", libplumL_check_valid_image_size },
// int plum_check_limited_image_size(uint32_t width, uint32_t height, uint32_t frames, size_t limit);
// size_t plum_color_buffer_size(size_t size, unsigned flags);
// size_t plum_pixel_buffer_size(const struct plum_image * image);
// size_t plum_palette_buffer_size(const struct plum_image * image);
    { "rotate", libplumL_image_rotate },
// void plum_convert_colors(void * restrict destination, const void * restrict source, size_t count, unsigned to, unsigned from);
    { "convert_color", libplumL_convert_color },
// uint64_t plum_convert_color(uint64_t color, unsigned from, unsigned to);
    { "remove_alpha", libplumL_image_remove_alpha },
    { "sort_palette", libplumL_image_sort_palette },
// unsigned plum_sort_palette_custom(struct plum_image * image, uint64_t (* callback) (void *, uint64_t), void * argument, unsigned flags);
    { "reduce_palette", libplumL_image_reduce_palette },
// const uint8_t * plum_validate_palette_indexes(const struct plum_image * image);
// int plum_get_highest_palette_index(const struct plum_image * image);
// int plum_convert_colors_to_indexes(uint8_t * restrict destination, const void * restrict source, void * restrict palette, size_t count, unsigned flags);
// void plum_convert_indexes_to_colors(void * restrict destination, const uint8_t * restrict source, const void * restrict palette, size_t count, unsigned flags);
// void plum_sort_colors(const void * restrict colors, uint8_t max_index, unsigned flags, uint8_t * restrict result);
// void * plum_malloc(struct plum_image * image, size_t size);
// void * plum_calloc(struct plum_image * image, size_t size);
// void * plum_realloc(struct plum_image * image, void * buffer, size_t size);
// void plum_free(struct plum_image * image, void * buffer);
// struct plum_metadata * plum_allocate_metadata(struct plum_image * image, size_t size);
// unsigned plum_append_metadata(struct plum_image * image, int type, const void * data, size_t size);
// struct plum_metadata * plum_find_metadata(const struct plum_image * image, int type);
    { NULL, NULL }
};

static inline void __libplum_pushconst(lua_State *L, const char *name, int value) {
    lua_pushstring(L, name);
    lua_pushinteger(L, value);
    lua_settable(L, -3);
}
#define libplum_pushconst(L, c) __libplum_pushconst(L, (#c) + 5, c)

int luaopen_libplum(lua_State* L) {
    luaL_newlib(L, plum_funcs);

    libplum_pushconst(L, PLUM_COLOR_32);
    libplum_pushconst(L, PLUM_COLOR_64);
    libplum_pushconst(L, PLUM_COLOR_16);
    libplum_pushconst(L, PLUM_COLOR_32X);
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

    luaL_newmetatable(L, LIBPLUM_IMAGE);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_settable(L, -3);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, libplumL_image_destroy);
    lua_settable(L, -3);
    
    lua_pop(L, 1);

    return 1;
}
