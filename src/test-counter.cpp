#include "include/countingbamboo.hpp"
#include <iostream>
using std::cout, std::endl;

CountingBamboo init_cbbf_default();
CountingBamboo init_cbbf_larger();

void counting_tests_simple();
void counting_tests_simple_larger();
void counting_tests_fill();

/* Wrap the tests in try - catch statements */
int main()
{
    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    /* Use to fix a known seed */
    seed = 1670624030;
    cout << "\n ### SEED = " << seed << " ### \n" << endl;

    srand(seed);
    counting_tests_simple();

    srand(seed);
    counting_tests_simple_larger();

    srand(seed);
    counting_tests_fill();


    cout <<  "\n======\nTests Complete\n======\n" << endl;
}



void counting_tests_simple()
{
    cout << "\n ++++ Begin counting simple test ++++ \n" << endl;
    CountingBamboo bbf = init_cbbf_default();

    int counter_max = 255;
    int sgl = 2*bbf._fgpt_per_bucket;
    try{
        cout << "Inserting... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.insert(1);
            if (i % counter_max == 0) {
                cout << bbf.count(1) << " " << flush;
            }
        }
        cout << endl << "Removing... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.remove(1);
            if (i % counter_max == 0) {
                cout << bbf.count(1) << " " << flush;
            }
        }
        cout << endl;
        // for (int i=0; i<8*bbf._fgpt_per_bucket; ++i) {
        //     bbf.insert(2);
        //     bbf.insert(3);
        //     print_count_bbf(bbf, 2);
        //     print_count_bbf(bbf, 3);
        //     print_count_bbf(bbf, 4);
        // }
        /* When cuckoo chaining is added, this should test the stop condition */
        // bbf.insert(2);
        cout << "Inserting to full... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.insert(1);
        }
        cout << " and now... ";
        bbf.insert(1);
    } catch (std::exception& e) {
        cout << "Buckets full!" << e.what() << endl;
    }
}


void counting_tests_simple_larger()
{
    cout << "\n ++++ Begin counting simple larger test ++++ \n" << endl;
    CountingBamboo bbf = init_cbbf_larger();

    int counter_max = 255;
    int sgl = 2*bbf._fgpt_per_bucket;
    try{
        cout << "Inserting... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.insert(1);
            if (i % counter_max == 0) {
                cout << bbf.count(1) << " " << flush;
            }
        }
        cout << endl << "Removing... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.remove(1);
            if (i % counter_max == 0) {
                cout << bbf.count(1) << " " << flush;
            }
        }
        cout << endl;
        cout << "Inserting to full... ";
        for (int i=0; i<sgl*counter_max; ++i) {
            bbf.insert(1);
        }
        cout << " and now... ";
        bbf.insert(1);
    } catch (std::exception& e) {
        cout << "Buckets full!" << e.what() << endl;
    }
}



void counting_tests_fill()
{
    cout << "\n ++++ Begin counting bamboo fill test ++++ \n" << endl;

    CountingBamboo bbf = init_cbbf_default();
    int elt = 0;
    
    try {
        while (elt < 480000000 && bbf.insert(elt)) {
            if (elt % 1000000 == 0) cout << elt << " " << std::flush;
            ++elt;
        }
    } catch (std::exception& e) {
        cout << "bucket full or something, error:" << e.what() << endl;
    }
    cout << "\nInserted: " << elt << " :: "
    << "Occupancy: " << bbf.occupancy() << "/" << bbf.capacity() << endl;

    /* Confirm that all elements are added correctly */
    
    for (int tst = 0; tst < elt; ++tst) {
        if (!bbf.remove(tst)) {
            cout << "*" << tst << "* " << flush;
            // u32 fgpt, seg_idx, b1, b2;
            // SegmentCounter* segment;
            // bbf._extract(tst, fgpt, seg_idx, segment, b1, b2);
            // cout << bitset<16>(fgpt) << " "
            //     << seg_idx << " " << segment << " " << b1 << " " << b2 << " " << endl;
        }
    }
    cout << "If no new integers printed before this line, then there are no false negatives!" << endl;
}



CountingBamboo init_cbbf_default()
{
    int bucket_idx_len = 8;
    int fgpt_size = 7;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return CountingBamboo(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}


CountingBamboo init_cbbf_larger()
{
    int bucket_idx_len = 8;
    int fgpt_size = 15;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return CountingBamboo(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}