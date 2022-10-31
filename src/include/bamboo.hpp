#include "vector"
#include "unordered_map"

using std::vector, std::unordered_map;


struct Bucket {
    vector<bool> bits;

    Bucket(int capacity)
    {
        bits = vector<bool>(capacity);
    }
};


struct Segment {
    vector<Bucket> buckets; 

    Segment(int num_buckets, int fpgt_size, int fgpt_per_bucket)
    {
        buckets = vector<Bucket>(num_buckets, Bucket(fpgt_size * fgpt_per_bucket));
    }
};


struct Bamboo {
    unordered_map<int, Segment*> segments;
    int _num_segments;
    int _buckets_per_segment;
    int _fgpt_size;
    int _fgpt_per_bucket;

    Bamboo(int num_segments, int buckets_per_segment, int fgpt_size, 
            int fgpt_per_bucket);
    ~Bamboo();

    int count(int elt);
    void insert(int elt);
    void remove(int elt);
    void adjust_to(int elt, int cnt);
};


struct CountingBamboo {
    vector<Bamboo*> bamboo_layers;
    int _depth;
    int _base_expn;


    CountingBamboo(int base_expn, vector<int> num_segments, vector<int> buckets_per_segment,
            vector<int> fgpt_size, vector<int> fgpt_per_bucket);
    ~CountingBamboo();

    int count(int elt);
    void increment(int elt);
    void decrement(int elt);  
};