#ifndef CNT_BAMBOO
#define CNT_BAMBOO

#include <vector>
#include <chrono>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <exception>
#include <string>

#include "SpookyV2.h"

using std::vector, std::unordered_map, std::bitset;
using std::cout, std::endl, std::cerr, std::flush;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


struct BucketCounter {
    u8 *_bits;    /* Each fgpt is stored in a contiguous subarray along with
                   * a flag bit. See implementation for details */
    u16 _len;     /* Length of array allocated to *_bits* */
    u8 _entry_size;
    u8 _step;

    BucketCounter(u8 fgpt_size) {_entry_size = fgpt_size + 8; 
                          _step = (_entry_size + 7) / 8;}
    ~BucketCounter() { delete[] _bits; }

    /* Must be called after initialization to allocate the array.
     * This is here because I have yet to study copy-semantics, and
     * including this directly in the constructor gives issues. */
    void initialize(int capacity, int fgpt_size);

    bool _occupied_idx (int idx);
    int _vacant_idx();
    u32 count_at(int idx);
    u32 count_fgpt_at(u32 fgpt, int idx);
    /* Returns the index where the first fingerprint is found */
    int find_fgpt(u32 fgpt); 
    int count_fgpt(u32 fpgt);
    u32 insert_fgpt(u32 fgpt);
    u32 insert_fgpt_count(u32 fgpt, u32 &count);
    void insert_fgpt_at(int idx, u32 fgpt);
    void insert_fgpt_count_at(int idx, u32 fgpt, u32 &count);
    u32 remove_fgpt(u32 fgpt);
    u32 remove_fgpt_at(int idx);

    u32 get_fgpt_at(int idx);
    u32 get_entry_at(int idx);
    bool insert_entry(u32 entry);
    void reset_entry_at(int idx);

    u32 evict_fgpt_at(int idx, u32 &count);

    void increment_at(int idx);
    void decrement_at(int idx);
    void add_at(int idx, int d);
    void sub_at(int idx, int d);

    vector<vector<u32>> retrieve_all();
    void split_bucket(BucketCounter &dst, int sep_lvl);
    u32 occupancy();

    void dump_bucket();
    static u32 entry_from_fgpt(u32 fgpt);
};


struct SegmentCounter {
    vector<BucketCounter> buckets;
    SegmentCounter *overflow;
    int expansion_count;
    u8 fgpt_size;
    
    SegmentCounter(int num_buckets, int fgpt_size, int fgpt_per_bucket, int expansion__count) :
            expansion_count(expansion__count),
            fgpt_size(fgpt_size)
    {
        if (fgpt_size != 7 && fgpt_size != 15 && fgpt_size != 23)
            throw std::runtime_error("Fgpt size not supported");
        buckets = vector<BucketCounter>(num_buckets, fgpt_size);
        overflow = nullptr;
        for (int i=0; i<num_buckets; ++i) {
            buckets[i].initialize(fgpt_per_bucket, fgpt_size);
        }
    }

    u32 occupancy();
};


struct BitTrieCounter {
    SegmentCounter *ptr;

    BitTrieCounter *zero;
    BitTrieCounter *one;

    BitTrieCounter();
    ~BitTrieCounter();

    void insert(u64 str, u8 len, SegmentCounter* val);
    void clear(u64 str, u8 len);
    SegmentCounter *retrieve(u64 str);
    SegmentCounter *retrieve(u64 str, u32 &depth);
    void dump(std::string s);
    void dump();
};


struct BambooBaseCounter {
    int _num_segments;
    int _bucket_idx_len;
    int _fgpt_size;
    int _fgpt_per_bucket;
    int _seg_idx_base;
    int _offset;
    SpookyHash _h;
    u32 _seed;
    u32 _alt_seed;
    u32 _bucket_mask;

    u32 _chain_max = 500;

    /* statistics */
    struct {
        int _time;
        int _counter0;
        int _counter1;
        int _expand_count;
        int _seg_find_cnt;

    } stats;

    BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base,
            u32 seed, u32 alt_seed);
    BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base, int offset);
    BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base, int offset,
            u32 seed, u32 alt_seed);
    virtual ~BambooBaseCounter();

    int count(int elt);
    virtual bool insert(int elt);
    virtual bool insert(int elt, u32 fgpt, u32 seg_idx, SegmentCounter *segment,
            u32 bidx1, u32 bidx2);
    bool remove(int elt);

    bool _cuckoo(SegmentCounter *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, 
            u32 fgpt, u32 fgpt_cnt, u32 chain_len);
    // u32 _find_segment_idx(u32 hash);
    bool _extract(int elt, u32 &fgpt, u32 &seg_idx, SegmentCounter *&segment,
            u32 &bidx1, u32 &bidx2); 

    inline u32 _alt_bucket(u32 fgpt, u32 bidx)
    {
        u32 alt = fgpt;
        while (true) {
            alt = (_h.Hash32(&alt, 4, _alt_seed) >> _offset) & _bucket_mask;
            if (alt)
                break;
        }
        return (bidx ^ alt) & _bucket_mask;
    }
    inline u32 _compute_hash(int elt)
    {
        return _h.Hash32(&elt, 4, _seed) >> _offset;
    }

    virtual SegmentCounter *_get_segment(u32 hash, u32 &seg_idx) = 0;
    virtual bool overflow(SegmentCounter *segment, u32 seg_idx, u32 bi_main, 
            u32 bi_alt, u32 fgpt, u32 fgpt_cnt) = 0;
    
    virtual u32 capacity() = 0;
    virtual u32 occupancy() = 0;
    void dump_info();
    void dump_percentage();
    virtual void dump_succinct() = 0;
};


struct CountingBamboo : BambooBaseCounter {
    BitTrieCounter *_trie_head;

    CountingBamboo(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    CountingBamboo(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base, 
            u32 seed, u32 alt_seed);
    ~CountingBamboo();

    void _initialize_segments();

    /* Returns the segment, and computes its index and stores it
     * in *seg_idx* */
    inline SegmentCounter *_get_segment(u32 hashfrag, u32 &seg_idx)
    {
        seg_idx = hashfrag;
        u32 depth = 0;
        SegmentCounter *s = _trie_head->retrieve(hashfrag, depth);
        seg_idx &= (1 << depth) - 1;
        return s;
    }

    bool overflow(SegmentCounter *segment, u32 seg_idx, u32 bi_main, 
            u32 bi_alt, u32 fgpt, u32 fgpt_cnt) override;

    u32 occupancy() override;
    u32 capacity() override;
    void dump_succinct() override;
};




#endif