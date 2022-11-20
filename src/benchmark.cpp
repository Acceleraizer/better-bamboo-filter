#include "include/bamboo.hpp"
#include "include/memory.hpp"
#include <iostream>
#include <chrono>

using std::cout, std::endl;

Bamboo init_bbf_default();
Bamboo init_bbf_larger();
BambooOverflow init_overflow_bbf_larger();
CountingBamboo init_cbbf_larger(bool is_overflow);

/* Wrap the tests in try - catch statements */
/* Run this test with Valgrind to see memory nsage */
int main()
{
    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    cout << "\n ### SEED = " << seed << " ### \n" << endl;

    /* use to set seed */
    seed = 1668564475;
    srand(seed);
    // size_t currentSize = getCurrentRSS( );
    // size_t peakSize = getPeakRSS( );

    // cout << "updated:" << currentSize << endl;
    // cout << "peak:" << peakSize << endl;
    sleep(1);

    int r;
    int counts = 100000;

    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = t1;
    cout << " normal test start " << endl;
    {
        t1 = std::chrono::high_resolution_clock::now();

        Bamboo bbf = init_bbf_larger();
        cout << " :: " << bbf._segments.bucket_count() << " " << endl;

        for (int i = 0; i < counts; ++i)
        {
            r = rand() % 100000;
            bbf.insert(r);
        }
        cout << " :: " << bbf._segments.bucket_count() << " " << endl;
        cout << " :: expansions: " << bbf.stats._expand_count << endl;
        cout << " normal test done. Time = " << bbf.stats._time << endl;
        
        t2 = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        cout << "++++ Total: " << ns.count() << " ns" << endl;
        cout << "++   Segment finds = " << bbf.stats._seg_find_cnt << endl;
    }


    sleep(2);

    srand(seed);
    cout << " overflow test start " << endl;
    {
        t1 = std::chrono::high_resolution_clock::now();

        BambooOverflow bbf = init_overflow_bbf_larger();
        for (int i = 0; i < counts; ++i)
        {
            r = rand() % 100000;
            bbf.insert(r);
        }
        cout << " :: expansions: " << bbf.stats._expand_count << endl;
        cout << " overflow test done. Time = " << bbf.stats._time << endl;

        t2 = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        cout << "++++ Total: " << ns.count() << " ns" << endl;
        cout << "++   Segment finds = " << bbf.stats._seg_find_cnt << endl;
    }
    sleep(2);
    // Bamboo bbf = init_bbf_larger();
    // CountingBamboo bbf = init_cbbf_larger(false);
    cout << " counting test start " << endl;
    srand(seed);

    {
        CountingBamboo cbbf = init_cbbf_larger(true);

        for (int i = 0; i < 100000; ++i)
        {
            r = rand() % 100000;
            cbbf.increment(r);
        }
    }
    cout << " counting test done " << endl;
    sleep(2);
    
    // currentSize = getCurrentRSS( ) - currentSize;
    // peakSize = getPeakRSS( ) - peakSize;

    // cout << "updated:" << currentSize << endl;
    // cout << "peak:" << peakSize << endl;


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


CountingBamboo init_cbbf_larger(bool is_overflow)
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    int max_depth = 12;
    return CountingBamboo(max_depth, bucket_idx_len, fgpt_size, 
        fgpt_per_bucket, seg_idx_base, is_overflow);
}