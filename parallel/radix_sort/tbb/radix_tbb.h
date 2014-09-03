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
    using parts_vector_t = std::vector<part_t>;

    static constexpr size_t mask_values_array_size = std::pow( 2, N );
    using mask_values_t = std::array<size_t, mask_values_array_size>;

    using parts_counters_map_t = std::map< part_t, mask_values_t >;
    using parts_offsets_map_t = std::map< part_t, mask_values_t >;

    parts_counters_map_t compute_counters_tbb( std::vector<part_t> parts, size_t mask_step )
    {
        parts_counters_map_t total_parts_counters;

        tbb::parallel_for( tbb::blocked_range<size_t>( 0, parts.size() ), 
            [=,&total_parts_counters]( const tbb::blocked_range<size_t>& r ) {
                for ( size_t part_index = r.begin(); part_index < r.end(); part_index++ ) {
                    mask_values_t part_counters;
                    
                    for ( size_t i = parts[ part_index ].first, i_end = parts[ part_index  ].second; i < i_end; i++ ) {
                        Tuint* elem_ptr = reinterpret_cast<Tuint*>( &parent_t::data[i] );
                        size_t pos = *elem_ptr;
                        pos >>= N * mask_step;
                        pos &= ~( ( ~0u ) << N );
                        part_counters[ pos ]++;
                    }

                    total_parts_counters[ parts[ part_index ] ] = part_counters;
                }
            }
        );

        return total_parts_counters;
    }

    virtual void pass_tbb( parts_vector_t parts, parts_offsets_map_t& parts_offsets_map, size_t mask_step )
    {
        tbb::parallel_for( tbb::blocked_range<size_t>( 0, parts.size() ), 
            [&]( const tbb::blocked_range<size_t>& r ) {
                for ( size_t part_index = r.begin(); part_index < r.end(); part_index++ ) {
                    auto part_offsets = parts_offsets_map[ parts[ part_index ] ];
                    for ( size_t i = parts[ part_index ].first; i < parts[ part_index ].second; i++ ) {
                        Tuint* elem_ptr = reinterpret_cast<Tuint*>( &parent_t::data[i] );
                        size_t elem_uint = *elem_ptr;
                        elem_uint >>= N * mask_step;
                        elem_uint &= ~( ( ~0u ) << N );
                        size_t offset_index = elem_uint;

                        parent_t::buffer[ part_offsets[ offset_index ]++ ] = parent_t::data[i];
                    }
                }
            }
        );
    }

    parts_offsets_map_t compute_offsets_map( parts_vector_t parts,  parts_counters_map_t counters_map ) 
    {
        parts_offsets_map_t offsets_map = counters_map; // just a fast creation, or maybe not 

        for ( size_t counter_index = 0; counter_index < N; counter_index++ ) {
            // counters is a single possible * bitmask value *, for example: if we have N = 8, then these values can be from 0 to 256
            offsets_map[ parts.front() ].at( counter_index ) = ( counter_index == 0 ) ? 0 : 
                                offsets_map[ parts.back() ].at( counter_index - 1 ) + counters_map[ parts.back() ].at( counter_index - 1 );

            for ( size_t part_index = 1; part_index < parts.size(); part_index++ ) {
                offsets_map[ parts[ part_index ] ].at( counter_index ) = offsets_map[ parts[ part_index - 1 ] ].at( counter_index ) + counters_map[ parts[ part_index - 1 ] ].at( counter_index );
            }
        }
         
        return offsets_map;
    }

    parts_vector_t split_data( const std::vector<T> data ) {
        assert( split_parts_count < data.size() );
        size_t part_size = data.size() / split_parts_count;

        parts_vector_t parts( split_parts_count );

        parts[0] = std::make_pair( 0, part_size );
        for ( size_t i = 1; i < parts.size(); i++ ) {
            parts[i].first = parts[i - 1].second;
            parts[i].second = parts[i].first + part_size;
        }
        parts.back().second = data.size();
        
        return parts;
    }
    
    virtual void sort() {
        parent_t::buffer.resize( parent_t::data.size() );
        auto parts = split_data( parent_t::data );

        for ( size_t mask_step = 0; sizeof(T) * 8 > mask_step * N; mask_step++ ) {
            auto parts_counters_map = compute_counters_tbb( parts, mask_step );
            auto parts_offsets_map = compute_offsets_map( parts, parts_counters_map );
            //pass_tbb( parts, parts_offsets_map, mask_step );
        }
    }
};
#endif
