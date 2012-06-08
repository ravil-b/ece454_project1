/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
A simple thread safe queue for this application.
*/


#include "ThreadSafeQueue.h"

#include <boost/thread/locks.hpp>

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue()
: stopReading_(false){
}

template<typename T>
void
ThreadSafeQueue<T>::push(T const &data){
    boost::mutex::scoped_lock lock(dataMutex_);
    q_.push(data);
    notEmpty_.notify_one();
}

template<typename T>
bool
ThreadSafeQueue<T>::pop(T *data){
    boost::mutex::scoped_lock lock(dataMutex_);
    while(q_.empty() && !stopReading_){
	notEmpty_.wait(lock);
    }
    if (stopReading_){
	return false;
    }
    *data = q_.front();
    q_.pop();
    return true;
}

template<typename T>
void
ThreadSafeQueue<T>::stopReading(){
    stopReading_ = true;
    notEmpty_.notify_all();
}

template<typename T>
bool
ThreadSafeQueue<T>::moreDataExpected(){
    return !stopReading;
}
