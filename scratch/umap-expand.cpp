#include <unordered_map>
#include <random>
#include <iostream>

using namespace std;



int main() 
{
    int r;
    int a;
    unordered_map<int, int> test;
    vector<bool> unique(100000);

    int last_count = test.bucket_count();
    cout << "start size: " << last_count << endl;
    for (int i=0; i<10000; ++i) 
    {
        r = rand() % 100000;
        unique[r] = true;
        a = test[r];
        test.erase(r);
        if (last_count != test.bucket_count()) {
            last_count = test.bucket_count();
            int sum = 0;
            for (bool b: unique)
                sum += b;
            
            cout << "@ " << sum << ": " << last_count << endl;
        }
    }   

}