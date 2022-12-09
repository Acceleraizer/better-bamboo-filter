#include "include/bamboo.hpp"
#include "include/memory.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using std::cout, std::endl, std::cin;

Bamboo init_bbf_default();
Bamboo init_bbf_larger();
BambooOverflow init_overflow_bbf_larger();
Abacus init_af_larger(bool is_overflow);

/* Wrap the tests in try - catch statements */
/* Run this test with Valgrind to see memory nsage */
int main()
{
    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    /* use to set seed */
    // seed = 1668564475;
    // srand(seed);
    cout << "\n ### SEED = " << seed << " ### \n" << endl;


    sleep(1);

    int r;
    int count = 50000;
    int reps = 50;

    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = t1;

    srand(seed);
    cout << " overflow test start " << endl;
    {
        u64 total_ns = 0;
        u64 time_ns = 0;
        u64 counter0 = 0;
        u64 counter1 = 0;

        for (int i= 0 ; i< reps; ++ i) {
            BambooOverflow bbf = init_overflow_bbf_larger();
            
            t1 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < count; ++i)
            {
                r = rand() % 100000;
                bbf.insert(r);
            }
            cout << " :: expansions: " << bbf.stats._expand_count << endl;
            // cout << " overflow test done. ** Time = " << bbf.stats._time << " ** " << endl;
            time_ns += bbf.stats._time;
            counter0 += bbf.stats._counter0;
            counter1 += bbf.stats._counter1;

            t2 = std::chrono::high_resolution_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
            total_ns += ns.count();
        }
        cout << "++++ \n Average total time: " << total_ns/reps << " ns" << endl;
        cout << "Average op Time: " << time_ns/reps << " ns" << endl;
        cout << "Average cuckoos " << counter0/reps << endl;
        cout << "Average chain max " << counter1/reps << endl;
    }
    sleep(1);

    cout << "\n normal test start " << endl;
    {
        u64 total_ns = 0;
        u64 time_ns = 0;
        u64 counter0 = 0;
        u64 counter1 = 0;

        for (int i= 0 ; i< reps; ++ i) {
            Bamboo bbf = init_bbf_larger();

            t1 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < count; ++i)
            {
                r = rand() % 100000;
                bbf.insert(r);
            }
            cout << " :: expansions: " << bbf.stats._expand_count << endl;
            // cout << " normal test done. ** Time = " << bbf.stats._time << " ** " << endl;
            time_ns += bbf.stats._time;
            counter0 += bbf.stats._counter0;
            counter1 += bbf.stats._counter1;
            
            t2 = std::chrono::high_resolution_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
            total_ns += ns.count();
        }

        cout << "++++ \n Average total time: " << total_ns/reps << " ns" << endl;
        cout << "Average op Time :" << time_ns/reps << " ns" << endl;
        cout << "Average cuckoos " << counter0/reps << endl;
        cout << "Average chain max " << counter1/reps << endl;
    }


    sleep(1);
    // cout << " counting test start " << endl;
    // srand(seed);

    // {
    //     Abacus cbbf = init_af_larger(true);

    //     for (int i = 0; i < 100000; ++i)
    //     {
    //         r = rand() % 100000;
    //         cbbf.increment(r);
    //     }
    // }
    // cout << " counting test done " << endl;
    // sleep(2);
    
    cout <<  "\n======\nTests Complete\n======\n" << endl;
}


Bamboo init_bbf_default()
{
    int bucket_idx_len = 8;
    int fgpt_size = 7;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return Bamboo(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}


Bamboo init_bbf_larger()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return Bamboo(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}

BambooOverflow init_overflow_bbf_larger()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return BambooOverflow(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}


Abacus init_af_larger(bool is_overflow)
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    int max_depth = 12;
    return Abacus(max_depth, bucket_idx_len, fgpt_size, 
        fgpt_per_bucket, seg_idx_base, is_overflow);
}