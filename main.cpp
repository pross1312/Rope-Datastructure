#include <iostream>
#include <functional>
#include <chrono>

#include "rope.hpp"
using namespace std;
using namespace std::chrono;

void run(const char* name, function<void(void)> func) {
    high_resolution_clock clock;
    time_point start = clock.now();
    func();
    duration<double, ratio<1, 1>> diff = clock.now() - start;
    cout << "[" << name << "] Time: " << diff.count() << "\n";
}

int main() {
    string text(2000000ull, 'a');
    string temp(1000ull, 'b');
    Rope rope(text.data(), text.size());
    run("String:Insert", [&]() {
        for (size_t i = 0; i < 10000; i++) {
            text.insert(100+i, temp);
        }
    });
    run("Rope:Insert", [&]() {
        for (size_t i = 0; i < 10000; i++) {
            rope.insert(100+i, temp);
        }
        rope.rebalance();
    });

    run("String:Index", [&]() {
        for (size_t i = 0; i < 20000; i++) {
            (void)text[i];
        }
    });
    run("Rope:Index", [&]() {
        for (size_t i = 0; i < 20000; i++) {
            (void)rope[i];
        }
    });

    run("String:Erase", [&]() {
        for (size_t i = 0; i < 2000; i++) {
            text.erase(0, 100);
        }
    });
    run("Rope:Erase", [&]() {
        for (size_t i = 0; i < 2000; i++) {
            rope.erase(0, 100);
        }
        rope.rebalance();
    });
    run("Rope:ToString", [&]() {
        string str = rope.str();
        cout << str.size() << "\n";
    });

    return 0;
}
