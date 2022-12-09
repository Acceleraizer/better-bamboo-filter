#include <random>
#include <time.h>
#include <chrono>
#include <cstring>
#include "include/bamboo.hpp"

/* Bamboo Implementation */

/* Constructor. Seeds the hash functions and initializes the segments */
BambooBase::BambooBase(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base) 
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    _seed = rand();
    _alt_seed = rand();
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBase::BambooBase(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base,
        u32 seed, u32 alt_seed) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base),
            _seed(seed),
            _alt_seed(alt_seed) 
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBase::~BambooBase()
{
}


Bamboo::Bamboo(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, int seg_idx_base) 
    : BambooBase(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base)
{
    _initialize_segments();
}


Bamboo::Bamboo(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, 
        int seg_idx_base, u32 seed, u32 alt_seed) 
    : BambooBase(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base,
        seed, alt_seed)
{
    _initialize_segments();
}


void Bamboo::_initialize_segments()
{
    Segment *s;
    _trie_head = new BitTrie();
    for (int idx = 0; idx < _num_segments; ++idx) {
        s = new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
        _trie_head->insert(idx, _seg_idx_base, s);
    }
}



Bamboo::~Bamboo()
{
    /* The BitTrie destructor takes care of the rest */
    delete _trie_head;
}


BambooOverflow::BambooOverflow(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base) :
    BambooBase(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base)
{
    _expand_prompt = fgpt_per_bucket * (1 << bucket_idx_len) * 1/2;
    _insert_count = 0;
    _next_seg_idx = 0;
    _expand_base = 0;
    for (int idx = 0; idx < _num_segments; ++idx)
        _segments.push_back(
            new Segment(1 << bucket_idx_len, fgpt_size, fgpt_per_bucket, 0));
}


BambooOverflow::~BambooOverflow()
{
    for (Segment *it : _segments)
        delete it;
}


/*
 * Key API
 */



/* Counts the number of copies of *elt* in the filter. */
int BambooBase::count(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    Segment *segment;
    if (!_extract(elt, fgpt, seg_idx, segment, bidx1, bidx2))
        return false;
    int count = 0;
    while (segment) 
    {
        count += segment->buckets[bidx1].count_fgpt(fgpt) 
            + segment->buckets[bidx2].count_fgpt(fgpt);
        segment = segment->overflow;
    }
    return count;
}

/* Insert an element into the Bamboo filter. 
 * Will call cuckoo-insertion if both buckets of elt are full*/
bool BambooBase::insert(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    bool r = false;
    Segment *segment;
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;
    std::chrono::_V2::system_clock::duration ns;

    t1 = std::chrono::high_resolution_clock::now();
    
    if (!_extract(elt, fgpt, seg_idx, segment, bidx1, bidx2))
        goto ret;
    
    // t2 = std::chrono::high_resolution_clock::now();
    // ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    // stats._time += ns.count();

    r =  insert(elt, fgpt, seg_idx, segment, bidx1, bidx2);
ret:
    // t2 = std::chrono::high_resolution_clock::now();
    // ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    // stats._time += ns.count();

    return r;
}

bool BambooBase::insert(int elt, u32 fgpt, u32 seg_idx, Segment *segment,
            u32 bidx1, u32 bidx2)
{
    bool r = !segment->buckets[bidx1].insert_fgpt(fgpt) 
            || !segment->buckets[bidx2].insert_fgpt(fgpt)
            || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, 1, 1);
    return r;
}


bool BambooOverflow::insert(int elt) 
{
    bool r = false;
    
    if(BambooBase::insert(elt))
    {
        // auto t1 = std::chrono::high_resolution_clock::now();
        _insert_count += 1;
        if(_insert_count == _expand_prompt)
        {
            _insert_count = 0;
            expand(_next_seg_idx);
            _next_seg_idx += 1;
            if(_next_seg_idx == 1<<(_expand_base+_seg_idx_base))
            {
                _next_seg_idx = 0;
                _expand_base += 1;
            }
        }
        r = true;
        // auto t2 = std::chrono::high_resolution_clock::now();
        // auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        // stats._time += ns.count();
    }

    
    return r;
}

/* Removes a copy of *elt* from the filter */
bool BambooBase::remove(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    Segment *segment;
    if (!_extract(elt, fgpt, seg_idx, segment, bidx1, bidx2))
        return false;
    while (segment)
    {
        if(segment->buckets[bidx1].remove_fgpt(fgpt)
            || segment->buckets[bidx2].remove_fgpt(fgpt))
            return true;
        segment = segment->overflow;
    }
    return false;
}


/* Main cuckoo-insertion logic */
bool BambooBase::_cuckoo(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, 
        u32 fgpt, u32 fgpt_cnt, u32 chain_len)
{
    ++stats._counter0;
    if (chain_len == _chain_max) {
        // cout << "Chain max " << _chain_max << " reached, attempt to expand segment" << endl;
        ++stats._counter1;
        return overflow(segment, seg_idx, bi_main, bi_alt, fgpt, fgpt_cnt);
    }

    int evict_bidx, evict_idx;
    u32 evict_fgpt, evict_fgpt_cnt;
    int i = 0;
    /* Can fine-tune the eviction strategy ... */

    /* Try randomly evicting from the alt - hope that we pick a different 
     * fingerprint.*/
    evict_bidx = bi_alt;
    evict_idx = rand() % _fgpt_per_bucket;
    if (!segment->buckets[evict_bidx].count_fgpt_at(fgpt, evict_idx)) {
        evict_fgpt = segment->buckets[evict_bidx]
            .evict_fgpt_at(evict_idx, evict_fgpt_cnt);
        goto evict;
    }

    /* Else check all elements for a different fingerprint to evict.
     * Prioritize the alt bucket to prevent bouncing. Roughly mirrors
     * the logic of the original 2014 Cuckoo filter paper. */
    for (int bidx : {bi_alt, bi_main}) {
        for (evict_idx = 0; evict_idx < _fgpt_per_bucket; ++evict_idx) {
            if (!segment->buckets[bidx].count_fgpt_at(fgpt, evict_idx)) {
                evict_fgpt = segment->buckets[bidx]
                    .evict_fgpt_at(evict_idx, evict_fgpt_cnt);
                evict_bidx = bidx;
                goto evict;
            }
            i += 1;
        }
    }

    /* Both buckets are filled by the same fingerprint - nothing we can evict */

    /* In the overflow segment case, this is probably allowed (?) */
    cout << "This item is represented " << i << " times in the filter."
        << " Both buckets filled by the same fingerprint - cuckoo not possible" << endl;
    cout << "Params: " << segment << " " << seg_idx << " " << bi_main << " " << bi_alt 
        << " " << bitset<32>(fgpt) << " " << chain_len << endl;
    throw std::runtime_error("Bucket capacity reached");

evict:
    /* Insert the current fingerprint in the new vacancy */
    segment->buckets[evict_bidx].insert_fgpt_count_at(evict_idx, fgpt, fgpt_cnt);
    /* Compute the other bucket for the evicted fingerprint */
    u32 alt_bidx = _alt_bucket(evict_fgpt, evict_bidx);
    
    /* Try to insert the evicted fingerprint in the alt bucket*/
    if (!segment->buckets[alt_bidx].insert_fgpt_count(evict_fgpt, evict_fgpt_cnt)) {
        // cout << "Chain: " << chain_len << endl;
        return true;
    }


    
    return _cuckoo(segment, seg_idx, evict_bidx, alt_bidx, evict_fgpt, 
            evict_fgpt_cnt, chain_len+1);
}

/* Expands the filter by adding a new segment and relocating fingerprints
 * based on "partial-key linear hashing". */
bool Bamboo::overflow(Segment *segment, u32 seg_idx, u32 bidx1, u32 bidx2, 
        u32 fgpt, u32 fgpt_cnt)
{
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;
    std::chrono::_V2::system_clock::duration ns;

    t1 = std::chrono::high_resolution_clock::now();

    ++stats._expand_count;
    int expansion_count = ++segment->expansion_count;
    if (expansion_count >= _fgpt_size) 
        throw std::runtime_error("Bamboo max expansion capacity breached");
    
    u8 ilen = expansion_count + _seg_idx_base;
    u32 new_idx = seg_idx | (1 << (ilen - 1));

    // cout << "Expanding segment " << bitset<16>(seg_idx) << " into segment " << bitset<16>(new_idx) 
    //     << " : Expansion count = " << expansion_count  
    //     << " ilen = " << (u32) ilen << " to insert " << bitset<16>(fgpt) << endl; 
    // cout << "Before expand: occupancy = " << occupancy() << endl;
    Segment *new_segment = new Segment(1 << _bucket_idx_len, _fgpt_size, 
        _fgpt_per_bucket, expansion_count);
    _trie_head->insert(new_idx, ilen, new_segment);
    _trie_head->insert(seg_idx, ilen, segment);
    _trie_head->clear(seg_idx, ilen-1);
    // _trie_head->dump("");
    
    for (int i = 0; i < (1 << _bucket_idx_len); i++) {
        segment->buckets[i]
            .split_bucket(new_segment->buckets[i], expansion_count-1);
    }
    if (1<<(expansion_count - 1) & fgpt) {
        segment = new_segment;
        seg_idx = new_idx;
    }
    // cout << "After expand: occupancy = " << occupancy() << endl;
    ++_num_segments;

    bool r = !segment->buckets[bidx1].insert_fgpt_count(fgpt, fgpt_cnt) 
        || !segment->buckets[bidx2].insert_fgpt_count(fgpt, fgpt_cnt)
        || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, fgpt_cnt, 1);

    t2 = std::chrono::high_resolution_clock::now();
    ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    stats._time += ns.count();

    return r;
}


bool BambooOverflow::overflow(Segment *segment, u32 seg_idx, u32 bidx1, 
        u32 bidx2, u32 fgpt, u32 fgpt_cnt)
{
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;
    std::chrono::_V2::system_clock::duration ns;

    t1 = std::chrono::high_resolution_clock::now();

    if(!segment->overflow)
        segment->overflow = 
            new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
    bool r = _cuckoo(segment->overflow, seg_idx, bidx1, bidx2, fgpt, 
            fgpt_cnt, 1);
    
    t2 = std::chrono::high_resolution_clock::now();
    ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    stats._time += ns.count();

    return r;
}


void BambooOverflow::expand(int seg_idx)
{
    std::chrono::_V2::high_resolution_clock::time_point t1,t2;
    std::chrono::_V2::system_clock::duration ns;

    t1 = std::chrono::high_resolution_clock::now();
    
    ++stats._expand_count;
    Segment *base_seg = _segments[seg_idx];
    Segment *new_seg = 
        new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
    for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        base_seg->buckets[i].split_bucket(new_seg->buckets[i], 0);

    Segment *overflow = base_seg->overflow;
    base_seg->overflow = nullptr;
    u32 fgpt, fgpt_cnt;
    while(overflow)
    {
        for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        {   
            vector<vector<u32>> fgpts = overflow->buckets[i].retrieve_all();
            for(vector<u32> data : fgpts)
            {
                fgpt = data[0];
                fgpt_cnt = data[1];
                Segment *insert_segment = (1<<_expand_base) & fgpt ? new_seg : base_seg;
                u32 bidx2 = _alt_bucket(fgpt, i);
                insert_segment->buckets[i].insert_fgpt_count(fgpt, fgpt_cnt)
                    || insert_segment->buckets[bidx2].insert_fgpt_count(fgpt, fgpt_cnt)
                    || _cuckoo(insert_segment, seg_idx, i, bidx2, fgpt, fgpt_cnt, 1);
            }
        }
        overflow = overflow->overflow;
    }
    _segments.push_back(new_seg);

    t2 = std::chrono::high_resolution_clock::now();
    ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    stats._time += ns.count();

}

/* Inserts or removes an elt until it has the desired *cnt*. */
void BambooBase::adjust_to(int elt, int cnt)
{

}


/* Writes the fingerprints and bucket indices from the elt into the
 * respective variables passed into the function. */
bool BambooBase::_extract(int elt, u32 &fgpt, u32 &seg_idx, Segment *&segment, 
        u32 &bidx1, u32 &bidx2)
{
    u32 hash = _compute_hash(elt);
    fgpt = (hash >> (_bucket_idx_len + _seg_idx_base)) & ((1<<_fgpt_size) - 1);
    
    /* Rehash if the fingerprint is all zeros */
    if (!fgpt) {
        return _extract(hash, fgpt, seg_idx, segment, bidx1, bidx2);
    }
    segment = _get_segment(hash >> _bucket_idx_len, seg_idx);
    bidx1 = hash & _bucket_mask;
    bidx2 = _alt_bucket(fgpt, bidx1);
    return true;
}


u32 count_helper(BitTrie *node)
{
    if (!node)
        return 0;
    u32 tot = 0;
    if (node->ptr)
        tot += node->ptr->occupancy();
    tot += count_helper(node->zero);
    tot += count_helper(node->one);

    return tot;
}


/* Computes the number of fingerprints stored */
u32 Bamboo::occupancy()
{
    return count_helper(_trie_head);
}

/* Returns the total number of fingerprints that can be stored. */
u32 Bamboo::capacity()
{
    return _num_segments * (1 << _bucket_idx_len) * _fgpt_per_bucket;
}


void Bamboo::dump_info()
{
    cout << "NS:" << _num_segments << flush;
    cout << " BIL:" << _bucket_idx_len << flush;
    cout << " FS:" << _fgpt_size << flush;
    cout << " FPB:" << _fgpt_per_bucket << flush;
    cout << " SIB:" << _seg_idx_base << flush;
    cout << " S:" << _seed << flush;
    cout << " AS:" << _alt_seed << flush;
    cout << endl;
}

void Bamboo::dump_percentage()
{
    double p = (double) occupancy() / capacity() * 100;
    cout << p << "% full" << endl;
}