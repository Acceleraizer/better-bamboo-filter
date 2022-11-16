#include "include/bamboo.hpp"

/* bucket implementations: assuming 7, 15, 23 or 31 bit fingerprints*/

int TEST_ENDIANNESS = 1;
int CBBF_LITTLE_ENDIAN;


void Bucket::initialize(int capacity, int fgpt_size)
{
    _len = capacity * ((fgpt_size + 7) /8);
    _bits = new u8[_len]();
    // cout << "create " << (void*) _bits << endl;
}


// void Bucket::_flip_if_big_endian(u32 &fgpt) 
// {
//     if (CBBF_LITTLE_ENDIAN)
//         return;
    
//     u8 *ptr = (u8*) &fgpt;
//     *ptr ^= *(ptr + 3);
//     *(ptr + 3) ^= *ptr; 
//     *ptr ^= *(ptr + 3);
//     *(ptr + 1) ^= *(ptr + 2);
//     *(ptr + 2) ^= *(ptr + 1);
//     *(ptr + 1) ^= *(ptr + 2);
// }

bool Bucket::_occupied_idx(int idx, u8 fgpt_size)
{
    u8 step = (fgpt_size + 7) / 8;
    return (1 << 7) & _bits[idx+step-1];
}


/* returns index of first bit of fgpt*/
int Bucket::_vacant_idx(u8 fgpt_size)
{   
    u8 step = (fgpt_size + 7) / 8;
    for (u32 idx = 0; idx <  _len; idx += step) {
        if (!_occupied_idx(idx, fgpt_size) ) {
            return idx;
        }
    }
    return -1;
}


bool Bucket::check_fgpt(u32 fgpt, int idx, u8 fgpt_size)
{
    u32 stored_fgpt = get_fgpt_at(idx, fgpt_size);
    return (fgpt) == (stored_fgpt);
}


/* returns index of first bit of fgpt*/
int Bucket::find_fgpt(u32 fgpt, u8 fgpt_size)
{
    u32 step = (fgpt_size + 7) / 8;
    for (u32 idx = 0; idx < _len; idx += step) {
        if (check_fgpt(fgpt, idx, fgpt_size)) {
            return idx;

        }
    }
    return -1;
}

int Bucket::count_fgpt(u32 fgpt, u8 fgpt_size)
{
    int cnt = 0;
    u32 step = (fgpt_size + 7) / 8;
    for (u32 idx = 0; idx < _len; idx += step) {
        cnt += check_fgpt(fgpt, idx, fgpt_size);
    } 
    return cnt;
}

bool Bucket::insert_fgpt(u32 fgpt, u8 fgpt_size)
{
    int idx = _vacant_idx(fgpt_size);
    if (idx == -1) {
        return false;
    }
    
    insert_fgpt_at(idx, fgpt, fgpt_size);
    return true;
}

bool Bucket::remove_fgpt(u32 fgpt, u8 fgpt_size)
{
    int idx = find_fgpt(fgpt, fgpt_size);
    if (idx == -1)
        return false;

    reset_fgpt_at(idx, fgpt_size);
    return true;
}


void Bucket::reset_fgpt_at(int idx, u8 fgpt_size)
{
    u32 step = (fgpt_size + 7) / 8;
    for (u32 i=0; i<step; ++i) {
        _bits[idx + i] = 0;
    }
}


u32 Bucket::get_fgpt_at(int idx, u8 fgpt_size)
{
    u32 stored_fgpt = 0;
    u32 step = (fgpt_size + 7) / 8;
    for (u32 i=0; i<step; ++i) {
        stored_fgpt += _bits[idx+i] << (8*i);
    }
    // cout << "retrieved: " << bitset<16>(stored_fgpt) << endl; 
    stored_fgpt &= ((1 << fgpt_size) - 1);
    return stored_fgpt;
}


u32 Bucket::remove_fgpt_at(int idx, u8 fgpt_size)
{
    u32 stored_fgpt = get_fgpt_at(idx, fgpt_size);    
    reset_fgpt_at(idx, fgpt_size);
    return stored_fgpt;
}


void Bucket::insert_fgpt_at(int idx, u32 fgpt, u8 fgpt_size)
{
    u32 step = (fgpt_size + 7) / 8;

    fgpt |= (1 << fgpt_size); // occ bit
    // cout << "insert " << bitset<32>(fgpt) << " step " << step << " --> ";
    while (step--) {
        _bits[idx] = (u8) fgpt;
        fgpt >>= 8;
        ++idx;
    }
    // cout << bitset<16>(get_fgpt_at(iidx, fgpt_size)) << endl;
}


void Bucket::split_bucket(Bucket &dst, int sep_lvl, u8 fgpt_size) 
{
    u32 mask = (1 << fgpt_size) | (1 << sep_lvl);
    u32 step = (fgpt_size + 7) / 8;
    u32 fgpt;
    for (u32 idx = 0; idx < _len; idx += step) {
        fgpt = get_fgpt_at(idx, fgpt_size);
        if(mask & fgpt) {
            reset_fgpt_at(idx, fgpt_size);
            dst.insert_fgpt(fgpt, fgpt_size);
        }
    }
}


vector<u32> Bucket::retrieve_all(u8 fgpt_size)
{
    vector<u32> result;
    u32 fgpt;
    for (u32 idx = 0; idx < _len; ++idx) {
        if ((fgpt = get_fgpt_at(idx, fgpt_size)))
            result.push_back(fgpt);
    }
    return result;
}


u32 Bucket::occupancy(u8 fgpt_size)
{
    u32 cnt = 0;
    u32 step = (fgpt_size + 7) / 8;
    for (u32 idx = 0; idx <  _len; idx += step) {
        cnt += _occupied_idx(idx, fgpt_size);
    }
    return cnt;
}


u32 Segment::occupancy()
{
    u32 cnt = 0;
    for (Bucket &b: buckets) {
        cnt += b.occupancy(fgpt_size);
    }
    return cnt;
}