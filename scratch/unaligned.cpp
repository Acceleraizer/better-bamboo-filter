#include <vector>
#include <array>
#include <chrono>
#include <iostream>
#include <exception>
#include <string>
#include <cstring>
#include <bitset>

using std::memcpy, std::bitset;

using std::vector, std::array;
using std::cout, std::endl, std::cerr, std::flush;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;



struct Bucket {
    u8* arr_;
    u16 len_;


    Bucket(int numbits) 
    {
        len_ = (numbits+7)/8;
        arr_ = new u8[len_];
    }

    ~Bucket() 
    {
        delete[] arr_;
    }

    u64 read(int first, int d)
    {
        // dangerous for d > 56
        int start = first / 8;
        u64 ret = *((u64 *)(arr_ + start));
        ret >>= (first % 8);
        ret &= (1 << d) - 1;
        return ret;
    }

    void reset(int first, int d)
    {
        u64 mask = (1 << (first%8)) - 1;
        *(u64 *) (arr_ + first/8) &= mask;
    }

    void write(u64 data, int first, int d)
    {
        // dangerous for d > 56;
        data <<= (first % 8);
        reset(first, d);
        *(u64 *) (arr_ + first/8) |= data;
    }


    void dump()
    {
        for (int i=0; i<len_; ++i)
        {
            cout << bitset<8>(arr_[i]) << " ";
            if (((i+1) % 5) == 0) 
            {
                cout << " :" << i << endl; 
            }
        }
        cout << endl;
    }
};



int main() {
    /* check memory usage */
cout << "int:" << sizeof(int) << endl;

    {
        Bucket b(128);
        b.write(15, 1, 4);
        cout << b.read(1, 4) << endl;
        b.dump();

        b.write(1<<15, 3, 16);
        b.dump();
        cout << b.read(3, 16) << endl;

    }
}