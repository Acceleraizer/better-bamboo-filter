#include <random>
#include <time.h>
#include <exception>
#include "include/bamboo.hpp"


/* Counting Bamboo Implementation */


Abacus::Abacus(int max_depth, int bucket_idx_len, int fgpt_size, 
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
}



Abacus::~Abacus()
{
    for (auto it = bamboo_layers.begin(); it != bamboo_layers.end(); ++it) {
        delete *it;
    }
}


int Abacus::count(int elt) 
{
    u32 count = 0;
    u32 layer_count;
    // cout << " <";
    for (int i = 0; i < _depth; i++) {
        if ((layer_count = bamboo_layers[i]->count(elt)))
            count += layer_count << i;
        else
            break;
        // cout << layer_count << " ";
    }

    // cout << "> ";
    
    return count;
}


void Abacus::increment(int elt) 
{
    /* Do naive implementation and then optimize later */
    int count;
    for (int i = 0; i < _depth; i++) {
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

    add_layer();
    bamboo_layers[_depth - 1]->insert(elt);
}


void Abacus::decrement(int elt)
{
    /* Do naive implementation and then optimize later */
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


void Abacus::add_layer()
{
    int bidxlen = _bucket_idx_len;
    int fgpt_size = _fgpt_size;
    int fgpt_pb = _fgpt_per_bucket;
    int segi_base = _seg_idx_base;
    int offset = 0;
    if (_depth > 2) {
        // bidxlen = 2;
        // segi_base = 2;
        offset = _seg_idx_base - segi_base + _bucket_idx_len - bidxlen;
    } 

    if (_dif_hash) {
        bamboo_layers.push_back(
            new Bamboo(bidxlen, fgpt_size, fgpt_pb, segi_base, offset));
    } else {
        bamboo_layers.push_back(
            new Bamboo(bidxlen, fgpt_size, fgpt_pb, 
                segi_base, offset, _seed, _alt_seed));
    }
    _depth += 1;
}


void Abacus::dump_abacus()
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


u32 Abacus::occupancy()
{
    u32 total = 0;
    for (int i=0; i<_depth; ++i)
    {
        total += bamboo_layers[i]->occupancy() << i;
    }
    return total;
}