#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>

template <class T>
class sorter {
public:

    std::vector<T> array;
    std::chrono::milliseconds time_spent;

    sorter() {}

    virtual void sort() {
        std::sort( array.begin(), array.end() ) ;
    }
    virtual void run() {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        sort(); 

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    void read_from_file( std::string filepath ) {
        std::ifstream file( filepath.c_str(), std::ios::trunc | std::ios::in );

        T buf;
        while ( file >> buf ) {
            array.push_back( buf );
        }
    }

    void write_to_file( std::string filepath ) {
        std::ofstream file( filepath.c_str(), std::ios::trunc | std::ios::out );

        for( size_t i = 0; i < array.size(); i++ ) {
            file << array[i] << std::endl;
        }
    }
};
