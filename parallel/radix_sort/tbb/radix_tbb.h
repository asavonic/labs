#ifndef __RADIX_TBB_H
#define __RADIX_TBB_H

#include <array>
#include <functional>
#include <sorter.h>
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"

template <class T, std::size_t N >
class radix_tbb  : public sorter<T> {
    typedef sorter<T> parent_t;

    typedef typename radix_simple<T, N>::Tuint Tuint;
    std::vector<T> buffer;

    virtual void sort() {
        buffer.resize( this->data.size() );
    } 
    
};
#endif
