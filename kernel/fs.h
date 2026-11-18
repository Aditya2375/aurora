#ifndef AURORA_FS_H
#define AURORA_FS_H
#include <stddef.h>
#define FS_NODES 48
#define FS_NAME 24
#define FS_DATA 1024
#define FS_PATH 256
enum { FS_OK=0, FS_NOT_FOUND=-1, FS_NOT_DIR=-2, FS_EXISTS=-3,
       FS_FULL=-4, FS_INVALID=-5, FS_READ_ONLY=-6, FS_NOT_EMPTY=-7,
       FS_IS_DIR=-8, FS_TOO_BIG=-9 };
struct fs_info { int id, parent, dir, readonly; size_t size; char name[FS_NAME]; };
void fs_init(void);
int fs_resolve(int cwd, const char *path);
int fs_stat(int id, struct fs_info *out);
int fs_create(int cwd, const char *path, int dir);
int fs_remove(int cwd, const char *path);
int fs_write(int cwd, const char *path, const char *text, int append);
int fs_read(int cwd, const char *path, const char **data, size_t *len);
int fs_path(int id, char *out, size_t cap);
int fs_used(void);
const char *fs_error(int code);
#endif
