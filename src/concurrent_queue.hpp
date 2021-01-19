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

#include <list>
#include <mutex>
#include <condition_variable>

template<class T>
class ConcurrentList{
private:
	std::mutex mutex_;
	std::condition_variable cv_;
	std::list<T> &list_;
	bool done_;
public:
	ConcurrentList(std::list<T> &list) : list_(list){
		done_ = false;
	}
	~ConcurrentList(void) = default;
	size_t size(void) const{
		return list_.size();
	}
	bool empty(void) const{
		return list_.empty();
	}
	void push_back(const T &val){
		{
			std::unique_lock<std::mutex> lk(mutex_);
			list_.push_back(val);
		}
		cv_.notify_one();
	}
	void pop_front(T &val){
		{
			std::unique_lock<std::mutex> lk(mutex_);
			cv_.wait(lk, [this](){ return !this->empty() && !this->done_; });
			if(done_) return;
			val = list_.front();
			list_.pop_front();
		}
	}
	void wake_all(void){
		done_ = true;
		cv_.notify_all();
	}
	std::list<T> &internal_list(void){
		return list_;
	}
};
