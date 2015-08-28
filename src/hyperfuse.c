#ifndef __APPLE__
#define _POSIX_C_SOURCE 199309 // to fix fuse c99 issues
#define _GNU_SOURCE
#endif
#define FUSE_USE_VERSION 29

#define HYPERFUSE_INIT 0
#define HYPERFUSE_GETATTR 1
#define HYPERFUSE_READDIR 2
#define HYPERFUSE_READ 3
#define HYPERFUSE_OPEN 4
#define HYPERFUSE_TRUNCATE 5
#define HYPERFUSE_CREATE 6
#define HYPERFUSE_UNLINK 7
#define HYPERFUSE_WRITE 8
#define HYPERFUSE_CHMOD 9
#define HYPERFUSE_CHOWN 10
#define HYPERFUSE_RELEASE 11
#define HYPERFUSE_MKDIR 12
#define HYPERFUSE_RMDIR 13
#define HYPERFUSE_UTIMENS 14
#define HYPERFUSE_RENAME 15
#define HYPERFUSE_SYMLINK 16
#define HYPERFUSE_READLINK 17
#define HYPERFUSE_LINK 18
#define HYPERFUSE_ACCESS 19
#define HYPERFUSE_STATFS 20
#define HYPERFUSE_FGETATTR 21
#define HYPERFUSE_FLUSH 22
#define HYPERFUSE_FSYNC 23
#define HYPERFUSE_FSYNCDIR 24
#define HYPERFUSE_FTRUNCATE 25
#define HYPERFUSE_MKNOD 26
#define HYPERFUSE_SETXATTR 27
#define HYPERFUSE_GETXATTR 28
#define HYPERFUSE_OPENDIR 29
#define HYPERFUSE_RELEASEDIR 30

#define WITH_PATH(path, len) \
  uint16_t path_len = strlen(path); \
  uint32_t buf_len = 7 + 2 + path_len + 1 + len; \
  char buf[buf_len]; \
  char *buf_offset = (char *) &buf + 7; \
  buf_offset = write_string(buf_offset, (char *) path, path_len);

#include <fuse.h>
#include <fuse_opt.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <string.h>

#include "socket.h"
#include "enc.h"
#include "id_map.h"

#include <unistd.h>

static int rpc_fd_out;
static int rpc_fd_in;
static id_map_t ids;
static char* mnt;
static struct stat mnt_st;

typedef struct {
  uint8_t method;
  void *result;
  char *buffer;
  uint32_t buffer_length;
  struct fuse_file_info *info;
  fuse_fill_dir_t filler; // for readdir
} rpc_t;

static int socket_error () {
  fprintf(stderr, "Connection error. Exiting...\n");
  exit(-3);
  return -1;
}

inline static void rpc_parse_getattr (rpc_t *req, char *frame, uint32_t frame_len) {
  uint32_t val_32;
  struct stat *st = (struct stat *) req->result;
  frame = read_uint32(frame, &val_32);
  st->st_dev = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_mode = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_nlink = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_uid = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_gid = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_rdev = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_blksize = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_ino = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_size = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_blocks = val_32;
#ifdef __APPLE__
  frame = read_uint32(frame, &val_32);
  st->st_atimespec.tv_sec = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_mtimespec.tv_sec = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_ctimespec.tv_sec = val_32;
#else
  frame = read_uint32(frame, &val_32);
  st->st_atime = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_mtime = val_32;
  frame = read_uint32(frame, &val_32);
  st->st_ctime = val_32;
#endif
}

inline static void rpc_parse_readlink (rpc_t *req, char *frame, uint32_t frame_len) {
  uint16_t str_len;
  char *str;
  read_string(frame, &str, &str_len);
  memcpy(req->result, str, str_len + 1);
}

inline static void rpc_parse_readdir (rpc_t *req, char *frame, uint32_t frame_len) {
  uint16_t str_len;
  char *str;
  char *offset = frame;

  while (frame - offset < frame_len) {
    frame = read_string(frame, &str, &str_len);
    req->filler(req->result, str, NULL, 0);
  }
}

inline static void rpc_parse_fd (rpc_t *req, char *frame, uint32_t frame_len) {
  uint16_t fd;
  read_uint16(frame, &fd);
  req->info->fh = fd;
}

inline static int rpc_request (rpc_t *req) {
  char *tmp = req->buffer;
  uint16_t send_id = id_map_alloc(&ids, req);

  // write header
  tmp = write_uint32(tmp, req->buffer_length - 4);
  tmp = write_uint16(tmp, send_id);
  tmp = write_uint8(tmp, req->method);

  // write request
  if (socket_write(rpc_fd_out, req->buffer, req->buffer_length) < 0) return socket_error();

  // read a response
  char header[10];
  tmp = (char *) &header;
  if (socket_read(rpc_fd_in, tmp, 10) < 0) return socket_error();

  uint32_t frame_size;
  uint16_t recv_id;
  int32_t ret;

  tmp = read_uint32(tmp, &frame_size);
  tmp = read_uint16(tmp, &recv_id);
  tmp = read_int32(tmp, &ret);

  // fprintf(stderr, "frame_size is %u, recv_id is %u, return value is %u\n", frame_size, recv_id, ret);

  id_map_free(&ids, send_id);

  frame_size -= 6;

  switch (req->method) {
    case HYPERFUSE_READ: {
      if (frame_size) {
        if (socket_read(rpc_fd_in, req->result, frame_size) < 0) return socket_error();
      }
      return ret;
    }
    case HYPERFUSE_SYMLINK:
    case HYPERFUSE_RENAME:
    case HYPERFUSE_UTIMENS:
    case HYPERFUSE_RMDIR:
    case HYPERFUSE_MKDIR:
    case HYPERFUSE_RELEASE:
    case HYPERFUSE_CHOWN:
    case HYPERFUSE_CHMOD:
    case HYPERFUSE_WRITE:
    case HYPERFUSE_UNLINK:
    case HYPERFUSE_TRUNCATE: {
      return ret;
    }
  }

  char rem[frame_size];
  tmp = (char *) &rem;
  if (socket_read(rpc_fd_in, tmp, frame_size) < 0) return socket_error();

  if (ret < 0) return ret;

  switch (req->method) {
    case HYPERFUSE_GETATTR: {
      rpc_parse_getattr(req, tmp, frame_size);
      break;
    }

    case HYPERFUSE_READDIR: {
      rpc_parse_readdir(req, tmp, frame_size);
      break;
    }

    case HYPERFUSE_READLINK: {
      rpc_parse_readlink(req, tmp, frame_size);
      break;
    }

    case HYPERFUSE_CREATE:
    case HYPERFUSE_OPEN: {
      rpc_parse_fd(req, tmp, frame_size);
    }
  }

  return 0;
}

static void* hyperfuse_init (struct fuse_conn_info *conn) {
  WITH_PATH(mnt, 0);

  rpc_t req = {
    .method = HYPERFUSE_INIT,
    .buffer = buf,
    .buffer_length = buf_len
  };

  rpc_request(&req);
  return NULL;
}

static int hyperfuse_getattr (const char *path, struct stat *st) {
  if (!strcmp(path, "/")) {
    memcpy(st, &mnt_st, sizeof(struct stat));
    return 0;
  }

  WITH_PATH(path, 0);

  rpc_t req = {
    .method = HYPERFUSE_GETATTR,
    .result = st,
    .buffer = buf,
    .buffer_length = buf_len
  };

  return rpc_request(&req);
}

static int hyperfuse_readdir (const char *path, void *fuse_buf, fuse_fill_dir_t filler, off_t pos, struct fuse_file_info *info) {
  WITH_PATH(path, 0);

  rpc_t req = {
    .method = HYPERFUSE_READDIR,
    .result = fuse_buf,
    .buffer = buf,
    .buffer_length = buf_len,
    .filler = filler
  };

  return rpc_request(&req);
}

static int hyperfuse_open (const char *path, struct fuse_file_info *info) {
  WITH_PATH(path, 2);

  rpc_t req = {
    .method = HYPERFUSE_OPEN,
    .buffer = buf,
    .buffer_length = buf_len,
    .info = info
  };

  buf_offset = write_uint16(buf_offset, info->flags);
  return rpc_request(&req);
}

static int hyperfuse_truncate (const char *path, off_t size) {
  WITH_PATH(path, 4);

  rpc_t req = {
    .method = HYPERFUSE_TRUNCATE,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint32(buf_offset, size);
  return rpc_request(&req);
}

static int hyperfuse_write (const char *path, const char *fuse_buf, size_t len, off_t pos, struct fuse_file_info *info) {
  WITH_PATH(path, 2 + 4 + len);

  rpc_t req = {
    .method = HYPERFUSE_WRITE,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, info->fh);
  buf_offset = write_uint32(buf_offset, pos);
  buf_offset = write_buffer(buf_offset, (char *) fuse_buf, len);
  return rpc_request(&req);
}

static int hyperfuse_read (const char *path, char *fuse_buf, size_t len, off_t pos, struct fuse_file_info *info) {
  WITH_PATH(path, 2 + 4 + len);

  rpc_t req = {
    .method = HYPERFUSE_READ,
    .result = fuse_buf,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, info->fh);
  buf_offset = write_uint32(buf_offset, len);
  buf_offset = write_uint32(buf_offset, pos);
  return rpc_request(&req);
}

static int hyperfuse_create (const char *path, mode_t mode, struct fuse_file_info *info) {
  WITH_PATH(path, 2);

  rpc_t req = {
    .method = HYPERFUSE_CREATE,
    .buffer = buf,
    .buffer_length = buf_len,
    .info = info
  };

  buf_offset = write_uint16(buf_offset, mode);
  return rpc_request(&req);
}

static int hyperfuse_unlink (const char *path) {
  WITH_PATH(path, 0);

  rpc_t req = {
    .method = HYPERFUSE_UNLINK,
    .buffer = buf,
    .buffer_length = buf_len
  };

  return rpc_request(&req);
}

static int hyperfuse_chmod (const char *path, mode_t mode) {
  WITH_PATH(path, 2);

  rpc_t req = {
    .method = HYPERFUSE_CHMOD,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, mode);
  return rpc_request(&req);
}

static int hyperfuse_chown (const char *path, uid_t uid, gid_t gid) {
  WITH_PATH(path, 2 + 2);

  rpc_t req = {
    .method = HYPERFUSE_CHOWN,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, uid);
  buf_offset = write_uint16(buf_offset, gid);
  return rpc_request(&req);
}

static int hyperfuse_release (const char *path, struct fuse_file_info *info) {
  WITH_PATH(path, 2);

  rpc_t req = {
    .method = HYPERFUSE_RELEASE,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, info->fh);
  return rpc_request(&req);
}

static int hyperfuse_mkdir (const char *path, mode_t mode) {
  WITH_PATH(path, 2);

  rpc_t req = {
    .method = HYPERFUSE_MKDIR,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_uint16(buf_offset, mode);
  return rpc_request(&req);
}

static int hyperfuse_rmdir (const char *path) {
  WITH_PATH(path, 0);

  rpc_t req = {
    .method = HYPERFUSE_RMDIR,
    .buffer = buf,
    .buffer_length = buf_len
  };

  return rpc_request(&req);
}

static uint32_t get_secs (const struct timespec *tv) {
  return tv->tv_sec;
}

static int hyperfuse_utimens (const char *path, const struct timespec tv[2]) {
  WITH_PATH(path, 4 + 4);

  rpc_t req = {
    .method = HYPERFUSE_UTIMENS,
    .buffer = buf,
    .buffer_length = buf_len
  };

  struct timespec *tv_ptr = (struct timespec *) tv;
  buf_offset = write_uint32(buf_offset, get_secs(tv_ptr));
  buf_offset = write_uint32(buf_offset, get_secs(tv_ptr + 1));
  return rpc_request(&req);
}

static int hyperfuse_rename (const char *path, const char *dst) {
  uint16_t dst_len = strlen(dst);
  WITH_PATH(path, 2 + dst_len + 1);

  rpc_t req = {
    .method = HYPERFUSE_RENAME,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_string(buf_offset, (char *) dst, dst_len);
  return rpc_request(&req);
}

static int hyperfuse_symlink (const char *path, const char *link) {
  uint16_t link_len = strlen(link);
  WITH_PATH(path, 2 + link_len + 1);

  rpc_t req = {
    .method = HYPERFUSE_SYMLINK,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_string(buf_offset, (char *) link, link_len);
  return rpc_request(&req);
}

static int hyperfuse_readlink (const char *path, char *fuse_buf, size_t len) {
  WITH_PATH(path, 0);

  rpc_t req = {
    .method = HYPERFUSE_READLINK,
    .result = fuse_buf,
    .buffer = buf,
    .buffer_length = buf_len
  };

  return rpc_request(&req);
}

static int hyperfuse_link (const char *path, const char *link) {
  uint16_t link_len = strlen(link);
  WITH_PATH(path, 2 + link_len + 1);

  rpc_t req = {
    .method = HYPERFUSE_LINK,
    .buffer = buf,
    .buffer_length = buf_len
  };

  buf_offset = write_string(buf_offset, (char *) link, link_len);
  return rpc_request(&req);
}


static int connect (char *addr) {
  if (!strcmp(addr, "-")) {
    rpc_fd_in = 0;
    rpc_fd_out = 1;
    return 0;
  }

  int len = strlen(addr);
  int colon = len;
  for (int i = 0; i < len; i++) {
    if (*(addr + i) == ':') colon = i;
  }

  *(addr + colon) = '\0';
  int port = colon < len ? atoi(addr + colon + 1) : 10000;
  rpc_fd_in = rpc_fd_out = socket_connect(port, strlen(addr) ? addr : NULL);
  return rpc_fd_in;
}

static int bitfield_get (uint8_t *bitfield, int index) {
  int b = index / 8;
  int mask = 1 << (7 - (index - b * 8));
  return *(bitfield + b) & mask;
}

int main (int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: hyperfuse [mountpoint] [host:port]\n");
    exit(1);
  }

#ifdef __APPLE__
  unmount(argv[1], 0);
#else
  umount(argv[1]);
#endif

  mnt = realpath(argv[1], mnt);
  char *addr = argv[2];

  if (mnt == NULL || stat(mnt, &mnt_st) < 0) {
    fprintf(stderr, "Mountpoint does not exist\n");
    return -1;
  }

  if (connect(addr) < 0) {
    fprintf(stderr, "Could not connect to server\n");
    return -2;
  }

  id_map_init(&ids);

  uint8_t methods[5];
  if (socket_read(rpc_fd_in, (char *) &methods, 5) < 0) return socket_error();

  struct fuse_operations ops = {
    .init = bitfield_get(methods, HYPERFUSE_INIT) ? hyperfuse_init : NULL,
    .readdir = bitfield_get(methods, HYPERFUSE_READDIR) ? hyperfuse_readdir : NULL,
    .getattr = bitfield_get(methods, HYPERFUSE_GETATTR) ? hyperfuse_getattr : NULL,
    .read = bitfield_get(methods, HYPERFUSE_READ) ? hyperfuse_read : NULL,
    .open = bitfield_get(methods, HYPERFUSE_OPEN) ? hyperfuse_open : NULL,
    .truncate = bitfield_get(methods, HYPERFUSE_TRUNCATE) ? hyperfuse_truncate : NULL,
    .create = bitfield_get(methods, HYPERFUSE_CREATE) ? hyperfuse_create : NULL,
    .unlink = bitfield_get(methods, HYPERFUSE_UNLINK) ? hyperfuse_unlink : NULL,
    .write = bitfield_get(methods, HYPERFUSE_WRITE) ? hyperfuse_write : NULL,
    .chmod = bitfield_get(methods, HYPERFUSE_CHMOD) ? hyperfuse_chmod : NULL,
    .chown = bitfield_get(methods, HYPERFUSE_CHOWN) ? hyperfuse_chown : NULL,
    .release = bitfield_get(methods, HYPERFUSE_RELEASE) ? hyperfuse_release : NULL,
    .mkdir = bitfield_get(methods, HYPERFUSE_MKDIR) ? hyperfuse_mkdir : NULL,
    .rmdir = bitfield_get(methods, HYPERFUSE_RMDIR) ? hyperfuse_rmdir : NULL,
    .utimens = bitfield_get(methods, HYPERFUSE_UTIMENS) ? hyperfuse_utimens : NULL,
    .rename = bitfield_get(methods, HYPERFUSE_RENAME) ? hyperfuse_rename : NULL,
    .symlink = bitfield_get(methods, HYPERFUSE_SYMLINK) ? hyperfuse_symlink : NULL,
    .readlink = bitfield_get(methods, HYPERFUSE_READLINK) ? hyperfuse_readlink : NULL,
    .link = bitfield_get(methods, HYPERFUSE_LINK) ? hyperfuse_link : NULL
  };

  struct fuse_args args = FUSE_ARGS_INIT(argc - 2, argv + 2);
  struct fuse_chan *ch = fuse_mount(mnt, &args);

  if (ch == NULL) {
    fprintf(stderr, "Could not mount fuse\n");
    return -3;
  }

  struct fuse *fuse = fuse_new(ch, &args, &ops, sizeof(struct fuse_operations), NULL);

  if (fuse == NULL) {
    fprintf(stderr, "Could not instantiate fuse\n");
    return -4;
  }

  fuse_loop(fuse);
  fuse_unmount(mnt, ch);
  fuse_session_remove_chan(ch);
  fuse_destroy(fuse);

  fprintf(stderr, "KTHXBYE\n");

  return 0;
}
