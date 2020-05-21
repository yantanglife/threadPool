#include "file.h"
#include "logger.h"

#ifdef _WIN32
#include <io.h>   
#include <direct.h>
#pragma warning(disable:4996) 
#else
#include <dirent.h>
#endif // _WIN32


#ifndef _WIN32
#define	_unlink	unlink
#define	_rmdir	rmdir
#define	_access	access
#endif // !_WIN32

#ifdef _WIN32

int mkdir(const char *path, int mode) {
	return _mkdir(path);
}

DIR *opendir(const char *name) {
	char nameBuff[512];
	sprintf(nameBuff, "%s\\*.*", name);
	WIN32_FIND_DATAA FindData;
	auto hFind = FindFirstFileA(nameBuff, &FindData);
	if (hFind == INVALID_HANDLE_VALUE) {
		WarnL << "FindFirstFileA failed: " << "error."; //get_uv_errmsg();
		return nullptr;
	}
	DIR *dir = (DIR*)malloc(sizeof(DIR));
	memset(dir, 0, sizeof(DIR));
	dir->dd_fd = 0;
	dir->handle = hFind;
	return dir;
}

int closedir(DIR *dir) {
	if (!dir) {
		return -1;
	}
	if (dir->handle != INVALID_HANDLE_VALUE) {
		FindClose(dir->handle);
		dir->handle = INVALID_HANDLE_VALUE;
	}
	if (dir->index) {
		free(dir->index);
		dir->index = nullptr;
	}
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *d) {
	HANDLE hFind = d->handle;
	WIN32_FIND_DATAA FileData;
	// fail or end.
	if (!FindNextFileA(hFind, &FileData)) {
		return nullptr;
	}
	struct dirent *dir = (struct dirent *)malloc(sizeof(struct dirent) + sizeof(FileData.cFileName));
	strcpy(dir->d_name, (char *)FileData.cFileName);
	dir->d_reclen = strlen(dir->d_name);
	// check there is file or directory.
	if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		dir->d_type = 2;
	}
	else {
		dir->d_type = 1;
	}
	if (d->index) {
		free(d->index);
		d->index = nullptr;
	}
	d->index = dir;
	return dir;
}

#endif // _WIN32

void
get_file_path(const char *path, const char *file_name, char *file_path) {
	strcpy(file_path, path);
	if (file_path[strlen(file_path) - 1] != '/') {
		strcat(file_path, "/");
	}
	strcat(file_path, file_name);
}

// File

bool
File::create_path(const char *file, unsigned int mod) {
	
	std::string path = file;
	std::string dir;
	int index = 1;
	while (1) {
		index = path.find('/', index) + 1;
		dir = path.substr(0, index);
		if (dir.size() == 0) {
			break;
		}
		if (_access(dir.c_str(), 0) == -1) {
			if (mkdir(dir.c_str(), mod) == -1) {
				WarnL << dir << ": " << "mkdir error."; //get_uv_errmsg();
				return false;
			}
		}
	}
	
	return true;
}

void
File::delete_file(const char *path) {
	
	DIR *dir;
	dirent *dir_info;
	char file_path[PATH_MAX];
	if (is_file(path)) {
		remove(path);
		return;
	}
	if (is_dir(path)) {
		if ((dir = opendir(path)) == NULL) {
			_rmdir(path);
			closedir(dir);
			return;
		}
		while ((dir_info = readdir(dir)) != NULL) {
			if (is_special_dir(dir_info->d_name)) {
				continue;
			}
			get_file_path(path, dir_info->d_name, file_path);
			delete_file(file_path);
		}
		_rmdir(path);
		closedir(dir);
		return;
	}
	_unlink(path);
	
}

bool 
File::is_dir(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) == 0) {
		//lstat返回文件的信息，文件信息存放在stat结构中
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
			return true;
		}
#ifndef _WIN32
		if (S_ISLNK(statbuf.st_mode)) {
			char realFile[256] = { 0 };
			readlink(path, realFile, sizeof(realFile));
			return File::is_dir(realFile);
		}
#endif // !defined(_WIN32)
	}
	return false;
}

bool 
File::is_file(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) == 0) {
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			return true;
		}
#ifndef _WIN32
		if (S_ISLNK(statbuf.st_mode)) {
			char realFile[256] = { 0 };
			readlink(path, realFile, sizeof(realFile));
			return File::is_file(realFile);
		}
#endif // !_WIN32
	}
	return false;
}

bool 
File::is_special_dir(const char *path) {
	return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

