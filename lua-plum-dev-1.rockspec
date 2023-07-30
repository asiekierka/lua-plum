package = "lua-plum"
version = "dev-1"
source = {
   url = "https://github.com/asiekierka/lua-plum"
}
description = {
   summary = "Lua bindings for the libplum image handling library.",
   homepage = "https://github.com/asiekierka/lua-plum",
   license = "Unlicense"
}
build = {
   type = "builtin",
   modules = {
      libplum = {"libplum/libplum.c", "lua-plum.c"}
   }
}
