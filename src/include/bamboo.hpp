#ifndef BAMBOO
#define BAMBOO

#include "vector"
#include "unordered_map"
#include "SpookyV2.h"

using std::vector, std::unordered_map;

typedef uint32_t u32;
typedef uint8_t u8;

// inline u32 segment_idx(u32 hash, u32 seg_start, u32 seg_len) {
//     return (hash >> seg_start) & ((1<< seg_len) - 1);
// }


struct Bucket {
    vector<u8> bits;    /* first bit: occupancy flag
                         * next 7 bits: fingerprint */

    Bucket(int capacity)
    {
        bits = vector<u8>(capacity);
    }

    int vacancy();
    int check_fgpt(u8 fgpt, int idx);
    int find_fpgt(u8 fgpt);
    int count_fpgt(u8 fpgt);

};


struct Segment {
    vector<Bucket> buckets;
    

    Segment(int num_buckets, int fpgt_size, int fgpt_per_bucket)
    {
        buckets = vector<Bucket>(num_buckets, Bucket(fpgt_size * fgpt_per_bucket));
    }

    int count(u32 hash, int fgpt_idx, int fpgt_size);

};


struct Bamboo {
    unordered_map<u32, Segment*> segments;
    int _num_segments;
    int _buckets_per_segment;
    int _fgpt_size;
    int _fgpt_per_bucket;
    int _seg_idx_base;
    SpookyHash _h;
    u32 _seed;

    Bamboo(int num_segments, int buckets_per_segment, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~Bamboo();

    int count(int elt);
    void insert(int elt);
    void remove(int elt);
    void adjust_to(int elt, int cnt);

    Segment *_find_segment(u32 hash);

    // int _count(u32 hash);
    // void _insert(u32 hash);
    // void _remove(u32 hash);
    // void _adjust_to(u32 hash, int cnt);

    u32 compute_hash(int elt)
    {
        return _h.Hash32(&elt, 4, _seed);
    }
};


struct CountingBamboo {
    vector<Bamboo*> bamboo_layers;
    int _depth;
    int _base_expn;


    CountingBamboo(int base_expn, vector<int> num_segments, vector<int> buckets_per_segment,
            vector<int> fgpt_size, vector<int> fgpt_per_bucket);
    ~CountingBamboo();

    int count(int elt);
    void increment(int elt);
    void decrement(int elt);  
};


#endif