#include <random>
#include <time.h>
#include "include/bamboo.hpp"



/* Bamboo Implementation */

/* Constructor. Seeds the hash functions and initializes the segments */
Bamboo::Bamboo(int num__segments, int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base) 
{
    cout << "Creating bamboo with " << _num_segments << " _segments" << endl; 
    for (int idx = 0; idx < _num_segments; ++idx) {
        _segments[idx] = new Segment(1 << bucket_idx_len, fgpt_size, fgpt_per_bucket, 0);
    }

    srand(time(NULL));
    _seed = rand();
    _alt_seed = rand();
    cout << "Seed = " << _seed << endl;
    cout << "Alt bucket seed = " << _alt_seed << endl;
}


Bamboo::~Bamboo()
{
    for (auto it = _segments.begin(); it != _segments.end(); ++it) {
        delete it->second;
    }
}


/* Counts the number of copies of *elt* in the filter. */
int Bamboo::count(int elt)
{
    u32 hash = _compute_hash(elt);
    Segment *segment = _find_segment(hash);
    if (!segment) {
        std::cerr << "Segment not found" << std::endl;
        return false;
    }

    u8 fgpt;
    u32 bidx1, bidx2;
    _extract(hash, fgpt, bidx1, bidx2);

    return segment->buckets[bidx1].count_fgpt(fgpt) 
            + segment->buckets[bidx2].count_fgpt(fgpt); 
}


/* Insert an element into the Bamboo filter. 
 * Will call cuckoo-insertion if both buckets of elt are full*/
bool Bamboo::insert(int elt)
{
    u32 hash = _compute_hash(elt);
    u32 seg_idx = _find_segment_idx(hash);
    if (seg_idx == (u32) -1) {
        std::cerr << "Segment not found" << std::endl;
        return false;
    }
    Segment *segment = _segments[seg_idx];
    
    u8 fgpt;
    u32 bidx1, bidx2;
    _extract(hash, fgpt, bidx1, bidx2);
    if (segment->buckets[bidx1].insert_fgpt(fgpt))
        return true;
    if (segment->buckets[bidx2].insert_fgpt(fgpt))
        return true;

    /* Both buckets are full - boot an element */
    cout << "Initial insertion for fingerprint " << bitset<8>(fgpt) << " failed" << endl;
    
    /* cuckoo chaining failed - must expand segment*/
    if (!_cuckoo(segment, bidx1, bidx2, fgpt, 0)) {
        return expand(seg_idx);
    } else {
        return true;
    }
}


/* Main cuckoo-insertion logic */
bool Bamboo::_cuckoo(Segment *segment, u32 bi_main, u32 bi_alt, u8 fgpt, 
        u32 chain_len)
{
    if (chain_len == _chain_max) {
        cout << "Chain max " << _chain_max << " reached, attempt to expand segment" << endl;
        return false;
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
    throw std::exception("Bucket capacity reached");

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

    return _cuckoo(segment, evict_bidx, alt_bidx, evict_fgpt, chain_len+1);
    
}

/* Expands the filter by adding a new segment and relocating fingerprints
 * based on "partial-key linear hashing". */
bool Bamboo::expand(u32 seg_idx)
{
    int expansion_count = ++_segments[seg_idx]->expansion_count;
    if (expansion_count >= _fgpt_size)
        throw std::exception("Bamboo max expansion capacity breached");
    
    u32 new_idx = seg_idx | (1<<(expansion_count+_seg_idx_base));

    cout << "Expanding segment " << seg_idx << " into segment " << new_idx; 

    _segments[new_idx] = new Segment(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, expansion_count);

    for (int i = 0; i < (1 << _bucket_idx_len); i++) 
        _segments[seg_idx]->buckets[i].split_bucket(_segments[new_idx]->buckets[i], expansion_count);
    return true;
}

/* Removes a copy of *elt* from the filter */
bool Bamboo::remove(int elt)
{
    u32 hash = _compute_hash(elt);
    Segment *segment = _find_segment(hash);
    if (!segment) {
        std::cerr << "Segment not found" << std::endl;
        return false;
    }

    u8 fgpt;
    u32 bidx1, bidx2;
    _extract(hash, fgpt, bidx1, bidx2);
    return segment->buckets[bidx1].remove_fgpt(fgpt)
            || segment->buckets[bidx2].remove_fgpt(fgpt);
}


/* Inserts or removes an elt until it has the desired *cnt*. */
void Bamboo::adjust_to(int elt, int cnt)
{

}


/* Finds the index of segment that stores a fingerprint associated with *hash*. */
u32 Bamboo::_find_segment_idx(u32 hash)
{
    /* we do linear search for now yea */
    /* find the largest segment length that hashes to a valid segment*/
    hash >>= _bucket_idx_len;
    for (u32 mask = (1<<(_seg_idx_base+_fgpt_size)) - 1; 
            mask != 0; 
            mask >>= 1) {
        // cout << mask << " " << (hash & mask) << endl;
        if (_segments[hash & mask])
            return hash & mask;
    }
    return -1;
}


/* Finds the segment that stores a fingerprint associated with *hash*. */
Segment *Bamboo::_find_segment(u32 hash) 
{
    u32 seg_idx = +_find_segment_idx(hash);
    if (seg_idx == (u32) -1)
        return nullptr;
    return _segments[seg_idx];
}


/* Writes the fingerprints and bucket indices from the hash into the
 * respective variables passed into the function. */
void Bamboo::_extract(u32 hash, u8 &fgpt, u32 &bidx1, u32 &bidx2)
{
    u32 bucket_mask = (1<<_bucket_idx_len)-1;
    fgpt = (hash >> (_bucket_idx_len + _seg_idx_base)) & ((1<<7) - 1);
    bidx1 = hash & bucket_mask;
    bidx2 = _alt_bucket(fgpt, bidx1);
    // cout << "Extract : " << bitset<32>(hash) << " -> " 
    //     << bitset<8>(fgpt) << " " 
    //     << bitset<32>(bidx1) << " " 
    //     << bitset<32>(bidx2) << endl;
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

