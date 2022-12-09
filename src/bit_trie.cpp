#include "include/bamboo.hpp"


BitTrie::BitTrie()
{
    zero = nullptr;
    one = nullptr;
    ptr = nullptr;
}

BitTrie::~BitTrie()
{
    if (ptr) 
        delete ptr;
    delete zero;
    delete one;
}


/* The one's place of the bitstring tells us to go left or right */
void BitTrie::insert(u64 str, u8 len, Segment *val)
{
    if (len == 0) {
        ptr = val;
        return;
    }
    BitTrie *next;
    if (str % 2) {
        if (!one) {
            one = new BitTrie();
        }
        next = one;
    }
    else {
        if (!zero) {
            zero = new BitTrie();
        }
        next = zero;
    }
    next->insert(str >> 1, len - 1, val);
}


void BitTrie::clear(u64 str, u8 len)
{
    if (len == 0) {
        ptr = nullptr;
        return;
    }
    BitTrie *next = str % 2 ? one : zero;
    return next->clear(str >> 1, len - 1);
}


Segment *BitTrie::retrieve(u64 str)
{
    if (!(zero || one))
        return ptr;
    BitTrie *next = (str % 2) ? one : zero;
    return next->retrieve(str >> 1);
}

/* Variant where the depth of the segment is calculated */
Segment *BitTrie::retrieve(u64 str, u32 &depth)
{
    if (!(zero || one))
        return ptr;
    ++depth;
    BitTrie *next = str % 2 ? one : zero;
    return next->retrieve(str >> 1, depth);
}


void BitTrie::dump()
{
    dump("");
}


void BitTrie::dump(std::string s)
{
    if (ptr) 
        cout << s << " " << ptr << endl;
    if (zero)
        zero->dump("0"+s);
    if (one)
        one->dump("1"+s);
}

