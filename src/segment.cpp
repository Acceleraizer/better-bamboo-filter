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
    u8 step = (_fgpt_size + 7) / 8;
    return  _bits[step*(idx+1)-1] >> 7;
}


/* returns index of first bit of fgpt*/
int Bucket::_vacant_idx()
{   
    u8 step = (_fgpt_size + 7) / 8;
    for (u32 idx = 0; idx <  _len/step; ++idx) {
        if (!_occupied_idx(idx) ) {
            return idx;
        }
    }
    return -1;
}


bool Bucket::check_fgpt(u32 fgpt, int idx)
{
    u32 stored_fgpt = get_fgpt_at(idx);
    return (fgpt) == (stored_fgpt);
}


/* returns logical index of fgpt*/
int Bucket::find_fgpt(u32 fgpt)
{
    u32 step = (_fgpt_size + 7) / 8;
    for (u32 idx = 0; idx < _len/step; ++idx) {
        if (check_fgpt(fgpt, idx)) {
            return idx;
        }
    }
    return -1;
}


int Bucket::count_fgpt(u32 fgpt)
{
    int cnt = 0;
    u32 step = (_fgpt_size + 7) / 8;

    for (u32 idx = 0; idx < _len/step; ++idx) {
        cnt += check_fgpt(fgpt, idx);
    } 
    return cnt;
}


void Bucket::insert_fgpt_at(int idx, u32 fgpt)
{
    u32 step = (_fgpt_size + 7) / 8;
    idx *= step;
    fgpt |= (1 << _fgpt_size); // Occupancy bit
    while (step--) {
        _bits[idx] = (u8) (fgpt % (1 << 8));
        fgpt >>= 8;
        ++idx;
    }
}


bool Bucket::insert_fgpt(u32 fgpt)
{
    int idx = _vacant_idx();
    if (idx == -1) {
        return false;
    }
    
    insert_fgpt_at(idx, fgpt);
    return true;
}


bool Bucket::remove_fgpt(u32 fgpt)
{
    int idx = find_fgpt(fgpt);
    if (idx == -1)
        return false;

    reset_fgpt_at(idx);
    return true;
}


/* idx is logical index */
void Bucket::reset_fgpt_at(int idx)
{
    u32 step = (_fgpt_size + 7) / 8;
    idx *= step;
    for (u32 i=0; i<step; ++i) {
        _bits[idx + i] = 0;
    }
}

/* idx is logical index */
u32 Bucket::get_entry_at(int idx)
{
    u32 stored_entry = 0;
    u32 step = (_fgpt_size + 7) / 8;
    idx *= step;
    for (u32 i=0; i<step; ++i) {
        stored_entry += (_bits[idx+i] << (8*i));
    }
    return stored_entry;
}


/* idx is logical index */
u32 Bucket::get_fgpt_at(int idx)
{
    /* Exclude the occupancy bit */
    return get_entry_at(idx) & ((1 << _fgpt_size) - 1);
}


/* idx is logical index */
u32 Bucket::remove_fgpt_at(int idx)
{
    u32 stored_fgpt = get_fgpt_at(idx);    
    reset_fgpt_at(idx);
    return stored_fgpt;
}


void Bucket::split_bucket(Bucket &dst, int sep_lvl) 
{
    u32 mask = (1 << _fgpt_size) | (1 << sep_lvl);
    u32 step = (_fgpt_size + 7) / 8;
    u32 entry;

    for (u32 idx = 0; idx < _len/step; ++idx) {
        entry = get_entry_at(idx);
        if((mask & entry) == mask) {
            reset_fgpt_at(idx);
            dst.insert_fgpt(entry);
        }
    }
    // cout << " " << occupancy(_fgpt_size) << "+" << dst.occupancy(_fgpt_size) << flush;
}


vector<u32> Bucket::retrieve_all()
{
    vector<u32> result;
    u32 step = (_fgpt_size + 7) / 8;
    u32 fgpt;
    for (u32 idx = 0; idx < _len/step; ++idx) {
        if ((fgpt = get_fgpt_at(idx)))
            result.push_back(fgpt);
    }
    return result;
}


u32 Bucket::occupancy()
{
    u32 cnt = 0;
    u32 step = (_fgpt_size + 7) / 8;
    for (u32 idx = 0; idx <  _len/step; ++idx) {
        cnt += _occupied_idx(idx);
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