#include <random>
#include <time.h>
#include <exception>
#include "include/bamboo.hpp"


/* Counting Bamboo Implementation */


CountingBamboo::CountingBamboo(int max_depth, int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base, bool bamboo_implementation) :
            _seg_idx_base(seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _bamboo_implementation(bamboo_implementation)
{
    _depth = 1;
    if(_bamboo_implementation)
        bamboo_layers.push_back(new BambooOverflow(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    else
        bamboo_layers.push_back(new Bamboo(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
}

// CountingBamboo::CountingBamboo(int base_expn, vector<int> num_segments, 
//         vector<int> buckets_per_segment, 
//         vector<int> fgpt_size, vector<int> fgpt_per_bucket)
// {

// }

CountingBamboo::~CountingBamboo()
{
    // for (auto it = bamboo_layers.begin(); it != bamboo_layers.end(); ++it) {
    //     delete it->second;
    // }
}


int CountingBamboo::count(int elt) 
{
    u32 count = 0;
    for (int i = 0; i < _depth; i++)
        if (bamboo_layers[i]->count(elt))
            count |= 1<<i;
    return count;

    // int i = 0;

    // int total = 0;
    // int layer = 0;
    // int place_val = 1;
    // bool fail_cond = false;

    // while(fail_cond) {
    //     total += place_val * bamboo_layers[layer]->count(elt);
    //     ++layer;
    //     place_val *= _base_expn;
    // }

    // return total;
}


void CountingBamboo::increment(int elt) 
{
    for (int i = 0; i < _depth; i++) {
        if(!bamboo_layers[i]->remove(elt)) {
            bamboo_layers[i]->insert(elt);
            return;
        }
    }
    if (_depth == _max_depth)
        throw std::runtime_error("Max depth reached");
    if(_bamboo_implementation)
        bamboo_layers.push_back(new BambooOverflow(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    else
        bamboo_layers.push_back(new Bamboo(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    bamboo_layers[_depth]->insert(elt);
    _depth += 1;
}


void CountingBamboo::decrement(int elt)
{
    for (int i = 0; i < _depth; i++) {
        if(bamboo_layers[i]->remove(elt)) {
            for (int j = 0; j < i; j++)
                bamboo_layers[j]->insert(elt);
            return;
        }
    }
    throw std::runtime_error("Can't decrement, item not found in filter");
}