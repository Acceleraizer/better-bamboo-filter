#ifndef BAMBOO
#define BAMBOO

#include <vector>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <exception>

#include "SpookyV2.h"

using std::vector, std::unordered_map, std::bitset;
using std::cout, std::endl, std::cerr, std::flush;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


extern int TEST_ENDIANNESS;
extern int CBBF_LITTLE_ENDIAN;

struct Bucket {
    u8 *_bits;    /* in memory: flag bit is to the right of the fgpt bits */
    u16 _len;

    Bucket() { }
    ~Bucket() { delete[] _bits; }

    /* Must be called after initialization to allocate the array.
     * This is here because I have yet to study copy-semantics, and
     * including this directly in the constructor gives issues. */
    void initialize(int capacity, int fgpt_size);

    // void _flip_if_big_endian(u32 &value);

    bool _occupied_idx (int idx, u8 fgpt_size);
    int _vacant_idx(u8 fgpt_size);
    bool check_fgpt(u32 fgpt, int idx, u8 fgpt_size);
    /* Returns the index where the first fingerprint is found */
    int find_fgpt(u32 fgpt, u8 fgpt_size); 
    int count_fgpt(u32 fpgt, u8 fgpt_size);
    bool insert_fgpt(u32 fgpt, u8 fgpt_size);
    bool remove_fgpt(u32 fgpt, u8 fgpt_size);

    void reset_fgpt_at(int idx, u8 fgpt_size);
    u32 get_fgpt_at(int idx, u8 fgpt_size);
    u32 remove_fgpt_at(int idx, u8 fgpt_size);
    void insert_fgpt_at(int idx, u32 fgpt, u8 fgpt_size);

    vector<u32> retrieve_all(u8 fgpt_size);
    void split_bucket(Bucket &dst, int sep_lvl, u8 fgpt_size);

    u32 occupancy(u8 fgpt_size);
};

struct Segment {
    vector<Bucket> buckets;
    Segment *overflow;
    int expansion_count;
    u8 fgpt_size;
    
    Segment(int num_buckets, u8 fgpt_size, int fgpt_per_bucket, int expansion__count) :
            expansion_count(expansion__count),
            fgpt_size(fgpt_size)
    {
        if (fgpt_size != 7 && fgpt_size != 15 && fgpt_size != 23)
            throw std::runtime_error("Fgpt size not supported");
        buckets = vector<Bucket>(num_buckets);
        overflow = nullptr;
        for (int i=0; i<num_buckets; ++i) {
            buckets[i].initialize(fgpt_per_bucket, fgpt_size);
        }
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
    u32 _bucket_mask;

    u32 _chain_max = 500;

    /* statistics */
    struct {
        int _expand_count;
        int _time;
        int _seg_find_cnt;

    } stats;

    BambooBase(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    virtual ~BambooBase();

    int count(int elt);
    virtual bool insert(int elt);
    bool remove(int elt);

    void adjust_to(int elt, int cnt);
    bool _cuckoo(Segment *segment, int seg_idx, u32 bi_main, u32 bi_alt, 
            u32 fgpt, u32 chain_len);
    u32 _find_segment_idx(u32 hash);
    bool _extract(int elt, u32 &fgpt, u32 &seg_idx, u32 &bidx1, u32 &bidx2); 

    inline u32 _alt_bucket(u32 fgpt, u32 bidx)
    {
        return (bidx ^ _h.Hash32(&fgpt, 4, _alt_seed)) & _bucket_mask;
    }
    inline u32 _compute_hash(int elt)
    {
        return _h.Hash32(&elt, 4, _seed);
    }

    virtual Segment *_get_segment(u32 seg_idx) = 0;
    virtual bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, 
            u32 bi_alt, u32 fgpt) = 0;
};


struct Bamboo : BambooBase {
    unordered_map<u32, Segment*> _segments;

    Bamboo(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~Bamboo();

    inline Segment *_get_segment(u32 seg_idx) override
    {
        if (_segments[seg_idx])
            return _segments[seg_idx];
        _segments.erase(seg_idx);
        return nullptr;
    }
    bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, 
            u32 bi_alt, u32 fgpt) override;

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

    bool insert(int elt) override;
    void expand(int seg_idx);

    inline Segment *_get_segment(u32 seg_idx) override
    {
        if (seg_idx < _segments.size())
            return _segments[seg_idx];
        return nullptr;
    }
    bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, u32 fgpt) override;

    // u32 occupancy();
    // u32 capacity();
};

struct CountingBamboo {
    vector<BambooBase*> bamboo_layers;
    int _depth;
    int _base_expn;
    
    int _max_depth;
    int _seg_idx_base;
    int _bucket_idx_len;
    int _fgpt_size;
    int _fgpt_per_bucket;
    bool _bamboo_implementation; // true = BambooOverflow, false = Bamboo

    CountingBamboo(int max_depth, int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base, bool bamboo_implementation);
    // CountingBamboo(int base_expn, vector<int> num_segments, vector<int> buckets_per_segment,
    //         vector<int> fgpt_size, vector<int> fgpt_per_bucket);
    ~CountingBamboo();

    int count(int elt);
    void increment(int elt);
    void decrement(int elt);  
};


#endif