/*
 *    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
 *    
 *    This file is part of cephgeorep.
 * 
 *    cephgeorep is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    cephgeorep is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "alert.hpp"
#include "signal.hpp"
#include <string>

extern "C" {
	#include <sys/stat.h>
}

class File{
private:
	off_t size_;
	bool is_directory_;
	size_t path_len_;
	char *path_;
	inline void init(const char *path, size_t snap_root_len, const struct stat &st){
		size_ = st.st_size;
		is_directory_ = S_ISDIR(st.st_mode);
		if(is_directory_){
			path_ = new char[strlen(path) + 1];
			strcpy(path_, path);
		}else{
			// split file path with /./
			// allocate new path
			path_ = new char[strlen(path) + 2 + 1]; // original path + ./ + \0
			// make copy of pointer for iteration
			char *dst_ptr = path_;
			// make pointer to iterate through snap path
			const char *src_ptr = path;
			// copy snap path into new path
			const char *snap_root_end = path + snap_root_len;
			while(src_ptr < snap_root_end)
				*(dst_ptr++) = *(src_ptr++); // copy snap root
			// append /.
			*(dst_ptr++) = '/';
			*(dst_ptr++) = '.';
			// set src pointer to original path starting after the snap path root
			src_ptr = snap_root_end;
			// copy the rest of the original path into the new path
			while(*src_ptr)
				*(dst_ptr++) = *(src_ptr++);
			// finally, append nul
			*dst_ptr = '\0';
			path_len_ = dst_ptr - path_;
		}
	}
public:
	File(void) : size_(0), is_directory_(false), path_len_(0), path_(0) {}
	File(const char *path, size_t snap_root_len) : path_len_(0){
		struct stat st;
		int res = lstat(path, &st);
		if(res == -1){
			int err = errno;
			Logging::log.error(std::string("Error calling stat on file: ") + strerror(err));
			l::exit(EXIT_FAILURE);
		}
		init(path, snap_root_len, st);
	}
	File(const char *path, size_t snap_root_len, const struct stat &st) : path_len_(0){
		init(path, snap_root_len, st);
	}
	File(const File &other) : size_(other.size_), is_directory_(other.is_directory_), path_len_(other.path_len_){
		path_ = new char[strlen(other.path_) + 1];
		strcpy(path_, other.path_);
	}
	~File(void){
		if(path_)
			delete[] path_;
	}
	uintmax_t size(void) const{
		return size_;
	}
	bool is_directory(void) const{
		return is_directory_;
	}
	const char *path(void) const{
		return path_;
	}
};
