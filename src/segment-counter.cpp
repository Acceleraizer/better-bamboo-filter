#include "include/countingbamboo.hpp"
#include <cstring>
#include <random>

/* bucket implementations: assuming 7, 15, 23 or 31 bit fingerprints*/


void BucketCounter::initialize(int capacity, int fgpt_size)
{
    _entry_size = fgpt_size + 8;
    _step = (_entry_size + 7) / 8;
    _len = capacity * _step;
    _bits = new u8[_len]();
}

/* idx is logical index */
bool BucketCounter::_occupied_idx(int idx)
{
    return count_at(idx);
}


/* returns index of first bit of fgpt*/
int BucketCounter::_vacant_idx()
{   
    for (u32 idx = 0; idx <  _len/_step; ++idx) {
        if (!_occupied_idx(idx) ) {
            return idx;
        }
    }
    return -1;
}

u32 inline BucketCounter::count_at(int idx) 
{
    return _bits[idx*_step];
}


void inline BucketCounter::increment_at(int idx) 
{
    ++_bits[idx*_step];
}

void inline BucketCounter::decrement_at(int idx) 
{
    --_bits[idx*_step];
}

void inline BucketCounter::add_at(int idx, int d) 
{
    _bits[idx*_step] += d;
}

void inline BucketCounter::sub_at(int idx, int d) 
{
    _bits[idx*_step] -= d;
}

/* Returns the count at this idx corresponding to fgpt */
u32 BucketCounter::count_fgpt_at(u32 fgpt, int idx)
{
    u32 stored_fgpt = get_fgpt_at(idx);
    // if (fgpt == stored_fgpt) cout << "M";
    return (fgpt == stored_fgpt) ? count_at(idx) : 0;
}


int BucketCounter::count_fgpt(u32 fgpt)
{
    int cnt = 0;
    for (u32 idx = 0; idx < _len/_step; ++idx) {
        cnt += count_fgpt_at(fgpt, idx);
    } 
    return cnt;
}

/* returns logical index of fgpt*/
int BucketCounter::find_fgpt(u32 fgpt)
{
    for (u32 idx = 0; idx < _len/_step; ++idx) {
        if (count_fgpt_at(fgpt, idx)) {
            return idx;
        }
    }
    return -1;
}

/* Should be called on an empty index */
void BucketCounter::insert_fgpt_at(int idx, u32 fgpt)
{
    u32 cnt = 1;
    insert_fgpt_count_at(idx, fgpt, cnt);
}


/* Should be called on an empty index */
void BucketCounter::insert_fgpt_count_at(int idx, u32 fgpt, u32 &cnt)
{
    u32 entry = entry_from_fgpt(fgpt);
    for (int i=0; i<_step; ++i) {
        _bits[idx*_step+i] = (u8) (entry & ((1 << 8) - 1));
        entry >>= 8;
    }
    int d = std::min(cnt, (u32)(1 << 8) - 1);
    add_at(idx, d);
    cnt -= d;
}


u32 BucketCounter::insert_fgpt(u32 fgpt)
{
    u32 cnt = 1;
    return insert_fgpt_count(fgpt, cnt);
}


/* Returns the remaining count */
u32 BucketCounter::insert_fgpt_count(u32 fgpt, u32 &count)
{
    /* Try to update the count of existing fgpt*/
    u32 num, d;
    int idx;
    for (idx = 0; idx < _len/_step; ++idx) {
        num = count_fgpt_at(fgpt, idx);
        if (num > 0 && num < (1 << 8) - 1) {
            d = std::min(count, (1 << 8) - 1 - num);
            add_at(idx, d);
            count -= d;
            if (count == 0)
                return count;
        }
    }
    /* Else find empty slot*/
    while ((idx = _vacant_idx()) > -1 && count) {
        insert_fgpt_at(idx, fgpt);
        // cout << "ENTRY " << bitset<16>(get_entry_at(idx)) << " " << bitset<8>(fgpt) << "|";
        if (--count == 0)
            return count;
        d = std::min(count, (u32) (1 << 8) - 1);
        add_at(idx, d);
        count -= d;
        if (count == 0)
            return count;
    }
    return count;
}


bool BucketCounter::insert_entry(u32 entry)
{
    int idx = _vacant_idx();
    if (idx == -1) {
        return false;
    }
    for (u32 i=0; i<_step; ++i) {
        _bits[idx*_step + i] = (u8) (entry % (1 << 8));
        entry >>= 8;
    }
    return true;
}

/* idx is logical index */
u32 BucketCounter::remove_fgpt_at(int idx)
{
    u32 stored_fgpt = get_fgpt_at(idx); 
    u32 count = count_at(idx);
    if (count == 1) {
        reset_entry_at(idx);
    }   
    else
        decrement_at(idx);
    return stored_fgpt;
}


u32 BucketCounter::remove_fgpt(u32 fgpt)
{
    int idx = find_fgpt(fgpt);
    if (idx == -1)
        return false;

    return remove_fgpt_at(idx);
}


/* idx is logical index */
void BucketCounter::reset_entry_at(int idx)
{
    for (u32 i=0; i<_step; ++i) {
        _bits[idx*_step + i] = 0;
    }
}

u32 BucketCounter::evict_fgpt_at(int idx, u32 &count)
{
    u32 stored_fgpt = get_fgpt_at(idx); 
    count = count_at(idx);
    reset_entry_at(idx);
    return stored_fgpt;
}


/* idx is logical index */
u32 BucketCounter::get_entry_at(int idx)
{
    u32 stored_entry = 0;
    for (u32 i=0; i<_step; ++i) {
        stored_entry += (_bits[idx*_step+i] << (8*i));
    }
    return stored_entry;
}


/* idx is logical index */
u32 BucketCounter::get_fgpt_at(int idx)
{
    return get_entry_at(idx) >> 8;
}

u32 BucketCounter::entry_from_fgpt(u32 fgpt) 
{
    return fgpt << 8;
}


/* Moves entries if bit in fgpt is set */
void BucketCounter::split_bucket(BucketCounter &dst, int sep_lvl) 
{
    u32 mask = (1 << (sep_lvl));
    u32 entry, fgpt;

    for (u32 idx = 0; idx < _len/_step; ++idx) {
        entry = get_entry_at(idx);
        fgpt = get_fgpt_at(idx);
        if((mask & fgpt) == mask) {
            reset_entry_at(idx);
            dst.insert_entry(entry);
        }
    }
}


vector<vector<u32>> BucketCounter::retrieve_all()
{
    vector<vector<u32>> result;
    u32 fgpt, cnt;
    for (u32 idx = 0; idx < _len/_step; ++idx) {
        fgpt = get_fgpt_at(idx);
        cnt = count_at(idx);
        if (fgpt)
            result.push_back({fgpt, cnt});
    }
    return result;
}


u32 BucketCounter::occupancy()
{
    u32 cnt = 0;
    for (u32 idx = 0; idx <  _len/_step; ++idx) {
        cnt += _occupied_idx(idx);
    }
    return cnt;
}


void BucketCounter::dump_bucket()
{
    cout << "Bucket dump: ";
    u32 fgpt;
    for (int i=0; i<_len; ++i){
        fgpt = get_fgpt_at(i);
        cout << bitset<16>(fgpt) << " ";
    }
    cout << endl;
}




u32 SegmentCounter::occupancy()
{
    u32 cnt = 0;
    for (BucketCounter &b: buckets) {
        cnt += b.occupancy();
    }
    return cnt;
}


BitTrieCounter::BitTrieCounter()
{
    zero = nullptr;
    one = nullptr;
    ptr = nullptr;
}

BitTrieCounter::~BitTrieCounter()
{
    if (ptr) 
        delete ptr;
    delete zero;
    delete one;
}


/* The one's place of the bitstring tells us to go left or right */
void BitTrieCounter::insert(u64 str, u8 len, SegmentCounter *val)
{
    if (len == 0) {
        ptr = val;
        return;
    }
    BitTrieCounter *next;
    if (str % 2) {
        if (!one) {
            one = new BitTrieCounter();
        }
        next = one;
    }
    else {
        if (!zero) {
            zero = new BitTrieCounter();
        }
        next = zero;
    }
    next->insert(str >> 1, len - 1, val);
}


void BitTrieCounter::clear(u64 str, u8 len)
{
    if (len == 0) {
        ptr = nullptr;
        return;
    }
    BitTrieCounter *next = str % 2 ? one : zero;
    return next->clear(str >> 1, len - 1);
}


SegmentCounter *BitTrieCounter::retrieve(u64 str)
{
    if (!(zero || one))
        return ptr;
    BitTrieCounter *next = (str % 2) ? one : zero;
    return next->retrieve(str >> 1);
}

/* Variant where the depth of the segment is calculated */
SegmentCounter *BitTrieCounter::retrieve(u64 str, u32 &depth)
{
    if (!(zero || one))
        return ptr;
    ++depth;
    BitTrieCounter *next = str % 2 ? one : zero;
    return next->retrieve(str >> 1, depth);
}


void BitTrieCounter::dump()
{
    dump("");
}


void BitTrieCounter::dump(std::string s)
{
    if (ptr) 
        cout << s << " " << ptr << endl;
    if (zero)
        zero->dump("0"+s);
    if (one)
        one->dump("1"+s);
}



/* Bamboo Implementation */

/* Constructor. Seeds the hash functions and initializes the segments */
BambooBaseCounter::BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base),
            _offset(0)
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    _seed = rand();
    _alt_seed = rand();
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBaseCounter::BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base,
        u32 seed, u32 alt_seed) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base),
            _offset(0),
            _seed(seed),
            _alt_seed(alt_seed) 
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBaseCounter::BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base, int offset) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base),
            _offset(offset)
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    _seed = rand();
    _alt_seed = rand();
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBaseCounter::BambooBaseCounter(int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base, int offset,
        u32 seed, u32 alt_seed) : 
            _num_segments(1 << seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _seg_idx_base(seg_idx_base),
            _offset(offset),
            _seed(seed),
            _alt_seed(alt_seed) 
{
    _bucket_mask = (1<<_bucket_idx_len)-1;
    
    std::memset(&stats, 0, sizeof(stats));
}


BambooBaseCounter::~BambooBaseCounter()
{
}


int BambooBaseCounter::count(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    SegmentCounter *segment;
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

bool BambooBaseCounter::insert(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    bool r = false;
    SegmentCounter *segment;

    if (!_extract(elt, fgpt, seg_idx, segment, bidx1, bidx2))
        goto ret;
    r =  insert(elt, fgpt, seg_idx, segment, bidx1, bidx2);
ret:
    return r;
}

bool BambooBaseCounter::insert(int elt, u32 fgpt, u32 seg_idx, 
        SegmentCounter *segment, u32 bidx1, u32 bidx2)
{
    bool r = !segment->buckets[bidx1].insert_fgpt(fgpt) 
            || !segment->buckets[bidx2].insert_fgpt(fgpt)
            || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, 1, 1);
    return r;
}


bool BambooBaseCounter::remove(int elt)
{
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    SegmentCounter *segment;
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
bool BambooBaseCounter::_cuckoo(SegmentCounter *segment, u32 seg_idx, 
        u32 bi_main, u32 bi_alt, u32 fgpt, u32 fgpt_cnt, u32 chain_len)
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
    evict_bidx = bi_alt;
    evict_idx = rand() % _fgpt_per_bucket;
    if (!segment->buckets[evict_bidx].count_fgpt_at(fgpt, evict_idx)) {
        evict_fgpt = segment->buckets[evict_bidx]
            .evict_fgpt_at(evict_idx, evict_fgpt_cnt);
        goto evict;
    }

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
    cout << "This item is represented " << i << " times in the filter."
        << " Both buckets filled by the same fingerprint - cuckoo not possible" << endl;
    cout << "Params: " << segment << " " << seg_idx << " " << bi_main << " " << bi_alt 
        << " " << bitset<32>(fgpt) << " " << chain_len << endl;
    throw std::runtime_error("Bucket capacity reached");

evict:
    segment->buckets[evict_bidx].insert_fgpt_count_at(evict_idx, fgpt, fgpt_cnt);
    u32 alt_bidx = _alt_bucket(evict_fgpt, evict_bidx);
    
    if (!segment->buckets[alt_bidx].insert_fgpt_count(evict_fgpt, evict_fgpt_cnt)) {
        return true;
    }
    return _cuckoo(segment, seg_idx, evict_bidx, alt_bidx, evict_fgpt, 
            evict_fgpt_cnt, chain_len+1);
}



CountingBamboo::CountingBamboo(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, int seg_idx_base) 
    : BambooBaseCounter(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base)
{
    _initialize_segments();
}

CountingBamboo::CountingBamboo(int bucket_idx_len, int fgpt_size, int fgpt_per_bucket, 
        int seg_idx_base, u32 seed, u32 alt_seed) 
    : BambooBaseCounter(bucket_idx_len, fgpt_size, fgpt_per_bucket, seg_idx_base,
        seed, alt_seed)
{
    _initialize_segments();
}

void CountingBamboo::_initialize_segments()
{
    SegmentCounter *s;
    _trie_head = new BitTrieCounter();
    for (int idx = 0; idx < _num_segments; ++idx) {
        s = new SegmentCounter(1 << _bucket_idx_len, _fgpt_size, _fgpt_per_bucket, 0);
        _trie_head->insert(idx, _seg_idx_base, s);
    }
}

CountingBamboo::~CountingBamboo()
{
    delete _trie_head;
}



bool CountingBamboo::overflow(SegmentCounter *segment, u32 seg_idx, u32 bidx1, u32 bidx2, 
        u32 fgpt, u32 fgpt_cnt)
{
    ++stats._expand_count;
    int expansion_count = ++segment->expansion_count;
    if (expansion_count >= _fgpt_size) 
        throw std::runtime_error("Bamboo max expansion capacity breached");
    
    u8 ilen = expansion_count + _seg_idx_base;
    u32 new_idx = seg_idx | (1 << (ilen - 1));

    // cout << "Expanding segment " << bitset<16>(seg_idx) << " into segment " << bitset<16>(new_idx) 
    //     << " : Expansion count = " << expansion_count  
    //     << " ilen = " << (u32) ilen << " to insert " << bitset<16>(fgpt) << endl; 
 
    SegmentCounter *new_segment = new SegmentCounter(1 << _bucket_idx_len, _fgpt_size, 
        _fgpt_per_bucket, expansion_count);
    _trie_head->insert(new_idx, ilen, new_segment);
    _trie_head->insert(seg_idx, ilen, segment);
    _trie_head->clear(seg_idx, ilen-1);
    
    for (int i = 0; i < (1 << _bucket_idx_len); i++) {
        segment->buckets[i]
            .split_bucket(new_segment->buckets[i], expansion_count-1);
    }
    if (1<<(expansion_count - 1) & fgpt) {
        segment = new_segment;
        seg_idx = new_idx;
    }
    ++_num_segments;

    bool r = !segment->buckets[bidx1].insert_fgpt_count(fgpt, fgpt_cnt) 
        || !segment->buckets[bidx2].insert_fgpt_count(fgpt, fgpt_cnt)
        || _cuckoo(segment, seg_idx, bidx1, bidx2, fgpt, fgpt_cnt, 1);


    return r;
}


/* Writes the fingerprints and bucket indices from the elt into the
 * respective variables passed into the function. */
bool BambooBaseCounter::_extract(int elt, u32 &fgpt, u32 &seg_idx, 
        SegmentCounter *&segment, u32 &bidx1, u32 &bidx2)
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


u32 count_helper(BitTrieCounter *node)
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
u32 CountingBamboo::occupancy()
{
    return count_helper(_trie_head);
}

/* Returns the total number of fingerprints that can be stored. */
u32 CountingBamboo::capacity()
{
    return _num_segments * (1 << _bucket_idx_len) * _fgpt_per_bucket;
}

void BambooBaseCounter::dump_info()
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

void BambooBaseCounter::dump_percentage()
{
    double p = (double) occupancy() / capacity() * 100;
    cout << p << "% full" << endl;
}

void CountingBamboo::dump_succinct()
{
    cout << _num_segments << " ";
    double p = (double) occupancy() / capacity();
    cout << p << endl;
}