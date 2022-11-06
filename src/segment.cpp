#include "include/bamboo.hpp"

/* bucket implementations: assuming 7 bit fingerprints*/

int Bucket::vacant_idx()
{
    u8 mask = 1<<7;
    for (u32 idx = 0; idx < _bits.size(); ++idx) {
        if (!(mask & _bits[idx]))
            return idx;
    }
    return -1;
}


bool Bucket::check_fgpt(u8 fgpt, int idx)
{
    u8 mask = (1<<7) - 1;
    return fgpt == (mask & _bits[idx]);
}


int Bucket::find_fgpt(u8 fgpt)
{
    for (u32 idx = 0; idx < _bits.size(); ++idx) {
        if (check_fgpt(fgpt, idx))
            return idx;
    }
    return -1;
}

int Bucket::count_fgpt(u8 fgpt)
{
    int cnt = 0;
    for (u32 idx = 0; idx < _bits.size(); ++idx) {
        cnt += check_fgpt(fgpt, idx);
    }
    return cnt;
}

bool Bucket::insert_fgpt(u8 fgpt)
{
    int idx = vacant_idx();
    if (idx == -1) {
        return false;
    }
    
    insert_fgpt_at(idx, fgpt);
    // cout << "Fingerprint " << bitset<8>(fgpt) <<  " added" << endl;
    return true;
}

bool Bucket::remove_fgpt(u8 fgpt)
{
    int idx = find_fgpt(fgpt);
    if (idx == -1)
        return false;

    _bits[idx] = 0;
    return true;
}

u8 Bucket::remove_fgpt_at(int idx)
{
    u8 fgpt = _bits[idx] & ((1<<7) - 1);
    _bits[idx] = 0;
    return fgpt;
}

void Bucket::insert_fgpt_at(int idx, u8 fgpt)
{
    _bits[idx] = fgpt;
    _bits[idx] |= (1<<7);
}


u32 Bucket::occupancy()
{
    u32 cnt = 0;
    for (u8 fgpt : _bits) {
        cnt += ((fgpt & (1<<7)) >> 7);
    }
    return cnt;
}






u32 Segment::occupancy()
{
    u32 cnt = 0;
    for (Bucket b: buckets) {
        cnt += b.occupancy();
    }
    return cnt;
}