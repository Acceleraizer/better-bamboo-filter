#include "include/bamboo.hpp"
#include "include/memory.hpp"
#include <iostream>

using std::cout, std::endl;

Bamboo init_bbf_default();
Bamboo init_bbf_larger();
BambooOverflow init_overflow_bbf_larger();
CountingBamboo init_cbbf_larger(bool is_overflow);

/* Wrap the tests in try - catch statements */
int main()
{
    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    cout << "\n ### SEED = " << seed << " ### \n" << endl;

    /* Use to set seed */
    seed = 1668564475;
    srand(seed);
    size_t currentSize = getCurrentRSS( );
    size_t peakSize = getPeakRSS( );

    cout << "updated:" << currentSize << endl;
    cout << "peak:" << peakSize << endl;


    // BambooOverflow bbf = init_overflow_bbf_larger();
    // Bamboo bbf = init_bbf_larger();
    // CountingBamboo bbf = init_cbbf_larger(false);
    CountingBamboo bbf = init_cbbf_larger(true);

    for (int i = 0; i < 100000; ++i)
    {
        bbf.increment(rand() % 100000);
    }
    currentSize = getCurrentRSS( ) - currentSize;
    peakSize = getPeakRSS( ) - peakSize;

    cout << "updated:" << currentSize << endl;
    cout << "peak:" << peakSize << endl;


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