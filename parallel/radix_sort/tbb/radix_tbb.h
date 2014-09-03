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
#include "tbb/concurrent_hash_map.h"
#include <atomic>

template <class T, std::size_t N, size_t split_parts_count >
class radix_tbb  : public radix_simple<T, N> {
    using parent_t = radix_simple<T, N>;

    using Tuint = typename parent_t::Tuint;

    using part_t = std::pair<size_t, size_t>;

    tbb::concurrent_hash_map< part_t, std::array<size_t, N> > compute_counters_tbb( std::vector<part_t> parts, size_t mask_step )
    {
        tbb::concurrent_hash_map< part_t, std::array<size_t, N> > total_parts_counters;

        tbb::parallel_for( tbb::blocked_range<size_t>( 0, parts.size() ), 
            [&]( const tbb::blocked_range<size_t>& r ) {
                for ( size_t part_index = r.begin(); part_index < r.end(); part_index++ ) {
                    std::array<size_t, N> part_counters;

                    for ( size_t i = parts[ part_index ].first; i < parts[ part_index ].second; i++ ) {
                        Tuint* elem_ptr = reinterpret_cast<Tuint*>( &parent_t::data[i] );
                        size_t pos = *elem_ptr;
                        pos >>= N * mask_step;
                        pos &= ~( ( ~0u ) << N );
                        part_counters[ pos ]++;
                    }

                    total_parts_counters.insert( part_counters, parts[ part_index ] );
                }
            }
        );

        return total_parts_counters;
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
        
        for ( size_t mask_step = 0; sizeof(T) * 8 > mask_step * N; mask_step++ ) {
            auto parts_counters = compute_counters_tbb( parts, mask_step );
        }
    
    }
};
#endif
