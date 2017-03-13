/*
 * ORACLE RMAN SBT API
 *
 * WARNING:
 * All interfaces are based on reverse engineering of 
 * RMAN SBT (System Backup to Tape). Since the SBT API 
 * is a big secret of Oracle, you have no way to obtain
 * it by free, so you use this at your own risk.
 *
 * Authors:
 *   Sunding Wei: swei@dingding.me
 *   Zhongkai He: zhe@dingding.me
 *
 * Dingding Technology, China
 * 2017.3.13
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#define SBTERROR_FATAL     7501 // fatal error, IO error etc
#define SBTERROR_NOTFOUND  7502 // file is not found
#define SBTERROR_EXIST     7503 // file already exists in catalog
#define SBTERROR_EOF       7504 // end of file
#define SBTERROR_NOTFOUND2 7090 // no such file
#define SBTERROR_7505      7505 // could not proxy backup file
#define SBTERROR_7012      7012 // file is already open
#define SBTERROR_NOTOPEN   7020 // no file is open
#define SBTERROR_CLOSE     7023 // error closing backup file

#define LOG(fmt, ...) \
    do {\
        if (ctx->fp) \
        fprintf(ctx->fp, "%s: " fmt, __FUNCTION__, __VA_ARGS__); } while (0)

struct sbtinfo {
    int type;
    void * value;
};

struct sbtopt {
    int type;
    void * value;
};

struct sbtdata {
    int status;
    int error;
};

struct sbtapi
{
    void * sbtbackup;
    void * sbtclose2;
    void * sbtcommand;
    void * sbtend;
    void * sbterror;
    void * sbtinfo2;
    void * sbtinit2;
    void * sbtread2;
    void * sbtremove2;
    void * sbtrestore;
    void * sbtwrite2;
    // Proxy copy 
    void * sbtpcbackup;
    void * sbtpccancel;
    void * sbtpccommit;
    void * sbtpcend;
    void * sbtpcquerybackup;
    void * sbtpcqueryrestore;
    void * sbtpcrestore;
    void * sbtpcstart;
    void * sbtpcstatus;
    void * sbtpcvalidate;
};

struct global_data_area {
    struct sbtopt options[10];
    struct sbtapi api;
    int api_version;
    int mm_version;
    int context_size;
    int proxy_limit;
    int proxy_copy_support;
    int maxsize;
};

struct sbtctx 
{
    int fd;
    int error;
    int block_size;
    int pid;
    int tid;
    char * file;
    char path[256];
    // Logger
    FILE * fp;
    // Related sbtinfo for backup or restore files
    struct sbtinfo * pairs;
};

struct global_data_area global = {};

int sbtopen(void)
{
    return (-1);
}

int sbtread(void)
{
    return (-1);
}

int sbtwrite(void)
{
    return (-1);
}

int sbtclose(void)
{
    return (-1);
}

int sbtinfo(void)
{
    return (-1);
}

int sbtremove(void)
{
    return (-1);
}

#define SBTOPT_END             1
#define SBTOPT_2               2
#define SBTOPT_MAXSIZE         3
#define SBTOPT_API_VERSION     4
#define SBTOPT_VENDOR          5   
#define SBTOPT_API             6
#define SBTOPT_CONTEXT_SIZE    7
#define SBTOPT_MM_VERSION      8
#define SBTOPT_PROXY_COPY      9
#define SBTOPT_PROXY_LIMIT     10

#define SBTINFO_NAME       1   // (value -> char *)  File name
#define SBTINFO_METHOD     2   // (value -> int*)    1 - stream, 2 - proxy copy
#define SBTINFO_CTIME      3   // (value -> time_t*) File creation date and time
#define SBTINFO_ETIME      4   // (value -> time_t*) File expiration date and time
#define SBTINFO_LABEL      5   // (value -> char *)  Media ID
#define SBTINFO_SHARE      6   // (value -> int*)    1 - Single user, 2 - Multiple concurrent users
#define SBTINFO_ORDER      7   // (value -> int*)    1 - Sequential, 2 - Random
#define SBTINFO_NOTFOUND   8   // (value -> NULL)    No such file
#define SBTINFO_COMMENT    9   // (value -> char*)   Comment
#define SBTINFO_END     9999   // (value -> NULL)    End marker

#define ROOT "/home/swei/libobk/data"

int catenate(char * buf, size_t n, const char * dir, const char * file)
{
    const char * p = file;

    // Remove leading slash
    while (*p == '/')
        ++p;

    return snprintf(buf, n, "%s/%s", dir, p);
}

int make_dir(char * buf, size_t n, const char * dir, const char * file)
{
    char * path = NULL;
    char * cursor = NULL;
    int ret = 0;

    if (catenate(buf, n, dir, file) < 0)
        return (-1);

    // mkdir
    path = strdup(buf);
    cursor = path + 1;
    while ((cursor = strchr(cursor, '/')) != NULL) {
        *cursor = '\0';
        if ((ret = mkdir(path, 0755)) == -1) {
            if (errno !=  EEXIST) {
                free(path);
                return (-1);
            }
            ret = 0;
        }
        *cursor++ = '/';
    }

    free(path);

    return (ret);
}

int sbtinit2(struct sbtctx *ctx, int flags, const struct sbtinfo *args)
{
    char buf[256];
    const struct sbtinfo * arg = args;

    ctx->pid = getpid();
    ctx->tid = pthread_self();

    snprintf(buf, sizeof(buf), "/tmp/log-%d-%x.txt", ctx->pid, ctx->tid);
    if ((ctx->fp = fopen(buf, "w")) == NULL)
        return (-1);

    LOG("pid %d, tid 0x%x\n", ctx->pid, ctx->tid);

    while (arg->type != SBTINFO_END) {
        if (arg->type == SBTINFO_NAME) {
            LOG("media %s\n", (const char *)arg->value);
        }
        LOG("type %d\n", arg->type);
        ++arg;
    }

    return (0);
}


int sbtbackup(struct sbtctx *ctx, 
        int flags, 
        const char *filename, 
        const struct sbtinfo *info, 
        int block_size, 
        int not_used, 
        int duplex)
{
    ctx->file = strdup(filename);
    ctx->block_size = block_size;
    if (make_dir(ctx->path, sizeof(ctx->path), ROOT, filename) == -1)
        return (-1);

    LOG("open %s\n", ctx->path);
    ctx->fd = open(ctx->path, O_CREAT | O_RDWR, 0644);
    if (ctx->fd == -1) {
        LOG("Error to open %s\n", ctx->path);
        return (-1);
    }

    LOG("Block size %d\n", block_size);
    LOG("not_used %d\n",  not_used);
    LOG("duplex %d\n", duplex);

    return (0);
}

int sbtclose2(struct sbtctx * ctx, char flags)
{
    LOG("Close %s\n", ctx->path);

    if (ctx->fd != -1) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    if (ctx->file) {
        free(ctx->file);
        ctx->file = NULL;
    }

    return (0);
}

int sbtcommand(struct sbtctx *ctx, int flags, const char *cmd)
{
    LOG("cmd = %s\n", cmd);
    return (0);
}

int sbtend(struct sbtctx *ctx, char flags)
{
    LOG("%s\n", "end");

    if (ctx->file) {
        free(ctx->file);
        ctx->file = NULL;
    }

    if (ctx->pairs) {
        // TODO: free values
        free(ctx->pairs);
        ctx->pairs = NULL;
    }

    if (ctx->fp) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }

    return (0);
}

int sbterror(struct sbtctx *ctx, 
        int flags, 
        int *error, 
        const char **error_native, 
        char **error_utf8
        )
{
    *error = ctx->error;
    LOG("error = %d\n", *error);
    *error_native = strerror(*error);
    *error_utf8 = "Error message in utf8";
    return (0);
}

int sbtinfo2(struct sbtctx *ctx, 
             char flags, 
             const char **files, 
             struct sbtinfo **output
             )
{
    char path[256];
    struct stat meta;
    const char ** p = files;
    const char * file = NULL;;
    int count = 0;
    int i = 0;
    int ret = 0;
    time_t * ts = NULL;
    int * v = NULL;

    while (*p++)
        ++count;

    // Allocate n sbtinfo groups,
    // each group starts with SBTINFO_NAME, 
    // SBTINFO_END must be set if no more data.
    LOG("files count %d\n", count);
    ctx->pairs = calloc(count * 9 + 1, sizeof(struct sbtinfo));
    ctx->error = 0;

    p = files;
    i = 0;
    while (*p) {
        file = *p++;
        catenate(path, sizeof(path), ROOT, file);
        LOG("%s\n", path);

        // Media information for backup file
        ctx->pairs[i].type = SBTINFO_NAME;
        ctx->pairs[i].value = strdup(file);
        ++i;

        if ((ret = stat(path, &meta)) == -1) {
            LOG("no such file %s\n", path);
            ctx->error = SBTERROR_NOTFOUND;
            ctx->pairs[i].type = SBTINFO_NOTFOUND;
            ctx->pairs[i].value = NULL;
            ++i;
            continue;
        }

        // Media sharing mode
        *(v = (int *)malloc(4)) = 1;
        ctx->pairs[i].type = SBTINFO_SHARE;
        ctx->pairs[i].value = v;
        ++i;

        // File ordering mode
        *(v = (int *)malloc(4)) = 1;
        ctx->pairs[i].type = SBTINFO_ORDER;
        ctx->pairs[i].value = v;
        ++i;

        // File creation method, stream
        *(v = (int *)malloc(4)) = 1;
        ctx->pairs[i].type = SBTINFO_METHOD;
        ctx->pairs[i].value = v;
        ++i;

        // Media ID
        ctx->pairs[i].type = SBTINFO_LABEL;
        ctx->pairs[i].value = strdup("Media ID");
        ++i;

        // File creation date and time
        *(ts = (time_t *)malloc(sizeof(time_t))) = meta.st_ctime;
        ctx->pairs[i].type = SBTINFO_CTIME;
        ctx->pairs[i].value = ts;
        ++i;

#if 0
        // File expiration date and time
        *(ts = (time_t *)malloc(sizeof(time_t))) = meta.st_ctime + 315360000;
        ctx->pairs[i].type = SBTINFO_ETIME;
        ctx->pairs[i].value = ts;
        ++i;
#endif

        // Comment
        ctx->pairs[i].type = SBTINFO_COMMENT;
        ctx->pairs[i].value = strdup("Comment of the file");
        ++i;
    }

    // END marker
    ctx->pairs[i].type = SBTINFO_END;
    ctx->pairs[i].value = NULL;
    *output = ctx->pairs;

    return (0);
}

// Read until eof
int sbtread2(struct sbtctx *ctx, int flags, void *buf)
{
    int ret = read(ctx->fd, buf, ctx->block_size);

    if (ret > 0) 
        return (0);

    ctx->error = 0;
    if (ret == 0)
        ctx->error = SBTERROR_EOF;

    return (-1);
}

int sbtwrite2(struct sbtctx *ctx, int flags, void *buf)
{
    int ret = write(ctx->fd, buf, ctx->block_size);

    if (ret == -1)
        return (-1);

    return (0);
}

int sbtremove2(struct sbtctx *ctx, int flags, const char **files)
{
    const char ** file = files;
    char buf[256];
    int ret;

    while (*file) {
        catenate(buf, sizeof(buf), ROOT, *file);
        ret = remove(buf);
        LOG("remove %s, ret %d\n", buf, ret);
        ++file;
    }

    return (0);
}

int sbtrestore(struct sbtctx *ctx, int flags, char *filename, int block_size)
{

    ctx->block_size = block_size;
    ctx->file = strdup(filename);

    if (catenate(ctx->path, sizeof(ctx->path), ROOT, filename) == -1)
        return (-1);

    LOG("Restore %s\n", ctx->path);
    if ((ctx->fd = open(ctx->path, O_RDONLY)) == -1) {
        LOG("Error (%d) to open %s\n", errno, ctx->path);
        return (-1);
    }

    return (0);
}

int sbtinit(struct sbtdata * data, 
            const struct sbtinfo * input, 
            struct sbtopt **output
            )
{
    int i = 0;
    int ret = 0;

    data->status = 0;
    data->error = 0;

    // Setup SBT version 2 api
    global.api.sbtbackup = sbtbackup;
    global.api.sbtclose2 = sbtclose2;
    global.api.sbtcommand = sbtcommand;
    global.api.sbtend = sbtend;
    global.api.sbterror = sbterror;
    global.api.sbtinfo2 = sbtinfo2;
    global.api.sbtinit2 = sbtinit2;
    global.api.sbtread2 = sbtread2;
    global.api.sbtremove2 = sbtremove2;
    global.api.sbtrestore = sbtrestore;
    global.api.sbtwrite2 = sbtwrite2;

    global.api_version = 0x0200;
    global.options[i].type = SBTOPT_API_VERSION;
    global.options[i].value = &global.api_version;
    ++i;

    global.options[i].type = SBTOPT_VENDOR;
    global.options[i].value = "SBT by Dingding Technology";
    ++i;

    global.options[i].type = SBTOPT_API;
    global.options[i].value = &global.api;
    ++i;

    global.context_size = sizeof(struct sbtctx);
    global.options[i].type = SBTOPT_CONTEXT_SIZE;
    global.options[i].value = &global.context_size;
    ++i;

    // Media manager version
    global.mm_version = 0x01020304;
    global.options[i].type = SBTOPT_MM_VERSION;
    global.options[i].value = &global.mm_version;
    ++i;

#if 0
    // Maximum backup file size
    global.maxsize = 1024*1024;
    global.options[i].type = SBTOPT_MAXSIZE;
    global.options[i].value = &global.maxsize;
    ++i;

    // Proxy copy
    global.options[i].type = 9;
    global.options[i].value = NULL;
    ++i;

    // Maximum concurrent proxy copy files
    global.proxy_limit = 0;
    global.options[i].type = SBTOPT_PROXY_LIMIT;
    global.options[i].value = &global.proxy_limit;
    ++i;
#endif

    // End
    global.options[i].type = SBTOPT_END;
    global.options[i].value = NULL;
    ++i;

    *output = global.options;

    return (ret);
}

