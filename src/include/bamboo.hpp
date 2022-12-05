#ifndef BAMBOO
#define BAMBOO

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



struct Bucket {
    u8 *_bits;    /* Each fgpt is stored in a contiguous subarray along with
                   * a flag bit. See implementation for details */
    u16 _len;     /* Length of array allocated to *_bits* */
    u8 _fgpt_size;

    Bucket(u8 fgpt_size) {_fgpt_size = fgpt_size;}
    ~Bucket() { delete[] _bits; }

    /* Must be called after initialization to allocate the array.
     * This is here because I have yet to study copy-semantics, and
     * including this directly in the constructor gives issues. */
    void initialize(int capacity, int fgpt_size);

    bool _occupied_idx (int idx);
    int _vacant_idx();
    bool check_fgpt(u32 fgpt, int idx);
    /* Returns the index where the first fingerprint is found */
    int find_fgpt(u32 fgpt); 
    int count_fgpt(u32 fpgt);
    bool insert_fgpt(u32 fgpt);
    bool remove_fgpt(u32 fgpt);

    void reset_fgpt_at(int idx);
    u32 get_fgpt_at(int idx);
    u32 get_entry_at(int idx);
    u32 remove_fgpt_at(int idx);
    void insert_fgpt_at(int idx, u32 fgpt);

    vector<u32> retrieve_all();
    void split_bucket(Bucket &dst, int sep_lvl);

    void dump_bucket();

    u32 occupancy();
};

struct Segment {
    vector<Bucket> buckets;
    Segment *overflow;
    int expansion_count;
    u8 fgpt_size;
    
    Segment(int num_buckets, int fgpt_size, int fgpt_per_bucket, int expansion__count) :
            expansion_count(expansion__count),
            fgpt_size(fgpt_size)
    {
        if (fgpt_size != 7 && fgpt_size != 15 && fgpt_size != 23)
            throw std::runtime_error("Fgpt size not supported");
        buckets = vector<Bucket>(num_buckets, fgpt_size);
        overflow = nullptr;
        for (int i=0; i<num_buckets; ++i) {
            buckets[i].initialize(fgpt_per_bucket, fgpt_size);
        }
    }

    u32 occupancy();
};


struct BitTrie {
    Segment *ptr;

    BitTrie *zero;
    BitTrie *one;

    BitTrie();
    ~BitTrie();

    void insert(u64 str, u8 len, Segment* val);
    void clear(u64 str, u8 len);
    Segment *retrieve(u64 str);
    Segment *retrieve(u64 str, u32 &depth);
    void dump(std::string s);
    void dump();
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
        int _time;
        int _counter0;
        int _counter1;
        int _expand_count;
        int _seg_find_cnt;

    } stats;

    BambooBase(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    virtual ~BambooBase();

    int count(int elt);
    virtual bool insert(int elt);
    bool remove(int elt);

    void adjust_to(int elt, int cnt);
    bool _cuckoo(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, 
            u32 fgpt, u32 chain_len);
    // u32 _find_segment_idx(u32 hash);
    bool _extract(int elt, u32 &fgpt, u32 &seg_idx, Segment *&segment,
            u32 &bidx1, u32 &bidx2); 

    inline u32 _alt_bucket(u32 fgpt, u32 bidx)
    {
        return (bidx ^ _h.Hash32(&fgpt, 4, _alt_seed)) & _bucket_mask;
    }
    inline u32 _compute_hash(int elt)
    {
        return _h.Hash32(&elt, 4, _seed);
    }

    virtual Segment *_get_segment(u32 hash, u32 &seg_idx) = 0;
    virtual bool overflow(Segment *segment, u32 seg_idx, u32 bi_main, 
            u32 bi_alt, u32 fgpt) = 0;
};


struct Bamboo : BambooBase {
    // unordered_map<u32, Segment*> _segments;
    BitTrie *_trie_head;

    Bamboo(int bucket_idx_len, int fgpt_size, 
            int fgpt_per_bucket, int seg_idx_base);
    ~Bamboo();

    /* Returns the segment, and computes its index and stores it
     * in *seg_idx* */
    inline Segment *_get_segment(u32 hashfrag, u32 &seg_idx)
    {
        // auto t1 = std::chrono::high_resolution_clock::now();
        seg_idx = hashfrag;
        u32 depth = 0;
        Segment *s = _trie_head->retrieve(hashfrag, depth);
        seg_idx &= (1 << depth) - 1;
        // auto t2 = std::chrono::high_resolution_clock::now();
        // auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        // stats._time += ns.count();
        return s;
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

    inline Segment *_get_segment(u32 hash, u32 &seg_idx) override
    {
        // auto t1 = std::chrono::high_resolution_clock::now();
        u32 mask = (u32)-1;
        
        while ((hash & mask) >= _segments.size())
        {
            mask >>= 1;
        }
        seg_idx = hash & mask;
        Segment *s = _segments[seg_idx];

        // auto t2 = std::chrono::high_resolution_clock::now();
        // auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        // stats._time += ns.count();

        return s;
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