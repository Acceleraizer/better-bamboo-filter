#include "include/bamboo.hpp"
#include "include/countingbamboo.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

using std::cout, std::endl, std::cin, std::cerr;

void test_abacus();
void test_counting();


int main()
{
    srand(0);
    test_abacus();
    // test_counting();
    
}


void test_abacus()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    int max_depth = 32;
    bool dif_hash = false;
    Abacus cbbf = Abacus(max_depth, bucket_idx_len, fgpt_size, 
        fgpt_per_bucket, seg_idx_base, dif_hash);


    long long num_elements;
    int elt;

    cin >> num_elements;

    cout << std::setprecision(8) << std::fixed;
    cerr << std::setprecision(8) << std::fixed;

    std::chrono::_V2::high_resolution_clock::time_point t1,t2;

    int ops_bucket_size = 100000;
    u64 time = 0;
    try {
        for (int i=0; i<num_elements; ++i) {
            cin >> elt;
            t1 = std::chrono::high_resolution_clock::now();
            cbbf.increment(elt);
            t2 = std::chrono::high_resolution_clock::now();
            time += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1)
                .count();
        
            if ((i+1) % ops_bucket_size == 0) {
                cout << i+1 << " " << (double) time / ops_bucket_size << endl; 
                time = 0;
            }

            // if ((i+1) % (num_elements/10) == 0)
            //     cerr << 100*(i+1)/num_elements << "%.. " << flush;
        }
    } catch (std::exception& e) {
            cout << endl << "Bucket full or something, error: " << e.what() << endl;
    }
    // cerr << "Abacus insertion complete. Received " << num_elements << " elements." << endl;
    // cout << endl;
    cbbf.dump_abacus();

    // return;

    cerr << "Verifying counts:" << endl;
    int actual_count, count;
    cin >> num_elements;
    for (int i=0; i<num_elements; ++i) {
        cin >> elt >> actual_count;
        count = cbbf.count(elt);
        cerr << elt << " " << count << " " << actual_count << '\n';
    }
}




void test_counting()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    CountingBamboo cbbf(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);

    int num_elements;
    int elt;

    cin >> num_elements;

    cout << std::setprecision(8) << std::fixed;
    
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;

    int ops_bucket_size = 1000;
    u64 time = 0;
    try {
        for (int i=0; i<num_elements; ++i) {
            cin >> elt;
            t1 = std::chrono::high_resolution_clock::now();
            cbbf.insert(elt);
            t2 = std::chrono::high_resolution_clock::now();
            time += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1)
                .count();
            
            if ((i+1) % ops_bucket_size == 0) {
                cout << i+1 << " " << (double) time / ops_bucket_size << endl; 
                time = 0;
            }
        }
    } catch (std::exception& e) {
            cout << endl << "Bucket full or something, error: " << e.what() << endl;
    }
    // cout << "Counting Bamboo insertion complete. Received " << num_elements << " elements." << endl;

    // cout << endl;
    cbbf.dump_succinct();

    cerr << "Verifying counts:" << endl;
    int actual_count, count;
    cin >> num_elements;
    for (int i=0; i<num_elements; ++i) {
        cin >> elt >> actual_count;
        count = cbbf.count(elt);
        cerr << elt << " " << count << " " << actual_count << "\n";
    }
}

