#include <iostream>
#include <vector>

using namespace std;

int main()
{
    vector<int> v(10);

    for(auto &i: v)
        i = 0;


    for(auto &i: v)
        cout << i << endl;
}

