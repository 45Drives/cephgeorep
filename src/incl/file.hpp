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
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class File{
private:
	fs::file_status status_;
	uintmax_t size_;
	fs::path path_;
public:
	File(void) : status_(), size_(0), path_() {}
	File(const fs::path &path) : status_(fs::symlink_status(path)), path_(path) {
		size_ = (fs::is_regular_file(status_))? fs::file_size(path) : 0;
	}
	File(const File &other) : status_(other.status_), size_(other.size_), path_(other.path_) {}
	~File(void) = default;
	const fs::file_status &status(void) const{
		return status_;
	}
	uintmax_t size(void) const{
		return size_;
	}
	const fs::path &path(void) const{
		return path_;
	}
	void path(const fs::path &formatted_path){
		path_ = formatted_path;
	}
};
