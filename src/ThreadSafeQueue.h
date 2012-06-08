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

template<typename T>
class ThreadSafeQueue {

 public:
    ThreadSafeQueue();
    // Pushes element to the back of the queue
    // If another thread is locked waiting for data, it is unlocked
    void push(T const &data);
    // Provides a copy of the first element and removes it from the queue
    // If the queue is empty, the calling thread blocks.
    bool pop(T *data);
    // To signify that no more incoming data is expected to be available.
    // For example, the connection from a socket is closed and no more data will
    // be pushed to the queue.
    // All readers waiting to pop are unblocked. 
    // A simple implementation, but should work for our purpose - one producer.
    void stopReading();
    // Check if stopReading has been issued.
    bool moreDataExpected();

 private:
    std::queue<T> q_;
    boost::condition_variable notEmpty_;
    mutable boost::mutex dataMutex_;
    bool stopReading_;
};


#endif

