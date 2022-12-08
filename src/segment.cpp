#include "include/bamboo.hpp"

/* bucket implementations: assuming 7, 15, 23 or 31 bit fingerprints*/


void Bucket::initialize(int capacity, int fgpt_size)
{
    _len = capacity * ((_fgpt_size + 7) /8);
    _bits = new u8[_len]();
    _fgpt_size = fgpt_size;
}

/* idx is logical index */
bool Bucket::_occupied_idx(int idx)
{
    bool empty = true;
    for (u32 i = 0; i < _step; ++i) {
        empty = empty && (_bits[_step*idx + i] == 0);
    }
    return !empty;
}


/* returns index of first bit of fgpt*/
int Bucket::_vacant_idx()
{   
    for (u32 idx = 0; idx <  _len/_step; ++idx) {
        if (!_occupied_idx(idx) ) {
            return idx;
        }
    }
    return -1;
}

u32 inline Bucket::count_at(int idx) 
{
    if (!_occupied_idx(idx))
        return 0;
        
    return (_bits[idx*_step] % 2) + 1;
}


void inline Bucket::increment_at(int idx) 
{
    ++_bits[idx*_step];
}


void inline Bucket::decrement_at(int idx) 
{
    --_bits[idx*_step];
}


/* Returns the count at this idx corresponding to fgpt */
u32 Bucket::count_fgpt_at(u32 fgpt, int idx)
{
    u32 stored_fgpt = get_fgpt_at(idx);
    return (fgpt == stored_fgpt) ? count_at(idx) : 0;
}


int Bucket::count_fgpt(u32 fgpt)
{
    int cnt = 0;
    for (u32 idx = 0; idx < _len/_step; ++idx) {
        cnt += count_fgpt_at(fgpt, idx);
    } 
    return cnt;
}

/* returns logical index of fgpt*/
int Bucket::find_fgpt(u32 fgpt)
{
    for (u32 idx = 0; idx < _len/_step; ++idx) {
        if (count_fgpt_at(fgpt, idx)) {
            return idx;
        }
    }
    return -1;
}

/* Should be called on an empty index */
void Bucket::insert_fgpt_at(int idx, u32 fgpt)
{
    u32 cnt = 1;
    insert_fgpt_count_at(idx, fgpt, cnt);
}


/* Should be called on an empty index */
void Bucket::insert_fgpt_count_at(int idx, u32 fgpt, u32 &cnt)
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


u32 Bucket::insert_fgpt(u32 fgpt)
{
    u32 cnt = 1;
    return insert_fgpt_count(fgpt, cnt);
}


/* Returns the remaining count */
u32 Bucket::insert_fgpt_count(u32 fgpt, u32 &count)
{
    /* Try to update the count of existing fgpt*/
    u32 num;
    int idx;
    for (idx = 0; idx < _len/_fgpt_size; ++idx) {
        num = count_fgpt_at(fgpt, idx);
        if (num > 0 && num < 2) {
            increment_at(idx);
            --count;
            if (count == 0)
                return count;
        }
    }
    /* Else find empty slot*/
    while ((idx = _vacant_idx()) > -1 && count) {
        insert_fgpt_at(idx, fgpt);
        if (--count == 0)
            return count;
        increment_at(idx);
        if (--count == 0)
            return count;
    }
    return count;
}


bool Bucket::insert_entry(u32 entry)
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
u32 Bucket::remove_fgpt_at(int idx)
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


u32 Bucket::remove_fgpt(u32 fgpt)
{
    int idx = find_fgpt(fgpt);
    if (idx == -1)
        return false;

    return remove_fgpt_at(idx);
}


/* idx is logical index */
void Bucket::reset_entry_at(int idx)
{
    for (u32 i=0; i<_step; ++i) {
        _bits[idx*_step + i] = 0;
    }
}

u32 Bucket::evict_fgpt_at(int idx, u32 &count)
{
    u32 stored_fgpt = get_fgpt_at(idx); 
    count = count_at(idx);
    reset_entry_at(idx);
    return stored_fgpt;
}


/* idx is logical index */
u32 Bucket::get_entry_at(int idx)
{
    u32 stored_entry = 0;
    for (u32 i=0; i<_step; ++i) {
        stored_entry += (_bits[idx*_step+i] << (8*i));
    }
    return stored_entry;
}


/* idx is logical index */
u32 Bucket::get_fgpt_at(int idx)
{
    return get_entry_at(idx) >> 1;
}

u32 Bucket::entry_from_fgpt(u32 fgpt) 
{
    return fgpt << 1;
}


/* Moves entries if bit in fgpt is set */
void Bucket::split_bucket(Bucket &dst, int sep_lvl) 
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


vector<vector<u32>> Bucket::retrieve_all()
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


u32 Bucket::occupancy()
{
    u32 cnt = 0;
    for (u32 idx = 0; idx <  _len/_step; ++idx) {
        cnt += count_at(idx);
    }
    return cnt;
}


void Bucket::dump_bucket()
{
    cout << "Bucket dump: ";
    u32 fgpt;
    for (int i=0; i<_len; ++i){
        fgpt = get_fgpt_at(i);
        cout << bitset<16>(fgpt) << " ";
    }
    cout << endl;
}




u32 Segment::occupancy()
{
    u32 cnt = 0;
    for (Bucket &b: buckets) {
        cnt += b.occupancy();
    }
    return cnt;
}