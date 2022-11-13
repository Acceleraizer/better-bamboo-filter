#ifndef BAMBOO
#define BAMBOO

#include <vector>
#include <unordered_map>
#include <bitset>
#include <iostream>

#include "SpookyV2.h"

using std::vector, std::unordered_map, std::bitset;
using std::cout, std::endl, std::cerr, std::flush;

typedef uint32_t u32;
typedef uint8_t u8;

// inline u32 segment_idx(u32 hash, u32 seg_start, u32 seg_len) {
//     return (hash >> seg_start) & ((1<< seg_len) - 1);
// }

struct Bucket {
    vector<u8> _bits;    /* first bit: occupancy flag
                         * next 7 bits: fingerprint */

    Bucket(int capacity)
    {
        _bits = vector<u8>(capacity, 0);
    }

    int vacant_idx();
    bool check_fgpt(u8 fgpt, int idx);
    int find_fgpt(u8 fgpt); /* Returns the index where the first fingerprint is found */
    int count_fgpt(u8 fpgt);
    bool insert_fgpt(u8 fgpt);
    bool remove_fgpt(u8 fgpt);
    vector<u8> retrieve_all();

    u8 remove_fgpt_at(int idx);
    void insert_fgpt_at(int idx, u8 fgpt);

    void split_bucket(Bucket bucket, int separation_level);

    u32 occupancy();
};

struct Segment {
    vector<Bucket> buckets;
    Segment *overflow;
    int expansion_count;
    
    Segment(int num_buckets, int fpgt_size, int fgpt_per_bucket, int expansion__count) :
            expansion_count(expansion__count)
    {
        buckets = vector<Bucket>(num_buckets, Bucket(fgpt_per_bucket));
    }

    u32 occupancy();
};


struct BambooBase {
    int _num_segments;
    int _bucket_idx_len;
    int _fgpt_size;
    int _fgpt_per_bucket;
    int _seg_idx_base;
    SpookyHash _h;
    u32 _seed;
    u32 _alt_seed;

    u32 _chain_max = 500;

    BambooBase(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~BambooBase();

    int count(int elt);
    bool insert(int elt);
    bool remove(int elt);

    void adjust_to(int elt, int cnt);
    bool _cuckoo(Segment *segment, int seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt, u32 chain_len);
    u32 _find_segment_idx(u32 hash);
    bool _extract(int elt, u8 &fgpt, u32 &seg_idx, u32 &bidx1, u32 &bidx2); 

    inline u32 _alt_bucket(u8 fgpt, u32 bidx)
    {
        return bidx ^ (u8) _h.Hash32(&fgpt, 4, _alt_seed);
    }
    inline u32 _compute_hash(int elt)
    {
        return _h.Hash32(&elt, 4, _seed);
    }

    virtual Segment *_get_segment(u32 seg_idx);
    virtual bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt);
};

struct Bamboo : BambooBase {
    unordered_map<u32, Segment*> _segments;

    Bamboo(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~Bamboo();

    inline Segment *_get_segment(u32 seg_idx)
    {
        if (_segments[seg_idx])
            return _segments[seg_idx];
        return nullptr;
    }
    bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt);

    u32 occupancy();
    u32 capacity();
};

struct BambooOverflow : BambooBase {
    int _expand_prompt;
    int _insert_count;
    int _next_seg_idx;
    int _expand_base;

    vector<Segment*> _segments;

    BambooOverflow(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~BambooOverflow();

    bool insert(int elt);
    void expand(int seg_idx);

    inline Segment *_get_segment(u32 seg_idx)
    {
        if (_segments[seg_idx])
            return _segments[seg_idx];
        return nullptr;
    }
    bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt);

    // u32 occupancy();
    // u32 capacity();
};

struct CountingBamboo {
    vector<Bamboo*> bamboo_layers;
    int _depth;
    int _base_expn;
    
    int _max_depth;
    int _seg_idx_base;
    int _bucket_idx_len;
    int _fgpt_size;
    int _fgpt_per_bucket;

    CountingBamboo(int max_depth, int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    CountingBamboo(int base_expn, vector<int> num_segments, vector<int> buckets_per_segment,
            vector<int> fgpt_size, vector<int> fgpt_per_bucket);
    ~CountingBamboo();

    int count(int elt);
    void increment(int elt);
    void decrement(int elt);  
};


#endif