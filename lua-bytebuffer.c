//
// $id: bytebuffer.c codetypes $
//

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#define LUA_BYTEBUFFER "bytebuffer*"

static int l_new(lua_State *L);

static int l_len(lua_State *L)
{
    lua_pushinteger(L, lua_rawlen(L, 1));
    return 1;
}

static int l_tostring(lua_State *L)
{
    const char *self = (const char *)lua_touserdata(L, 1);
    size_t len = lua_rawlen(L, 1);
    lua_pushlstring(L, self, len);
    return 1;
}

static lua_Unsigned get_offset(lua_State *L, int idx)
{
    lua_Unsigned offset;
    lua_getuservalue(L, idx);
    offset = (lua_Unsigned)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return offset;
}

static void set_offset(lua_State *L, int idx, lua_Unsigned offset)
{
    idx = lua_absindex(L, idx);
    lua_pushinteger(L, offset);
    lua_setuservalue(L, idx);
}

static unsigned char *current_position(lua_State *L)
{
    lua_Unsigned offset = get_offset(L, 1);
    return (unsigned char *)lua_touserdata(L, 1) + offset;
}

static void check_and_advance_for_read(lua_State *L, size_t size)
{
    lua_Unsigned offset = get_offset(L, 1);
    size_t len = lua_rawlen(L, 1);
    if (len - offset < size) {
        luaL_error(L, "no more data");
    }
    set_offset(L, 1, offset + size);
}

static void check_and_advance_for_write(lua_State *L, size_t size)
{
    lua_Unsigned offset = get_offset(L, 1);
    size_t len = lua_rawlen(L, 1);
    if (len - offset < size) {
        luaL_error(L, "not enough space");
    }
    set_offset(L, 1, offset + size);
}

static uint64_t _read_value(lua_State *L, size_t num)
{
    uint64_t value = 0;
    unsigned char *curr = current_position(L);
    check_and_advance_for_read(L, num);
    
    if (num == 1) {
        value = (uint8_t)*curr++;
    } else if (num == 2) {
        value |= ((uint16_t)*curr++) << 8;
        value |= ((uint16_t)*curr++);
    } else if (num == 4) {
        value |= ((uint32_t)*curr++) << 24;
        value |= ((uint32_t)*curr++) << 16;
        value |= ((uint32_t)*curr++) << 8;
        value |= ((uint32_t)*curr++);
    } else if (num == 8) {
        value |= ((uint64_t)*curr++) << 56;
        value |= ((uint64_t)*curr++) << 48;
        value |= ((uint64_t)*curr++) << 40;
        value |= ((uint64_t)*curr++) << 32;
        value |= ((uint64_t)*curr++) << 24;
        value |= ((uint64_t)*curr++) << 16;
        value |= ((uint64_t)*curr++) << 8;
        value |= ((uint64_t)*curr++);
    }
    
    return value;
}

static void _write_value(lua_State *L, uint64_t value, size_t num)
{
    unsigned char *curr = current_position(L);
    check_and_advance_for_write(L, num);

    if (num == 1) {
        *curr++ = value & 0xff;
    } else if (num == 2) {
        *curr++ = value >> 8 & 0xff;
        *curr++ = value & 0xff;
    } else if (num == 4) {
        *curr++ = value >> 24 & 0xff;
        *curr++ = value >> 16 & 0xff;
        *curr++ = value >> 8 & 0xff;
        *curr++ = value & 0xff;
    } else if (num == 8) {
        *curr++ = value >> 56 & 0xff;
        *curr++ = value >> 48 & 0xff;
        *curr++ = value >> 40 & 0xff;
        *curr++ = value >> 32 & 0xff;
        *curr++ = value >> 24 & 0xff;
        *curr++ = value >> 16 & 0xff;
        *curr++ = value >> 8 & 0xff;
        *curr++ = value & 0xff;
    }
}

static int l_read_ubyte(lua_State *L)
{
    lua_pushnumber(L, (uint8_t)_read_value(L, 1));
    return 1;
}

static int l_write_ubyte(lua_State *L)
{
    _write_value(L, (uint8_t)luaL_checkinteger(L, 2), 1);
    return 0;
}

static int l_read_byte(lua_State *L)
{
    lua_pushnumber(L, (int8_t)(uint8_t)_read_value(L, 1));
    return 1;
}

static int l_write_byte(lua_State *L)
{
    _write_value(L, (uint8_t)(int8_t)luaL_checkinteger(L, 2), 1);
    return 0;
}

static int l_read_ushort(lua_State *L)
{
    lua_pushnumber(L, (uint16_t)_read_value(L, 2));
    return 1;
}

static int l_write_ushort(lua_State *L)
{
    _write_value(L, (uint16_t)luaL_checkinteger(L, 2), 2);
    return 0;
}

static int l_read_short(lua_State *L)
{
    lua_pushnumber(L, (int16_t)(uint16_t)_read_value(L, 2));
    return 1;
}

static int l_write_short(lua_State *L)
{
    _write_value(L, (uint16_t)(int16_t)luaL_checkinteger(L, 2), 2);
    return 0;
}

static int l_read_uint(lua_State *L)
{
    lua_pushnumber(L, (uint32_t)_read_value(L, 4));
    return 1;
}

static int l_write_uint(lua_State *L)
{
    _write_value(L, (uint32_t)luaL_checkinteger(L, 2), 4);
    return 0;
}

static int l_read_int(lua_State *L)
{
    lua_pushnumber(L, (int32_t)(uint32_t)_read_value(L, 4));
    return 1;
}

static int l_write_int(lua_State *L)
{
    _write_value(L, (uint32_t)(int32_t)luaL_checkinteger(L, 2), 4);
    return 1;
}

static int l_read_ulong(lua_State *L)
{
    lua_pushnumber(L, (uint64_t)_read_value(L, 8));
    return 1;
}

static int l_write_ulong(lua_State *L)
{
    _write_value(L, (uint64_t)luaL_checkinteger(L, 2), 8);
    return 0;
}

static int l_read_long(lua_State *L)
{
    lua_pushnumber(L, (int64_t)(uint64_t)_read_value(L, 8));
    return 1;
}

static int l_write_long(lua_State *L)
{
    _write_value(L, (uint64_t)(int64_t)luaL_checkinteger(L, 2), 8);
    return 0;
}

static int l_read_float(lua_State *L)
{
    union {
        uint32_t i;
        float f;
    } u;
    u.i = (uint32_t)_read_value(L, 4);
    lua_pushnumber(L, u.f);
    return 1;
}

static int l_write_float(lua_State *L)
{
    union {
        uint32_t i;
        float f;
    } u;
    u.f = (float)luaL_checknumber(L, 2);
    _write_value(L, u.i, 4);
    return 0;
}

static int l_read_double(lua_State *L)
{
    union {
        uint64_t i;
        double f;
    } u;
    u.i = (uint64_t)_read_value(L, 8);
    lua_pushnumber(L, u.f);
    return 1;
}

static int l_write_double(lua_State *L)
{
    union {
        uint64_t i;
        double f;
    } u;
    u.f = (double)luaL_checknumber(L, 2);
    _write_value(L, u.i, 8);
    return 0;
}

static int l_read_bytes(lua_State *L)
{
    void *dest;
    size_t len = (size_t)luaL_checkinteger(L, 2);
    const char *curr = (const char *)current_position(L);
    check_and_advance_for_read(L, len);
    lua_pushcfunction(L, l_new);
    lua_pushinteger(L, len);
    lua_call(L, 1, 1);
    dest = lua_touserdata(L, -1);
    memcpy(dest, (const void *)curr, len);
    return 1;
}

static int l_write_bytes(lua_State *L)
{
    void *curr = (void *)current_position(L);
    const char *src = (const char *)luaL_checkudata(L, 2, LUA_BYTEBUFFER);
    size_t len = (size_t)lua_rawlen(L, 2);
    check_and_advance_for_write(L, len);
    memcpy(curr, (const void *)src, len);
    return 0;
}

static int l_read_string(lua_State *L)
{
    int len = (int)luaL_checkinteger(L, 2);
    const char *curr = (const char *)current_position(L);
    check_and_advance_for_read(L, len);
    lua_pushlstring(L, curr, len);
    return 1;
}

static int l_write_string(lua_State *L)
{
    size_t len;
    void *curr = (void *)current_position(L);
    const char *src = luaL_checklstring(L, 2, &len);
    check_and_advance_for_write(L, len);
    memcpy(curr, (const void *)src, len);
    return 0;
}

static int l_offset(lua_State *L)
{
    lua_pushinteger(L, get_offset(L, 1));
    return 1;
}

static int l_mark_offset(lua_State *L)
{
    size_t len = lua_rawlen(L, 1);
    size_t offset = (size_t)luaL_checkinteger(L, 2);
    if (offset > len) {
        luaL_error(L, "invalid offset");
    }
    set_offset(L, 1, offset);
    return 0;
}

static int l_dump(lua_State *L)
{
    static char X[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    unsigned char *data = (unsigned char *)lua_touserdata(L, 1);
    size_t len = lua_rawlen(L, 1);
    
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    unsigned char *start = data;
    unsigned char *end = data + len;
    while (start < end) {
        uint8_t c = (uint8_t)*start++;
        luaL_addchar(&b, X[c >> 4 & 0xF]);
        luaL_addchar(&b, X[c & 0xF]);
        luaL_addchar(&b, ' ');
    }
    luaL_pushresult(&b);
    return 1;
}

int l_new(lua_State *L)
{
    const char *data = NULL;
    size_t len = 0;

    if (lua_type(L, 1) == LUA_TSTRING) {
        data = luaL_checklstring(L, 1, &len);
    } else if (lua_isuserdata(L, 1)) {
        data = (const char *)lua_touserdata(L, 1);
        len = lua_rawlen(L, 1);
    } else {
        len = (size_t)luaL_checkinteger(L, 1);
    }

    void *curr = lua_newuserdata(L, len);
    if (data) {
        memcpy(curr, (const void *)data, len);
    }

    lua_pushinteger(L, 0);
    lua_setuservalue(L, -2);

    luaL_setmetatable(L, LUA_BYTEBUFFER);
    
    return 1;
}

LUALIB_API int luaopen_bytebuffer(lua_State *L)
{
    luaL_checkversion(L);

    luaL_Reg l[] = {
        {"__len", l_len},
        {"__tostring", l_tostring},
        {"read_ubyte", l_read_ubyte},
        {"write_ubyte", l_write_ubyte},
        {"read_ushort", l_read_ushort},
        {"write_ushort", l_write_ushort},
        {"read_uint", l_read_uint},
        {"write_uint", l_write_uint},
        {"read_ulong", l_read_ulong},
        {"write_ulong", l_write_ulong},
        {"read_byte", l_read_byte},
        {"write_byte", l_write_byte},
        {"read_short", l_read_short},
        {"write_short", l_write_short},
        {"read_int", l_read_int},
        {"write_int", l_write_int},
        {"read_long", l_read_long},
        {"write_long", l_write_long},
        {"read_float", l_read_float},
        {"write_float", l_write_float},
        {"read_double", l_read_double},
        {"write_double", l_write_double},
        {"read_bytes", l_read_bytes},
        {"write_bytes", l_write_bytes},
        {"read_string", l_read_string},
        {"write_string", l_write_string},
        {"offset", l_offset},
        {"mark_offset", l_mark_offset},
        {"dump", l_dump},
        {"new", l_new},
        {NULL, NULL},
    };

    luaL_newmetatable(L, LUA_BYTEBUFFER);
    luaL_setfuncs(L, l, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    
    return 1;
}
