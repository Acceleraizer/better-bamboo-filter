#include "include/bamboo.hpp"

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
    if (--cnt == 0)
        return;
    increment_at(idx);
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
            d = std::min(count, (1 << 8) - 1 - count);
            add_at(idx, d);
            count -= d;
            if (count == 0)
                return count;
        }
    }
    /* Else find empty slot*/
    while ((idx = _vacant_idx()) > -1 && count) {
        insert_fgpt_at(idx, fgpt);
        if (--count == 0)
            return count;
        d = std::min(count, (1 << 8) - 1 - count);
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
void BucketCounter::split_bucket(Bucket &dst, int sep_lvl) 
{
    u32 mask = (1 << (sep_lvl+1));
    u32 entry;

    for (u32 idx = 0; idx < _len/_step; ++idx) {
        entry = get_entry_at(idx);
        if((mask & entry) == mask) {
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


