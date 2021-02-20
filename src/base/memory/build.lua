-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")

FileOption("src/thirdparty/zlib/adler32.c", "nopch")
FileOption("src/thirdparty/zlib/crc32.c", "nopch")
FileOption("src/thirdparty/zlib/deflate.c", "nopch")
FileOption("src/thirdparty/zlib/inffast.c", "nopch")
FileOption("src/thirdparty/zlib/inflate.c", "nopch")
FileOption("src/thirdparty/zlib/inftrees.c", "nopch")
FileOption("src/thirdparty/zlib/trees.c", "nopch")
FileOption("src/thirdparty/zlib/uncompr.c", "nopch")
FileOption("src/thirdparty/zlib/compress.c", "nopch")
FileOption("src/thirdparty/zlib/zutil.c", "nopch")

FileOption("src/thirdparty/lz4/lz4.c", "nopch")
FileOption("src/thirdparty/lz4/lz4frame.c", "nopch")
FileOption("src/thirdparty/lz4/lz4hc.c", "nopch")
FileOption("src/thirdparty/lz4/xxhash.c", "nopch")
