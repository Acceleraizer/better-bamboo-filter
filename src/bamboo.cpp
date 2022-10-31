#include "include/bamboo.hpp"



/* Bamboo Implementation */

Bamboo::Bamboo(int num_segments, int buckets_per_segment, int fgpt_size, int fgpt_per_bucket)
{
    _num_segments = num_segments;
    _buckets_per_segment = buckets_per_segment;
    _fgpt_size = fgpt_size;
    _fgpt_per_bucket = fgpt_per_bucket;
    
    for (int idx = 0; idx < num_segments; ++ idx) {
        segments[idx] = new Segment(buckets_per_segment, fgpt_size, fgpt_per_bucket);
    }
}


Bamboo::~Bamboo()
{
    for (auto it = segments.begin(); it != segments.end(); ++it)
    {
        delete it->second;
    }
}


void Bamboo::insert(int elt)
{

}


void Bamboo::remove(int elt)
{

}


void Bamboo::adjust_to(int elt, int cnt)
{

}





/* Counting Bamboo Implementation */


CountingBamboo::CountingBamboo(int base_expn, vector<int> num_segments, vector<int> buckets_per_segment,
        vector<int> fgpt_size, vector<int> fgpt_per_bucket)
{


}


CountingBamboo::~CountingBamboo()
{


}


int CountingBamboo::count(int elt) 
{
    int total = 0;
    int layer = 0;
    int place_val = 1;
    bool fail_cond = false;

    while(fail_cond) {
        total += place_val * bamboo_layers[layer]->count(elt);
        ++layer;
        place_val *= _base_expn;
    }

    return total;
}


void CountingBamboo::increment(int elt) 
{


}


void CountingBamboo::decrement(int elt)
{

}