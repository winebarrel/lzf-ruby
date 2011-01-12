#ifdef _WIN32
__declspec(dllexport) void Init_lzfruby(void);
typedef int ssize_t;
#define _CRT_SECURE_DEPRECATE_MEMORY
#include <memory.h>
#endif

#include <string.h>
#include "lzf.h"
#include "ruby.h"

#ifndef RSTRING_PTR
#define RSTRING_PTR(s) (RSTRING(s)->ptr)
#endif

#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#define Check_IO(x) do { \
  const char *classname = rb_class2name(CLASS_OF(x)); \
  if (rb_obj_is_instance_of((x), rb_cIO)) { \
    rb_io_binmode(x); \
  } else if (strcmp(classname, "StringIO") != 0) { \
    rb_raise(rb_eTypeError, "wrong argument type %s (expected IO or StringIO)", classname); \
  } \
} while(0)

#define VERSION "0.1.3"
#define BLOCKSIZE (1024 * 64 - 1)
#define MAX_BLOCKSIZE BLOCKSIZE
#define TYPE0_HDR_SIZE 5
#define TYPE1_HDR_SIZE 7
#define MAX_HDR_SIZE 7
#define MIN_HDR_SIZE 5

static VALUE LZF;
static VALUE LZF_Error;

/* */
static VALUE lzfruby_compress(int argc, VALUE *argv, VALUE self) {
  VALUE from, to, blocksize;
  unsigned char buf1[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  unsigned char buf2[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  unsigned char *header;
  int i_blocksize;

  rb_scan_args(argc, argv, "21", &from, &to, &blocksize);
  Check_IO(from);
  Check_IO(to);

  if (NIL_P(blocksize)) {
    i_blocksize = BLOCKSIZE;
  } else {
    i_blocksize = NUM2INT(blocksize);

    if (blocksize < 1 || MAX_BLOCKSIZE < blocksize) {
      i_blocksize = BLOCKSIZE;
    }
  }

  blocksize = INT2FIX(i_blocksize);

  while (1) {
    VALUE in = rb_funcall(from, rb_intern("read"), 1, blocksize);
    ssize_t us, cs, len;

    if (NIL_P(in) || (us = RSTRING_LEN(in)) < 1) {
      break;
    }

    memcpy(&buf1[MAX_HDR_SIZE], RSTRING_PTR(in), us);
    cs = lzf_compress(&buf1[MAX_HDR_SIZE], us, &buf2[MAX_HDR_SIZE], (us > 4) ? us - 4 : us);

    if (cs) {
      header = &buf2[MAX_HDR_SIZE - TYPE1_HDR_SIZE];
      header[0] = 'Z';
      header[1] = 'V';
      header[2] = 1;
      header[3] = cs >> 8;
      header[4] = cs & 0xff;
      header[5] = us >> 8;
      header[6] = us & 0xff;
      len = cs + TYPE1_HDR_SIZE;
    } else {
      header = &buf1[MAX_HDR_SIZE - TYPE0_HDR_SIZE];
      header[0] = 'Z';
      header[1] = 'V';
      header[2] = 0;
      header[3] = us >> 8;
      header[4] = us & 0xff;
      len = us + TYPE0_HDR_SIZE;
    }

    rb_funcall(to, rb_intern("write"), 1, rb_str_new(header, len));
  }

  return Qnil;
}

/* */
static VALUE lzfruby_decompress(VALUE self, VALUE from, VALUE to) {
  unsigned char header[MAX_HDR_SIZE];
  unsigned char buf1[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  unsigned char buf2[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  ssize_t rc, cs, us, bytes, over = 0;
  int l, rd;

  Check_IO(from);
  Check_IO(to);

  while (1) {
    VALUE in, in_header;
    unsigned char *p;

    in_header = rb_funcall(from, rb_intern("read"), 1, INT2FIX(MAX_HDR_SIZE - over));

    if (NIL_P(in_header) || (rc = RSTRING_LEN(in_header)) < 1) {
      break;
    }

    memcpy(header + over, RSTRING_PTR(in_header), MAX_HDR_SIZE - over);
    rc += over;
    over = 0;

    if (header[0] == 0) {
      break;
    }

    if (rc < MIN_HDR_SIZE || header[0] != 'Z' || header[1] != 'V') {
      rb_raise(LZF_Error, "invalid data stream - magic not found or short header");
    }

    switch (header[2]) {
    case 0:
      cs = -1;
      us = (header[3] << 8) | header[4];
      p = &header[TYPE0_HDR_SIZE];
      break;

    case 1:
      if (rc < TYPE1_HDR_SIZE) {
        rb_raise(LZF_Error, "short data");
      }

      cs = (header[3] << 8) | header[4];
      us = (header[5] << 8) | header[6];
      p = &header[TYPE1_HDR_SIZE];
      break;

    default:
      rb_raise(LZF_Error, "unknown blocktype");
    }

    bytes = (cs == -1) ? us : cs;
    l = &header[rc] - p;

    if (l > 0) {
      memcpy(buf1, p, l);
    }

    if (l > bytes) {
      over = l - bytes;
      memmove(header, &p[bytes], over);
    }

    p = &buf1[l];
    rd = bytes - l;

    if (rd > 0) {
      in = rb_funcall(from, rb_intern("read"), 1, INT2FIX(rd));

      if (NIL_P(in) || (rc = RSTRING_LEN(in)) < 1 || rc != rd) {
        rb_raise(LZF_Error, "short data");
      }

      memcpy(p, RSTRING_PTR(in), rc);
    }

    if (cs == -1) {
      rb_funcall(to, rb_intern("write"), 1, rb_str_new(buf1, us)); 
    } else {
      if (lzf_decompress(buf1, cs, buf2, us) != us) {
        rb_raise(LZF_Error, "decompress: invalid stream - data corrupted");
      }

      rb_funcall(to, rb_intern("write"), 1, rb_str_new(buf2, us));
    }
  }

  return Qnil;
}

void Init_lzfruby() {
  LZF = rb_define_module("LZF");
  LZF_Error = rb_define_class_under(LZF, "Error", rb_eStandardError);

  rb_define_const(LZF, "VERSION", rb_str_new2(VERSION));
  rb_define_module_function(LZF, "compress", lzfruby_compress, -1);
  rb_define_module_function(LZF, "decompress", lzfruby_decompress, 2);
}
