#include "include/bamboo.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

using std::cout, std::endl, std::cin;
Bamboo init_bbf_default();
Bamboo init_bbf_larger();
BambooOverflow init_overflow_bbf_default();
BambooOverflow init_overflow_bbf_larger();

void time(BambooBase &bbf);
void space(BambooBase &bbf);


int main() 
{
    srand(0);
    // int bucket_idx_len = 10;
    // int fgpt_size = 7;
    // int fgpt_per_bucket = 8;
    // int seg_idx_base = 2;
    // Bamboo bbf(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);

    BambooOverflow bbf = init_overflow_bbf_default();
    // Bamboo bbf = init_bbf_default();
    // time(bbf);
    space(bbf);
    
}


void time(BambooBase &bbf) 
{
    int num_elements;
    int elt;
    int i;
    cin >> num_elements;
    cout << std::setprecision(8) << std::fixed;
    
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;
    
    int ops_bucket_size = 1000;

    u64 time = 0;
    try {

        for (i=0; i<num_elements; ++i) {
            cin >> elt;
            t1 = std::chrono::high_resolution_clock::now();
            bbf.insert(elt);
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
}


void space(BambooBase &bbf) 
{
    int num_elements;
    int elt;
    int i;
    cin >> num_elements;
    cout << std::setprecision(8) << std::fixed;

    try {
        for (i=0; i<num_elements; ++i) {
            cin >> elt;
            bbf.insert(elt);
            if ((i+1) % 100 == 0) {
                cout << i+1 << " "; 
                bbf.dump_succinct();
            }
        }
    } catch (std::exception& e) {
        cout << endl << "Bucket full or something, error: " << e.what() << endl;
    }
}



/* Default filters for testing */


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


BambooOverflow init_overflow_bbf_default()
{
    int bucket_idx_len = 8;
    int fgpt_size = 7;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return BambooOverflow(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}


BambooOverflow init_overflow_bbf_larger()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return BambooOverflow(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}
