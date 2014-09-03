#ifndef __RADIX_TBB_H
#define __RADIX_TBB_H

#include <array>
#include <functional>
#include <sorter.h>
#include <bitset>
#include "tbb/parallel_reduce.h"
#include "tbb/parallel_scan.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/concurrent_vector.h"
#include <atomic>

template <class T, std::size_t N, size_t split_parts_count >
class radix_tbb  : public radix_simple<T, N> {
    using parent_t = radix_simple<T, N>;

    using Tuint = typename parent_t::Tuint;

    using part_t = std::pair<size_t, size_t>;

    
    std::vector<size_t> compute_counters_tbb() {
        // for merge_but_size = 4 we got 1111000000000...0
        constexpr Tuint merge_bitmask_shift = sizeof(Tuint) * 8 - merge_bit_size;
        constexpr Tuint merge_bitmask = ( ~0u >> merge_bitmask_shift ) << merge_bitmask_shift;

        std::vector<std::atomic_size_t> counters( merge_bit_size );

        tbb::parallel_for( tbb::blocked_range<size_t>( 0, parent_t::data.size() ), 
            [&]( const tbb::blocked_range<size_t>& r ) {
                for ( size_t i = r.begin(); i < r.end(); i++ ) {
                    Tuint* elem_ptr = reinterpret_cast<Tuint*>( &parent_t::data[i] );
                    size_t pos = *elem_ptr;
                    counters[ *elem_ptr >> merge_bitmask_shift ]++;
                }
            }
        );

        return counters;
    }

    std::vector<std::atomic_size_t> compute_offsets( std::vector<std::atomic_size_t> counters ) {
        std::vector<std::atomic_size_t> offsets( counters.size() );

        for ( size_t i = 1; i < offsets.size(); i++ ) {
            offsets[i] = offsets[i - 1]  + counters[i];
        }
         
        return offsets;
    }

    std::vector<part_t> split_data( const std::vector<T> data ) {
        assert( split_parts_count < data.size() );
        size_t part_size = data.size() / split_parts_count;

        std::vector<part_t> parts( split_parts_count );

        parts[0] = std::make_pair( 0, part_size );
        for ( size_t i = 1; i < parts.size(); i++ ) {
            parts[i].first = parts[i - 1].second;
            parts[i].first = parts[i].first + part_size;
        }
        parts.back().second = data.size();
        
        return parts;
    }
    
    virtual void sort() {
        auto parts = split_data( parent_t::data );
    }
};
#endif
