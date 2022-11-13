#include "include/bamboo.hpp"
#include "iostream"

using std::cout, std::endl;

Bamboo init_default_bbf();

void hash_tests();
void bamboo_tests_simple();
void bamboo_tests_cuckoo();
void bamboo_tests_fill();


int main()
{
    cout << "\n==\nRun tests\n==\n" << endl;

    try {
        hash_tests();
        bamboo_tests_simple();
        bamboo_tests_cuckoo();
    } catch (std::exception& e) {
        cout << "bucket full or something, error:" << e.what() << endl;
    }
    // bamboo_tests_fill();

    cout <<  "\n==\nTests Complete\n==\n" << endl;
}


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


void print_count(Bamboo &bbf, int elt)
{
    cout << "count(" << elt << "): " << bbf.count(elt) << endl;

}

void bamboo_tests_simple()
{
    Bamboo bbf = init_default_bbf();
    int fgpt_per_bucket = 4;

    for (int i=0; i<2*fgpt_per_bucket; ++i) {
        bbf.insert(1);
        print_count(bbf, 1);
    }
    for (int i=0; i<2*fgpt_per_bucket; ++i) {
        bbf.remove(1);
        print_count(bbf, 1);
    }
    for (int i=0; i<2*fgpt_per_bucket; ++i) {
        bbf.insert(2);
        bbf.insert(3);
        print_count(bbf, 2);
        print_count(bbf, 3);
    }
    /* When cuckoo chaining is added, this should test the stop condition */
    if (!bbf.insert(2)) {
        cout << "Buckets full!" << endl;
    }
}


void bamboo_tests_cuckoo()
{
    cout << "\n  Begin bamboo cuckoo test\n" << endl;

    Bamboo bbf = init_default_bbf();
    int elt = 0;

    u8 fgpt;
    u32 bidx1, bidx2, seg_idx;
    bbf._extract(elt, fgpt, seg_idx, bidx1, bidx2);

    Segment *seg = bbf._get_segment(seg_idx);
    cout << "Test elt " << elt << ": fgpt = " << bitset<8>(fgpt)
        << " buckets = " << bidx1 << ", " << bidx2 << endl;

    /* fill both buckets */
    u8 alt_fgpt = fgpt ^ 1;
    for (int i=0; i<bbf._fgpt_per_bucket; ++i) {
        seg->buckets[bidx1].insert_fgpt_at(i, alt_fgpt);
        seg->buckets[bidx2].insert_fgpt_at(i, alt_fgpt);
    }

    for (int i=0; i< 2* bbf._fgpt_per_bucket + 1; ++i) {
        cout << "Inserting " << elt << ": " << endl;
        bbf.insert(elt);
    }
}

/* This test adds successive integers to the cuckoo filter until
 * an insertion fails. */
void bamboo_tests_fill()
{
    cout << "\n  Begin bamboo fill test\n" << endl;

    Bamboo bbf = init_default_bbf();
    int elt = 0;
    while (bbf.insert(elt)) {
        // cout << elt << " " << std::flush;
        ++elt;
    }
    cout << "Occupancy: " << bbf.occupancy() << "/" << bbf.capacity() << endl;
}


Bamboo init_default_bbf()
{
    int bucket_idx_len = 8;
    int fgpt_size = 7;
    int fgpt_per_bucket = 8;
    int seg_idx_base = 4;
    return Bamboo(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base);
}