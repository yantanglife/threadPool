#pragma once
#include <string>


#ifdef _WIN32
//#define FD_SETSIZE 1024
#include <WinSock2.h>   
#pragma comment (lib,"WS2_32")
#endif // WIN32

#ifdef _WIN32

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif    // PATH_MAX

struct dirent {
    long d_ino;              /* inode number*/
    off_t d_off;             /* offset to this dirent*/
    unsigned short d_reclen; /* length of this d_name*/
    unsigned char d_type;    /* the type of d_name*/
    char d_name[1];          /* file name (null-terminated)*/
};

typedef struct _dirdesc {
    int     dd_fd;      /** file descriptor associated with directory */
    long    dd_loc;     /** offset in current buffer */
    long    dd_size;    /** amount of data returned by getdirentries */
    char    *dd_buf;    /** data buffer */
    int     dd_len;     /** size of data buffer */
    long    dd_seek;    /** magic cookie returned by getdirentries */
    HANDLE handle;
    struct dirent *index;
} DIR;

# define __dirfd(dp)    ((dp)->dd_fd)

int mkdir(const char *path, int mode);
DIR *opendir(const char *);
int closedir(DIR *);
struct dirent *readdir(DIR *);

#endif    // _WIN32


class File {
public:
    static bool create_path(const char *file, unsigned int mod);
    static void delete_file(const char *path);
    static bool is_file(const char *path);
    static bool is_dir(const char *path);
    static bool is_special_dir(const char *path);
private:
    File();
    ~File();
};
