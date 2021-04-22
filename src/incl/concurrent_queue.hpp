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

#include <atomic>
#include <queue>
#include <list>
#include <mutex>
#include <condition_variable>

template<class T>
class ConcurrentQueue{
private:
	mutable std::mutex mutex_;
	std::condition_variable empty_;
	std::queue<T> queue_;
	std::atomic<bool> done_;
public:
	ConcurrentQueue() : mutex_(), empty_(), queue_(), done_(false){}
	~ConcurrentQueue(void) = default;
	size_t size(void) const{
		return queue_.size();
	}
	bool empty(void) const{
		return queue_.empty();
	}
	void push(const T &val){
		std::unique_lock<std::mutex> lk(mutex_);
		queue_.push(val);
		empty_.notify_all();
	}
	bool pop(T &val, std::atomic<int> &threads_running){
		// return true if successfully got item, false if done
		std::unique_lock<std::mutex> lk(mutex_);
		threads_running--;
		if(threads_running <= 0 && queue_.empty()){
			done_ = true;
			empty_.notify_all();
		}
		while(queue_.empty() && !done_){
			empty_.wait(lk);
		}
		if(done_ && queue_.empty()){
			return false;
		}
		val = queue_.front();
		queue_.pop();
		threads_running++;
		return true;
	}
};
