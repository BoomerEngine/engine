//* [# filter:thirdparty\zlib #]

#ifndef mangle_h
#define mangle_h

/*

This header file mangles all symbols exported from the zlib library.
It is included in all files while building the zlib library.  Due to
namespace pollution, no zlib headers should be included in .h files in
cm.

The following command was used to obtain the symbol list:

nm libcmzlib.so |grep " [TRD] "

This is the way to recreate the whole list:

nm libcmzlib.so |grep " [TRD] " | awk '{ print "#define "$3" "$3 }'

REMOVE the "_init" and "_fini" entries.

*/

#define adler32 adler32
#define adler32_combine adler32_combine
#define compress compress
#define compress2 compress2
#define compressBound compressBound
#define crc32 crc32
#define crc32_combine crc32_combine
#define get_crc_table get_crc_table
#define deflate deflate
#define deflateBound deflateBound
#define deflateCopy deflateCopy
#define deflateEnd deflateEnd
#define deflateInit2_ deflateInit2_
#define deflateInit_ deflateInit_
#define deflateParams deflateParams
#define deflatePrime deflatePrime
#define deflateReset deflateReset
#define deflateSetDictionary deflateSetDictionary
#define deflateSetHeader deflateSetHeader
#define deflateTune deflateTune
#define deflate_copyright deflate_copyright
#define gzclearerr gzclearerr
#define gzclose gzclose
#define gzdirect gzdirect
#define gzdopen gzdopen
#define gzeof gzeof
#define gzerror gzerror
#define gzflush gzflush
#define gzgetc gzgetc
#define gzgets gzgets
#define gzopen gzopen
#define gzprintf gzprintf
#define gzputc gzputc
#define gzputs gzputs
#define gzread gzread
#define gzrewind gzrewind
#define gzseek gzseek
#define gzsetparams gzsetparams
#define gztell gztell
#define gzungetc gzungetc
#define gzwrite gzwrite
#define inflate_fast inflate_fast
#define inflate inflate
#define inflateCopy inflateCopy
#define inflateEnd inflateEnd
#define inflateGetHeader inflateGetHeader
#define inflateInit2_ inflateInit2_
#define inflateInit_ inflateInit_
#define inflatePrime inflatePrime
#define inflateReset inflateReset
#define inflateSetDictionary inflateSetDictionary
#define inflateSync inflateSync
#define inflateSyncPoint inflateSyncPoint
#define inflate_copyright inflate_copyright
#define inflate_table inflate_table
#define _dist_code _dist_code
#define _length_code _length_code
#define _tr_align _tr_align
#define _tr_flush_block _tr_flush_block
#define _tr_init _tr_init
#define _tr_stored_block _tr_stored_block
#define _tr_tally _tr_tally
#define uncompress uncompress
#define zError zError
#define z_errmsg z_errmsg
#define zcalloc zcalloc
#define zcfree zcfree
#define zlibCompileFlags zlibCompileFlags
#define zlibVersion zlibVersion

#endif
