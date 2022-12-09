#include "include/bamboo.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using std::cout, std::endl, std::cin;
Bamboo init_bbf_default();
Bamboo init_bbf_larger();
CountingBamboo init_cbbf_larger(bool is_overflow);
CountingBamboo init_cbbf_default(bool is_overflow);


int main()
{
    CountingBamboo cbbf = init_cbbf_larger(false);

    int num_elements;
    int elt;

    cin >> num_elements;

    for (int i=0; i<num_elements; ++i) {
        cin >> elt;
        cbbf.increment(elt);
    }

    cout << "Abacus insertion complete. Received "
     << cbbf.occupancy() << "/" << num_elements << " elements." << endl;

    cout << endl;
    cbbf.dump_abacus();
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


CountingBamboo init_cbbf_default(bool dif_hash)
{
    int bucket_idx_len = 8;
    int fgpt_size = 7;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    int max_depth = 12;
    return CountingBamboo(max_depth, bucket_idx_len, fgpt_size, 
        fgpt_per_bucket, seg_idx_base, dif_hash);
}


CountingBamboo init_cbbf_larger(bool dif_hash)
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    int max_depth = 20;
    return CountingBamboo(max_depth, bucket_idx_len, fgpt_size, 
        fgpt_per_bucket, seg_idx_base, dif_hash);
}