#include <random>
#include <time.h>
#include <exception>
#include "include/bamboo.hpp"


/* Counting Bamboo Implementation */


CountingBamboo::CountingBamboo(int max_depth, int bucket_idx_len, int fgpt_size, 
        int fgpt_per_bucket, int seg_idx_base, bool dif_hash) :
            _seg_idx_base(seg_idx_base),
            _bucket_idx_len(bucket_idx_len),
            _fgpt_size(fgpt_size),
            _fgpt_per_bucket(fgpt_per_bucket),
            _dif_hash(dif_hash)
{
    _depth = 0;
    _seed = rand();
    _alt_seed = rand();
    add_layer();
    // if(_bamboo_implementation)
        // bamboo_layers.push_back(new BambooOverflow(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    // else
    // bamboo_layers.push_back(new Bamboo(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
}



CountingBamboo::~CountingBamboo()
{
    for (auto it = bamboo_layers.begin(); it != bamboo_layers.end(); ++it) {
        delete *it;
    }
}


int CountingBamboo::count(int elt) 
{
    u32 count = 0;
    u32 layer_count;
    // cout << " <";
    for (int i = 0; i < _depth; i++) {
        if ((layer_count = bamboo_layers[i]->count(elt)))
            count += layer_count << i;
        // cout << layer_count << " ";
    }

    // cout << "> ";
    
    return count;
}


void CountingBamboo::increment(int elt) 
{
    /* Do naive implementation and then optimize later */
    u32 fgpt;
    u32 bidx1, bidx2, seg_idx;
    Segment *segment;
    int count;
    int idx = -1;
    for (int i = 0; i < _depth; i++) {
        // bamboo_layers[i]->_extract(elt, fgpt, seg_idx, segment, bidx1, bidx2);
        // count = 0;
        // if ((count = segment->buckets[bidx1].count_fgpt(fgpt))) {
        //     idx = bidx1;
        // }
        // else if ((count = segment->buckets[bidx2].count_fgpt(fgpt))) {
        //     idx = bidx2;
        // }

        // if (count == 0) {
        //     bamboo_layers[i]->insert(elt, fgpt, seg_idx, segment, bidx1, bidx2);
        //     return;
        // }
        // if (count == 1) {
        //     segment->buckets[idx].insert_fgpt(fgpt);
        //     return;
        // }
        // segment->buckets[idx].remove_fgpt(fgpt);

        count = bamboo_layers[i]->count(elt);
        if (count == 2) {
            bamboo_layers[i]->remove(elt);
        } else {
            bamboo_layers[i]->insert(elt);
            return;
        }
    }
    if (_depth == _max_depth)
        throw std::runtime_error("Max depth reached");

    // if(_bamboo_implementation)
    //     bamboo_layers.push_back(new BambooOverflow(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    // else
    // bamboo_layers.push_back(new Bamboo(_bucket_idx_len, _fgpt_size, _fgpt_per_bucket, _seg_idx_base));
    add_layer();
    bamboo_layers[_depth - 1]->insert(elt);
}


void CountingBamboo::decrement(int elt)
{
    /* Do naive implementation and then optimize later */
    int count;
    for (int i = 0; i < _depth; i++) {
        if (bamboo_layers[i]->count(elt) == 2) {
            bamboo_layers[i]->remove(elt);
            return;
        }
        if(i < _depth-1 && bamboo_layers[i+1]->count(elt)) {
            bamboo_layers[i]->insert(elt);
        }
        else {
            bamboo_layers[i]->remove(elt);
            return;
        }
    }
    throw std::runtime_error("Can't decrement, item not found in filter");
}


void CountingBamboo::add_layer()
{
    int bidxlen = _bucket_idx_len;
    int fgpt_size = _fgpt_size;
    int fgpt_pb = _fgpt_per_bucket;
    int segi_base = _seg_idx_base;

    if (_depth > 2) {
        bidxlen = 4;
        segi_base = 1;
    } 

    if (_dif_hash) {
        bamboo_layers.push_back(
            new Bamboo(bidxlen, fgpt_size, fgpt_pb, segi_base));
    } else {
        bamboo_layers.push_back(
            new Bamboo(bidxlen, fgpt_size, fgpt_pb, 
                segi_base, _seed, _alt_seed));
    }
    _depth += 1;
}


void CountingBamboo::dump_abacus()
{
    cout << "Dump Abacus Start ===" << endl;
    for (int i=0; i<_depth; ++i) {
        cout << "Layer " << i << ": " 
            << " Occ:"<< bamboo_layers[i]->occupancy() << endl;
        bamboo_layers[i]->dump_info();
        bamboo_layers[i]->dump_percentage();
    }
    cout << "Dump Abacus End ===" << endl;
}


u32 CountingBamboo::occupancy()
{
    u32 total = 0;
    for (int i=0; i<_depth; ++i)
    {
        total += bamboo_layers[i]->occupancy() << i;
    }
    return total;
}