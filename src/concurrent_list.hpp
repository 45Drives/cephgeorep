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
class ConcurrrentList{
private:
	std::mutex mutex_;
	std::condition_variable cv_;
	std::list<T> queue_;
public:
	bool empty(void) const{
		return queue_.empty();
	}
	void push_back(const T &val){
		{
			std::unique_lock<std::mutex> lk(mutex_);
			queue_.push_back(val);
		}
		cv_.notify_one();
	}
	const T& pop_front(void){
		{
			std::unique_lock<std::mutex> lk(mutex_);
			cv_.wait(lk, [this](){ return !queue_.empty(); });
			auto val = queue_.front();
			queue_.pop_front();
			return val;
		}	
	}
};
