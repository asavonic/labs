#ifndef __SORTER_H
#define __SORTER_H

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <bitset>

template <class T>
class sorter {
public:
    virtual ~sorter() {}

    sorter() {}

    sorter( std::vector<T>&& in_data ) {
        data = in_data;
    }

    sorter( std::vector<T>& in_data ) {
        data = in_data;
    }

    std::vector<T> data;
    std::chrono::milliseconds time_spent;

    virtual void sort() = 0;

    virtual void run() {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        this->sort(); 

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    virtual void read_from_file( std::string filepath ) {
        std::ifstream file( filepath.c_str(), std::ios::trunc | std::ios::in );

        T buf;
        while ( file >> buf ) {
            data.push_back( buf );
        }
    }

    virtual void write_to_file( std::string filepath ) {
        std::ofstream file( filepath.c_str(), std::ios::trunc | std::ios::out );

        for( size_t i = 0; i < data.size(); i++ ) {
            //file << std::bitset< sizeof(T) * 8 >( data[i] ) << std::endl;
            file << data[i] << std::endl;
        }
    }
};

#endif
