/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
A simple thread safe queue for this application.
*/


#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>

template<typename T>
class ThreadSafeQueue {

 public:
    ThreadSafeQueue()
	: stopReading_(false),
	maxSize_(10){
    }
    // Pushes element to the back of the queue
    // If another thread is locked waiting for data, it is unlocked.
    // If the queue is cuurently at its max size, the calling thread is locked
    // until more data is popped from the queue.
    void push(T const &data){
	boost::mutex::scoped_lock lock(dataMutex_);
	while (q_.size() == (unsigned)maxSize_){
	    notFull_.wait(lock);
	}
	q_.push(data);
	notEmpty_.notify_one();
    }
    // Provides a copy of the first element and removes it from the queue
    // If the queue is empty, the calling thread blocks.
    bool pop(T *data){
        boost::mutex::scoped_lock lock(dataMutex_);
        while (q_.empty() && !stopReading_){
	    notEmpty_.wait(lock);
        }
        if (q_.empty() && stopReading_){
	    return false;
        }
	if (q_.size() == (unsigned)maxSize_){
	    notFull_.notify_one();
        }
        *data = q_.front();
        q_.pop();
	
        return true;
    }
    // To signify that no more incoming data is expected to be available.
    // For example, the connection from a socket is closed and no more data will
    // be pushed to the queue.
    // All readers waiting to pop are unblocked. 
    // A simple implementation, but should work for our purpose - one producer.
    void stopReading(){
	stopReading_ = true;
	notEmpty_.notify_all();
    }

    // Check if stopReading has been issued.
    bool moreDataExpected(){
	return !stopReading;
    }
    // The max size of the queue upon reaching which producers should block
    // until data is poped.
    void maxSize(int maxSize){
	maxSize_ = maxSize;
    }

 private:
    std::queue<T> q_;
    boost::condition_variable notEmpty_;
    boost::condition_variable notFull_;
    boost::mutex dataMutex_;
    bool stopReading_;
    int maxSize_;
};


#endif

