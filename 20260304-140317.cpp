#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "sleep.h"
using namespace std;

int main() {
    string name = "hello world";
    cout << name << ":"<< flush;
    sleep(5);
    cin >> name;
    cout << name;
}
