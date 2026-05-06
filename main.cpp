#include "arm_neon_processor.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <sys/utsname.h>

using namespace std;

string get_timestamp() {
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    tm* tm_now = localtime(&time_t_now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", tm_now);
    return string(buf);
}

string get_system_info() {
    struct utsname un;
    if (uname(&un) == 0) {
        return string(un.sysname) + " " + un.release + " (" + un.machine + ")";
    }
    return "Unknown system";
}

bool test_correctness(ofstream& log) {
    const size_t test_sizes[] = {0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 100, 1000, 10000};
    
    for (size_t n : test_sizes) {
        vector<int32_t> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = static_cast<int32_t>(i % 100) - 50;
        }
        
        int64_t scalar_result = process_array_scalar(data.data(), n);
        int64_t neon_result = process_array_neon(data.data(), n);
        int64_t neon_unrolled_result = process_array_neon_unrolled(data.data(), n);
        
        if (scalar_result != neon_result || scalar_result != neon_unrolled_result) {
            log << "ERROR: Mismatch at size " << n << ": scalar=" << scalar_result 
                << ", neon=" << neon_result 
                << ", unrolled=" << neon_unrolled_result << endl;
            cerr << "ERROR: Mismatch at size " << n << ": scalar=" << scalar_result 
                 << ", neon=" << neon_result 
                 << ", unrolled=" << neon_unrolled_result << endl;
            return false;
        }
        log << "OK: Size " << n << " -> " << scalar_result << endl;
    }
    
    vector<int32_t> mixed = {-100, 0, 50, -25, 0, 75, -1, 1};
    int64_t expected = 100 + 0 + 50 + 25 + 0 + 75 + 1 + 1;
    int64_t result = process_array_neon(mixed.data(), mixed.size());
    if (result != expected) {
        log << "ERROR: Mixed test: expected=" << expected << ", got=" << result << endl;
        cerr << "Mixed test failed: expected=" << expected << ", got=" << result << endl;
        return false;
    }
    log << "OK: Mixed test -> " << result << endl;
    
    return true;
}

bool test_null_pointer(ofstream& log) {
    try {
        process_array_scalar(nullptr, 0);
        process_array_neon(nullptr, 0);
        process_array_neon_unrolled(nullptr, 0);
    } catch (...) {
        log << "ERROR: Null pointer with size 0 should not throw" << endl;
        cerr << "Null pointer with size 0 should not throw" << endl;
        return false;
    }
    
    bool caught_scalar = false;
    bool caught_neon = false;
    bool caught_unrolled = false;
    
    try {
        process_array_scalar(nullptr, 1);
    } catch (const invalid_argument&) {
        caught_scalar = true;
    }
    
    try {
        process_array_neon(nullptr, 1);
    } catch (const invalid_argument&) {
        caught_neon = true;
    }
    
    try {
        process_array_neon_unrolled(nullptr, 1);
    } catch (const invalid_argument&) {
        caught_unrolled = true;
    }
    
    if (!caught_scalar || !caught_neon || !caught_unrolled) {
        log << "ERROR: Null pointer with non-zero size should throw" << endl;
        cerr << "Null pointer with non-zero size should throw" << endl;
        return false;
    }
    
    log << "OK: Null pointer handling" << endl;
    return true;
}

void run_benchmark(ofstream& log) {
    const size_t sizes[] = {1000, 10000, 100000, 1000000};
    
    log << "Size        Scalar(ms)    NEON(ms)      Unrolled(ms)  Speedup" << endl;
    cout << "Size        Scalar(ms)    NEON(ms)      Unrolled(ms)  Speedup" << endl;
    
    for (size_t n : sizes) {
        vector<int32_t> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = static_cast<int32_t>(i % 100) - 50;
        }
        
        const int iterations = 100;
        
        clock_t start = clock();
        volatile int64_t sum_scalar = 0;
        for (int iter = 0; iter < iterations; ++iter) {
            sum_scalar = process_array_scalar(data.data(), n);
        }
        double time_scalar = static_cast<double>(clock() - start) / CLOCKS_PER_SEC * 1000.0 / iterations;
        
        start = clock();
        volatile int64_t sum_neon = 0;
        for (int iter = 0; iter < iterations; ++iter) {
            sum_neon = process_array_neon(data.data(), n);
        }
        double time_neon = static_cast<double>(clock() - start) / CLOCKS_PER_SEC * 1000.0 / iterations;
        
        start = clock();
        volatile int64_t sum_unrolled = 0;
        for (int iter = 0; iter < iterations; ++iter) {
            sum_unrolled = process_array_neon_unrolled(data.data(), n);
        }
        double time_unrolled = static_cast<double>(clock() - start) / CLOCKS_PER_SEC * 1000.0 / iterations;
        
        double speedup = time_scalar / time_neon;
        
        log << setw(12) << n 
            << setw(14) << fixed << setprecision(4) << time_scalar
            << setw(14) << time_neon 
            << setw(14) << time_unrolled 
            << setw(12) << setprecision(2) << speedup << endl;
        
        cout << setw(12) << n 
             << setw(14) << fixed << setprecision(4) << time_scalar
             << setw(14) << time_neon 
             << setw(14) << time_unrolled 
             << setw(12) << setprecision(2) << speedup << endl;
    }
}

int main() {
    string log_filename = "benchmark_" + get_timestamp() + ".log";
    ofstream log_file(log_filename);
    
    if (!log_file.is_open()) {
        cerr << "Cannot open log file: " << log_filename << endl;
        return 1;
    }
    
    log_file << "ARM NEON Array Processor Benchmark" << endl;
    log_file << "Time: " << get_timestamp() << endl;
    log_file << "System: " << get_system_info() << endl;
    log_file << endl;
    
    cout << "ARM NEON Array Processor Test" << endl;
    cout << "Log file: " << log_filename << endl;
    
    cout << "\nRunning correctness tests..." << endl;
    if (!test_correctness(log_file)) {
        log_file << "RESULT: FAILED" << endl;
        cerr << "Correctness tests FAILED" << endl;
        return 1;
    }
    log_file << "RESULT: PASSED" << endl << endl;
    cout << "Correctness tests PASSED" << endl;
    
    cout << "\nRunning null pointer tests..." << endl;
    if (!test_null_pointer(log_file)) {
        log_file << "RESULT: FAILED" << endl;
        cerr << "Null pointer tests FAILED" << endl;
        return 1;
    }
    log_file << "RESULT: PASSED" << endl << endl;
    cout << "Null pointer tests PASSED" << endl;
    
    cout << "\nRunning benchmark..." << endl;
    run_benchmark(log_file);
    
    log_file << endl << "Summary:" << endl;
    log_file << "All tests completed successfully!" << endl;
    
    cout << "\nAll tests completed successfully!" << endl;
    cout << "Results saved to: " << log_filename << endl;
    
    log_file.close();
    
    return 0;
}