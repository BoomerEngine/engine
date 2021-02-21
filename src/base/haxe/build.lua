-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_object")
Dependency("base_reflection")
Dependency("base_app")
Dependency("base_parser")
Dependency("base_config")
Dependency("base_resource")

FileOption("src/pcre/pcre_chartables.c", "nopch")
FileOption("src/pcre/pcre_compile.c", "nopch")
FileOption("src/pcre/pcre_dfa_exec.c", "nopch")
FileOption("src/pcre/pcre_exec.c", "nopch")
FileOption("src/pcre/pcre_fullinfo.c", "nopch")
FileOption("src/pcre/pcre_globals.c", "nopch")
FileOption("src/pcre/pcre_newline.c", "nopch")
FileOption("src/pcre/pcre_string_utils.c", "nopch")
FileOption("src/pcre/pcre_tables.c", "nopch")
FileOption("src/pcre/pcre_ucd.c", "nopch")
FileOption("src/pcre/pcre_xclass.c", "nopch")
FileOption("src/pcre/pcre16_ord2utf16.c", "nopch")
FileOption("src/pcre/pcre16_valid_utf16.c", "nopch")

FileOption("src/haxe/code.c", "nopch")
FileOption("src/haxe/debugger.c", "nopch")
FileOption("src/haxe/gc.c", "nopch")
FileOption("src/haxe/jit.c", "nopch")
FileOption("src/haxe/module.c", "nopch")
FileOption("src/haxe/profile.c", "nopch")

FileOption("src/haxe/std/array.c", "nopch")
FileOption("src/haxe/std/buffer.c", "nopch")
FileOption("src/haxe/std/bytes.c", "nopch")
FileOption("src/haxe/std/cast.c", "nopch")
FileOption("src/haxe/std/date.c", "nopch")
FileOption("src/haxe/std/debug.c", "nopch")
FileOption("src/haxe/std/error.c", "nopch")
FileOption("src/haxe/std/file.c", "nopch")
FileOption("src/haxe/std/fun.c", "nopch")
FileOption("src/haxe/std/maps.c", "nopch")
FileOption("src/haxe/std/math.c", "nopch")
FileOption("src/haxe/std/obj.c", "nopch")
FileOption("src/haxe/std/process.c", "nopch")
FileOption("src/haxe/std/random.c", "nopch")
FileOption("src/haxe/std/regexp.c", "nopch")
FileOption("src/haxe/std/socket.c", "nopch")
FileOption("src/haxe/std/string.c", "nopch")
FileOption("src/haxe/std/sys.c", "nopch")
FileOption("src/haxe/std/sys_android.c", "nopch")
FileOption("src/haxe/std/thread.c", "nopch")
FileOption("src/haxe/std/types.c", "nopch")
FileOption("src/haxe/std/ucs2.c", "nopch")
FileOption("src/haxe/std/track.c", "nopch")