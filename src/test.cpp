#include "include/bamboo.hpp"
#include <iostream>

using std::cout, std::endl;

Bamboo init_bbf_default();
Bamboo init_bbf_larger();
CountingBamboo init_cbbf_larger(bool is_overflow);

void hash_tests();
void bamboo_tests_simple();
void bamboo_tests_cuckoo();
void bamboo_tests_fill();
void bamboo_tests_larger_simple();
void bamboo_tests_larger_fill();
void cbamboo_tests_larger_count();


/* Wrap the tests in try - catch statements */
int main()
{
    cout << "\n======\nRun tests\n======\n" << endl;
    auto seed = time(NULL);
    /* Use to fix a known seed */
    // seed = 1670511456;
    cout << "\n ### SEED = " << seed << " ### \n" << endl;

    srand(seed);

    // hash_tests();
    // srand(seed);
    // bamboo_tests_simple();
    // bamboo_tests_cuckoo();
    srand(seed);
    bamboo_tests_fill();
    // bamboo_tests_larger_simple();
    srand(seed);
    bamboo_tests_larger_fill();

    // cbamboo_tests_larger_count();

    cout <<  "\n======\nTests Complete\n======\n" << endl;
}


/* 
 * Test definitions
 */

void hash_tests()
{
    SpookyHash h;
    for (int i=0; i< 32; ++i) {
        cout << h.Hash32(&i, 4, 0) << " ";
    }
    cout << endl;
    for (int i=0; i< 32; ++i) {
        cout << h.Hash32(&i, 4, rand()) << " ";
    }
    cout << endl;
}


void print_count_bbf(Bamboo &bbf, int elt)
{
    cout << "count(" << elt << "): " << bbf.count(elt) << endl;

}

/* Bamboo tests */

void bamboo_tests_simple()
{
    cout << "\n ++++ Begin bamboo simple test ++++ \n" << endl;
    Bamboo bbf = init_bbf_default();

    try{
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            bbf.insert(1);
            print_count_bbf(bbf, 1);
        }
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            bbf.remove(1);
            print_count_bbf(bbf, 1);
        }
        for (int i=0; i<8*bbf._fgpt_per_bucket; ++i) {
            bbf.insert(2);
            bbf.insert(3);
            print_count_bbf(bbf, 2);
            print_count_bbf(bbf, 3);
            print_count_bbf(bbf, 4);
        }
        /* When cuckoo chaining is added, this should test the stop condition */
        bbf.insert(2);
    } catch (std::exception& e) {
        cout << "Buckets full!" << e.what() << endl;
    }
}


void bamboo_tests_cuckoo()
{
    cout << "\n ++++ Begin bamboo cuckoo test ++++ \n" << endl;
    try {

        Bamboo bbf = init_bbf_default();
        int elt = 0;

        u32 fgpt;
        u32 bidx1, bidx2, seg_idx;
        Segment *seg;
        bbf._extract(elt, fgpt, seg_idx, seg, bidx1, bidx2);

        cout << "Test elt " << elt << ": fgpt = " << bitset<8>(fgpt)
            << " buckets = " << bidx1 << ", " << bidx2 << endl;

        /* fill both buckets */
        u8 alt_fgpt = fgpt ^ 2;
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            seg->buckets[bidx1].insert_fgpt_at(i, alt_fgpt);
            seg->buckets[bidx2].insert_fgpt_at(i, alt_fgpt);
        }

        for (int i=0; i< 5* bbf._fgpt_per_bucket + 1; ++i) {
            cout << "Inserting " << elt << ": " << endl;
            bbf.insert(elt);
        }
    } catch (std::exception& e) {
        cout << "bucket full or something, error:" << e.what() << endl;
    }
}

/* This test adds successive integers to the cuckoo filter until
 * an insertion fails. */
void bamboo_tests_fill()
{
    cout << "\n ++++ Begin bamboo fill test ++++ \n" << endl;

    Bamboo bbf = init_bbf_default();
    int elt = 0;
    
    try {
        while (elt < 2000000 && bbf.insert(elt)) {
            if (elt % 100000 == 0) cout << elt << " " << std::flush;
            ++elt;
        }
    } catch (std::exception& e) {
        cout << "bucket full or something, error:" << e.what() << endl;
    }
    cout << "\nInserted: " << elt << " :: "
     << "Occupancy: " << bbf.occupancy() << "/" << bbf.capacity() << endl;

    /* Confirm that all elements are added correctly */

    int cnt;
    for (int tst = 0; tst < elt; ++tst) {
        cnt = bbf.remove(tst);
        if (!cnt) {
            cout << "*" << tst << "* " << flush;
        } else {
            // cout << cnt << " ";
        }
    }
    cout << "If no new integers printed before this line, then there are no false negatives!" << endl;
}


void bamboo_tests_larger_simple()
{
    cout << "\n ++++ Begin bamboo larger simple test ++++ \n" << endl;

    Bamboo bbf = init_bbf_larger();

    try{
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            bbf.insert(1);
            print_count_bbf(bbf, 1);
        }
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            bbf.remove(1);
            print_count_bbf(bbf, 1);
        }
        for (int i=0; i<2*bbf._fgpt_per_bucket; ++i) {
            bbf.insert(2);
            bbf.insert(3);
            print_count_bbf(bbf, 2);
            print_count_bbf(bbf, 3);
            print_count_bbf(bbf, 4);
        }
        /* When cuckoo chaining is added, this should test the stop condition */
        bbf.insert(2);
    } catch (std::exception& e) {
        cout << "Buckets full!" << e.what() << endl;
    }
}


void bamboo_tests_larger_fill()
{
    cout << "\n ++++ Begin bamboo larger fill test ++++ \n" << endl;

    Bamboo bbf = init_bbf_larger();
    int elt = 0;
    
    try {
        while (elt < 9950000000 && bbf.insert(elt)) {
            // if (elt >= 930) {
            //     cout << "At " << elt << ": 55 = " << bbf.count(55) ;
            //     cout << " 935 = " << bbf.count(935) << endl;
            // }
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
        // if (tst % 1000000 == 0) cout << "V:" << tst << " " << std::flush;

        if (!bbf.remove(tst)) {
            cout << "*" << tst << "* " << flush;
            u32 fgpt, seg_idx, b1, b2;
            Segment* segment;
            bbf._extract(tst, fgpt, seg_idx, segment, b1, b2);
            cout << bitset<16>(fgpt) << " "
                << seg_idx << " " << segment << " " << b1 << " " << b2 << " " << endl;
        }
    }
    cout << "If no new integers printed before this line, then there are no false negatives!" << endl;
}


/* Counting Bamboo tests */


void cbamboo_tests_larger_count()
{
    cout << "\n ++++ Begin counting bamboo larger fill test ++++ \n" << endl;

    CountingBamboo cbbf = init_cbbf_larger(true);

    for (int i=0; i<64; ++i) {
        cbbf.increment(1);
        cout << cbbf.count(1) << " " << flush;
    } 
    cout << endl;

    for (int i=0; i<64; ++i) {
        cbbf.decrement(1);
        cout << cbbf.count(1) << " " << flush;
    } 
    cout << endl;

    cout << "\n Now count two elements \n" << endl;

    /* Prints triplets of counts of 1,2,3. 
     * Expected outcome: count of 3 should always be 0. */
    for (int i=0; i<64; ++i) {
        cbbf.increment(1);
        cbbf.increment(2);
        cout << "(" << cbbf.count(1) << " " << cbbf.count(2) 
            << " " << flush << cbbf.count(3) << ") " << flush;
    } 
    for (int i=0; i<64; ++i) {
        cbbf.decrement(1);
        cbbf.decrement(2);
        cout << "(" << cbbf.count(1) << " " << cbbf.count(2) 
            << " " << flush << cbbf.count(3) << ") " << flush;
    } 
}

void cbamboo_test_count_max()
{
    CountingBamboo cbbf = init_cbbf_larger(false);
    int elt = 99;

    try {
        while (true) {
        }
    } catch (std::exception& e) {
        cout << "bucket full or something, error:" << e.what() << endl;
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