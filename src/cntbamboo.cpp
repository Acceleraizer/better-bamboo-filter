#include <random>
#include <time.h>
#include "include/bamboo.hpp"


/* Counting Bamboo Implementation */


CountingBamboo::CountingBamboo(int base_expn, vector<int> num_segments, 
        vector<int> buckets_per_segment, 
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