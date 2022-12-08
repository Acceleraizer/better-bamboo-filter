#include "include/bamboo.hpp"
#include "include/memory.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using std::cout, std::endl, std::cin;

Bamboo init_bbf_default();
Bamboo init_bbf_larger();
BambooOverflow init_overflow_bbf_larger();
CountingBamboo init_cbbf_larger(bool is_overflow);

/* Wrap the tests in try - catch statements */
/* Run this test with Valgrind to see memory nsage */
int main()
{
    /* Select the experiment configurations */
    vector<int> counts;
    cout << "Enter counts (0 to end) " << endl;
    int count;
    while ((cin >> count), count) {
        counts.push_back(count);
    }

    /* Prepare outfile for writing results */
    cout << "Enter outfile" << endl;
    std::string OF;
    cin >> OF;
    std::ofstream outfile;
    outfile.open(OF, std::ios::out);


    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    /* use to set seed */
    // seed = 1668564475;
    srand(seed);
    cout << "\n ### SEED = " << seed << " ### \n" << endl;



    int r;
    // int counts = 150000;
    int reps = 200;
    int universe = 1000000;

    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = t1;


    sleep(1);

    cout << "\n normal test start " << endl;
    for (int count: counts)
    {
        u64 total_ns = 0;

        for (int i= 0 ; i< reps; ++ i) {
            Bamboo bbf = init_bbf_larger();

            t1 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < count; ++i)
            {
                r = rand() % universe;
                bbf.insert(r);
            }
            // cout << " :: expansions: " << bbf.stats._expand_count << endl;
            // cout << " normal test done. ** Time = " << bbf.stats._time << " ** " << endl;
            
            t2 = std::chrono::high_resolution_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
            total_ns += ns.count();
        }

        cout << "++++ " << count << ": " << total_ns/reps << " ns" << endl;
        outfile << count << "\t" << total_ns << "\t" << reps << "\n";
    }

    srand(seed);
    sleep(1);
    cout << " overflow test start " << endl;
    for (int count: counts)
    {
        u64 total_ns = 0;
        try {
            for (int i= 0 ; i< reps; ++ i) {
                BambooOverflow bbf = init_overflow_bbf_larger();
                
                t1 = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < count; ++i)
                {
                    r = rand() % universe;
                    bbf.insert(r);
                }
                // cout << " :: expansions: " << bbf.stats._expand_count << endl;
                // cout << " overflow test done. ** Time = " << bbf.stats._time << " ** " << endl;

                t2 = std::chrono::high_resolution_clock::now();
                auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
                total_ns += ns.count();
            }
        } catch (const std::exception &e) {
            cout << e.what() << endl;
            total_ns = -1;
        }
        
        cout << "++++ " << count << ": " << total_ns/reps << " ns" << endl;
        outfile << count << "\t" << total_ns << "\t" << reps << "\n";
    }


   
    outfile.close();

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