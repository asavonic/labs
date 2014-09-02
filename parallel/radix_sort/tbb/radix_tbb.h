#ifndef __RADIX_TBB_H
#define __RADIX_TBB_H

#include <array>
#include <functional>
#include <sorter.h>
#include "tbb/parallel_reduce.h"
#include "tbb/parallel_scan.h"
#include "tbb/blocked_range.h"

template <class T, std::size_t N >
class radix_tbb  : public sorter<T> {
    typedef sorter<T> parent_t;

    typedef typename radix_simple<T, N>::Tuint Tuint;
    std::vector<T> buffer;


    class radix_scan_tbb {
    public:
        radix_scan_tbb( Tuint* _src_array, Tuint* _dst_array, size_t _array_size, Tuint _bit)
            : src_array( _src_array ), dst_array( _dst_array ), array_size( _array_size), l_bound(0), r_bound(0), bit( _bit ){}


        bool order(const Tuint x, const Tuint bit)
        {
            return !!(((x >> 1) ^ x) & bit);
        }

        void operator()( const tbb::blocked_range<int>& r, tbb::pre_scan_tag )
        {
            for(int i = r.begin(); i < r.end(); ++i)
            {
                if ( order( src_array[i], bit ) )
                    ++r_bound;
                else
                    ++l_bound;
            }
        }

        void operator()( const tbb::blocked_range<int>& r, tbb::final_scan_tag)
        {
            size_t k = 0;
            int m = 0;

            for(int i = r.begin(); i < r.end(); ++i )
            {
                if ( order( src_array[i], bit ) )
                {
                    dst_array[ array_size - r_bound - 1 ] = src_array[i];
                    ++r_bound;
                }
                else
                {
                    dst_array[ l_bound++ ] = src_array[i];
                }
            }
        }

        radix_scan_tbb( radix_scan_tbb& b, tbb::split ) 
                    : src_array( b.src_array ), dst_array( b.dst_array ), array_size( b.array_size ), l_bound(0), r_bound(0), bit( b.bit ){}

        void reverse_join( radix_scan_tbb& a )
        {
            l_bound = a.l_bound + l_bound;
            r_bound = a.r_bound + r_bound;
        }

        void assign( radix_scan_tbb& b ) {l_bound = b.l_bound; r_bound = b.r_bound;}

    private:
        Tuint* src_array;
        Tuint* dst_array;
        size_t array_size;

        Tuint bit;

        size_t l_bound;
        size_t r_bound;
    }; 

    virtual void sort() {
        buffer.resize( this->data.size() );
        for ( size_t bit_num = 1; bit_num < sizeof( Tuint ) * 8; bit_num++ ) {
            std::cout << "processing " << bit_num << std::endl;
            radix_scan_tbb body( reinterpret_cast<Tuint*>( &parent_t::data.front() ), reinterpret_cast<Tuint*>( &buffer.front() ), buffer.size(), 1 << bit_num );
            tbb::parallel_scan( tbb::blocked_range<int>(0, buffer.size(), 4 ), body);
            std::swap( this->data, this->buffer );
        }
        std::swap( this->data, this->buffer );
    } 
    
};
#endif
