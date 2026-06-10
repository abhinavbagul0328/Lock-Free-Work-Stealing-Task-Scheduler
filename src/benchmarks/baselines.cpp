#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std;
using namespace std::chrono;

struct Unpadded { volatile uint64_t v = 0; };
struct alignas(64) Padded { volatile uint64_t v = 0; };

void bench_false_sharing() {
    int N = 50000000;
    int num_threads = 4;
    
    Unpadded u[4];
    auto t1 = high_resolution_clock::now();
    vector<thread> threads;
    for(int i=0; i<num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for(int j=0; j<N; ++j) u[i].v = u[i].v + 1;
        });
    }
    for(auto& t : threads) t.join();
    auto t2 = high_resolution_clock::now();
    double unpad_time = duration<double>(t2 - t1).count();

    Padded p[4];
    auto t3 = high_resolution_clock::now();
    vector<thread> threads2;
    for(int i=0; i<num_threads; ++i) {
        threads2.emplace_back([&, i]() {
            for(int j=0; j<N; ++j) p[i].v = p[i].v + 1;
        });
    }
    for(auto& t : threads2) t.join();
    auto t4 = high_resolution_clock::now();
    double pad_time = duration<double>(t4 - t3).count();

    cout << "False Sharing: Unpadded = " << unpad_time << "s, Padded = " << pad_time << "s, Gain = " << unpad_time/pad_time << "x\n";
}

int main() {
    bench_false_sharing();
    return 0;
}
