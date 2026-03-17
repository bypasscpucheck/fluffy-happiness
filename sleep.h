/**
 * sleep.h
 * you can using sleep() void 
 *  sleep(1);    // sleep 1s
 *  msleep(1);  // sleep 1ms
 *  
 *  know bug:
 *      When using cout in a main function that contains sleep or msleep, you must add flush. 
 *      For example: cout << name << ":" << flush;
 */
#ifndef SLEEP_H
#define SLEEP_H

#include <chrono>
#include <thread>

inline void sleep(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

inline void msleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

#endif
