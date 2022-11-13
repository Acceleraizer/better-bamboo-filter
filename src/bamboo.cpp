#include <random>
#include <time.h>
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
    cout << "Creating bamboo with " << _num_segments << " _segments" << endl; 
    srand(time(NULL));
    _seed = rand();
    _alt_seed = rand();
    cout << "Seed = " << _seed << endl;
    cout << "Alt bucket seed = " << _alt_seed << endl;
}

BambooBase::~BambooBase()
{
}

// Segment* BambooBase::_get_segment(u32 seg_idx) {}
// bool BambooBase::overflow(Segment *segment, u32 seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt) {}

Bamboo::Bamboo(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, int seg_idx_base) 
    : BambooBase(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base)
{
    for (int idx = 0; idx < _num_segments; ++idx)
        _segments[idx] = new Segment(1 << bucket_idx_len, fgpt_size, fgpt_per_bucket, 0);
}


Bamboo::~Bamboo()
{
    for (auto it = _segments.begin(); it != _segments.end(); ++it)
        delete it->second;
}

BambooOverflow::BambooOverflow(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, int seg_idx_base) 
    : BambooBase(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base)
{
    _expand_prompt = 1/2 * fgpt_per_bucket * (1 << bucket_idx_len);
    _insert_count = 0;
    _next_seg_idx = 0;
    _expand_base = 0;
    for (int idx = 0; idx < _num_segments; ++idx)
        _segments.push_back(new Segment(1 << bucket_idx_len, fgpt_size, fgpt_per_bucket, 0));
}

/* Counts the number of copies of *elt* in the filter. */
int BambooBase::count(int elt)
{
    u8 fgpt;
    u32 bidx1, bidx2, seg_idx;
    if (!_extract(elt, fgpt, seg_idx, bidx1, bidx2))
        return false;
    Segment *segment = _get_segment(seg_idx);
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
    u8 fgpt;
    u32 bidx1, bidx2, seg_idx;
    if (!_extract(elt, fgpt, seg_idx, bidx1, bidx2))
        return false;
    Segment *segment = _get_segment(seg_idx);
    return segment->buckets[bidx1].insert_fgpt(fgpt) 
            || segment->buckets[bidx2].insert_fgpt(fgpt)
            || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, 0); 
}

bool BambooOverflow::insert(int elt) 
{
    if(BambooBase::insert(elt))
    {
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
        return true;
    }
    return false;
}

/* Removes a copy of *elt* from the filter */
bool BambooBase::remove(int elt)
{
    u8 fgpt;
    u32 bidx1, bidx2, seg_idx;
    if (!_extract(elt, fgpt, seg_idx, bidx1, bidx2))
        return false;
    Segment *segment = _get_segment(seg_idx);
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
bool BambooBase::_cuckoo(Segment *segment, int seg_idx, u32 bi_main, u32 bi_alt, u8 fgpt, 
        u32 chain_len)
{
    if (chain_len == _chain_max) {
        cout << "Chain max " << _chain_max << " reached, attempt to expand segment" << endl;
        return overflow(segment, seg_idx, bi_main, bi_alt, fgpt);
    }

    int evict_bidx, evict_idx;
    u8 evict_fgpt = -1;

    /* Can fine-tune the eviction strategy ... */

    /* Try randomly evicting from the alt - hope that we pick a different 
     * fingerprint.*/
    evict_bidx = bi_alt;
    evict_idx = rand() % _fgpt_per_bucket;
    if (!segment->buckets[evict_bidx].check_fgpt(fgpt, evict_idx)) {
        evict_fgpt = segment->buckets[evict_bidx].remove_fgpt_at(evict_idx);
        goto evict;
    }

    /* Else check all elements for a different fingerprint to evict.
     * Prioritize the alt bucket to prevent bouncing. Roughly mirrors
     * the logic of the original 2014 Cuckoo filter paper. */
    for (int bidx : {bi_alt, bi_main}) {
        for (evict_idx = 0; evict_idx < _fgpt_per_bucket; ++evict_idx) {
            if (!segment->buckets[bidx].check_fgpt(fgpt, evict_idx)) {
                evict_fgpt = segment->buckets[bidx].remove_fgpt_at(evict_idx);
                evict_bidx = bidx;
                goto evict;
            }
        }
    }

    /* Both buckets are filled by the same fingerprint - nothing we can evict */
    cout << "Both buckets filled by the same fingerprint - cuckoo not possible" << endl;
    throw std::runtime_error("Bucket capacity reached");

evict:
    /* Insert the current fingerprint in the new vacancy */
    segment->buckets[evict_bidx].insert_fgpt_at(evict_idx, fgpt);
    /* Compute the other bucket for the evicted fingerprint */
    u32 alt_bidx = _alt_bucket(evict_fgpt, evict_bidx);
    
    // cout << "Removed fgpt " << bitset<8>(evict_fgpt) << " from "
    //     << evict_bidx << "(" << evict_idx << ")" << flush;    
    // cout << "  >>  alt bucket = " << alt_bidx << endl;

    /* Try to insert the evicted fingerprint in the alt bucket*/
    if (segment->buckets[alt_bidx].insert_fgpt(evict_fgpt)) {
        cout << "Chain: " << chain_len << endl;
        return true;
    }

    return _cuckoo(segment, seg_idx, evict_bidx, alt_bidx, evict_fgpt, chain_len+1);
}

/* Expands the filter by adding a new segment and relocating fingerprints
 * based on "partial-key linear hashing". */
bool Bamboo::overflow(Segment *segment, u32 seg_idx, u32 bidx1, u32 bidx2, u8 fgpt)
{
    int expansion_count = ++_segments[seg_idx]->expansion_count;
    if (expansion_count >= _fgpt_size) 
        throw std::runtime_error("Bamboo max expansion capacity breached");
    
    u32 new_idx = seg_idx | (1<<(expansion_count+_seg_idx_base));

    cout << "Expanding segment " << seg_idx << " into segment " << new_idx; 

    _segments[new_idx] = new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, expansion_count);

    for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        _segments[seg_idx]->buckets[i].split_bucket(_segments[new_idx]->buckets[i], expansion_count);
    
    if(1<<expansion_count & fgpt) {
        segment = _get_segment(new_idx);
        seg_idx = new_idx;
    }
    return segment->buckets[bidx1].insert_fgpt(fgpt) 
        || segment->buckets[bidx2].insert_fgpt(fgpt)
        || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, 0); 
}

bool BambooOverflow::overflow(Segment *segment, u32 seg_idx, u32 bidx1, u32 bidx2, u8 fgpt)
{
    if(!segment->overflow)
        segment->overflow = new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
    return _cuckoo(segment->overflow, seg_idx, bidx1, bidx2, fgpt, 0);
}

void BambooOverflow::expand(int seg_idx)
{
    Segment *base_seg = _segments[seg_idx];
    Segment *new_seg = new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
    for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        base_seg->buckets[i].split_bucket(new_seg->buckets[i], 0);

    Segment *overflow = base_seg->overflow;
    base_seg->overflow = nullptr;
    while(overflow)
    {
        for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        {   
            vector<u8> fgpts = overflow->buckets[i].retrieve_all();
            for(u8 fgpt : fgpts)
            {
                Segment *insert_segment = (1<<_expand_base) & fgpt ? new_seg : base_seg;
                u32 bidx2 = _alt_bucket(fgpt, i);
                insert_segment->buckets[i].insert_fgpt(fgpt)
                || insert_segment->buckets[bidx2].insert_fgpt(fgpt)
                || _cuckoo(insert_segment, seg_idx, i, bidx2, fgpt, 0);
            }
        }
        overflow = overflow->overflow;
    }
    _segments.push_back(new_seg);
}

/* Inserts or removes an elt until it has the desired *cnt*. */
void BambooBase::adjust_to(int elt, int cnt)
{

}


/* Finds the index of segment that stores a fingerprint associated with *hash*. */
u32 BambooBase::_find_segment_idx(u32 hash)
{
    /* we do linear search for now yea */
    /* find the largest segment length that hashes to a valid segment*/
    hash >>= _bucket_idx_len;
    for (u32 mask = (1<<(_seg_idx_base+_fgpt_size)) - 1; 
            mask != 0; 
            mask >>= 1) {
        // cout << mask << " " << (hash & mask) << endl;
        if (_get_segment(hash & mask))
            return hash & mask;
    }
    return -1;
}

/* Writes the fingerprints and bucket indices from the elt into the
 * respective variables passed into the function. */
bool BambooBase::_extract(int elt, u8 &fgpt, u32 &seg_idx, u32 &bidx1, u32 &bidx2)
{
    u32 hash = _compute_hash(elt);
    seg_idx = _find_segment_idx(hash);
    if (seg_idx == (u32) -1) {
        std::cerr << "Segment not found" << std::endl;
        return false;
    }
    u32 bucket_mask = (1<<_bucket_idx_len)-1;
    fgpt = (hash >> (_bucket_idx_len + _seg_idx_base)) & ((1<<7) - 1);
    bidx1 = hash & bucket_mask;
    bidx2 = _alt_bucket(fgpt, bidx1);
    return true;
}

/* Computes the number of fingerprints stored */
u32 Bamboo::occupancy()
{
    u32 cnt = 0;
    for (auto item : _segments) {
        if (!item.second)
            continue;
        cnt += item.second->occupancy();
    }
    return cnt;
}

/* Returns the total number of fingerprints that can be stored. */
u32 Bamboo::capacity()
{
    return _num_segments * (1 << _bucket_idx_len) * _fgpt_per_bucket;
}

