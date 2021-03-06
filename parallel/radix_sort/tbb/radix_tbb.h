#ifndef __RADIX_TBB_H
#define __RADIX_TBB_H

#include <array>
#include <sorter.h>
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include <mutex>

template <class T, std::size_t N, size_t split_parts_count >
class radix_tbb  : public radix_simple<T, N> {
    using parent_t = radix_simple<T, N>;

    using Tuint = typename parent_t::Tuint;

    using part_t = std::pair<size_t, size_t>;
    using parts_vector_t = std::vector<part_t>;

    static constexpr Tuint MSB_mask = 1 << ( sizeof(Tuint) * 8 - 1 );

    static constexpr size_t mask_values_array_size = std::pow( 2, N );
    using mask_values_t = std::array<size_t, mask_values_array_size>;

    using parts_counters_map_t = std::map< part_t, mask_values_t >;
    using parts_offsets_map_t = std::map< part_t, mask_values_t >;

    parts_counters_map_t compute_counters_tbb( parts_vector_t parts, size_t mask_step )
    {
        parts_counters_map_t parts_counters_map;

        std::mutex map_lock;

        tbb::parallel_for( tbb::blocked_range<size_t>( 0, parts.size() ), 
            [&]( const tbb::blocked_range<size_t>& r ) {
                for ( size_t part_index = r.begin(); part_index < r.end(); part_index++ ) {
                    mask_values_t part_counters = { 0 };
                    
                    for ( size_t i = parts[ part_index ].first, i_end = parts[ part_index  ].second; i < i_end; i++ ) {
                        Tuint* elem_ptr = reinterpret_cast<Tuint*>( &parent_t::data[i] );
                        size_t pos = *elem_ptr;
                        pos >>= N * mask_step;
                        pos &= ~( ( ~0u ) << N );
                        part_counters[ pos ]++;
                    }

                    assert( part_index < parts.size() );
                    assert( part_index >= 0 );

                    std::lock_guard<std::mutex> locked( map_lock );
                    parts_counters_map[ parts[ part_index ] ] =  part_counters;
                }
            }
        );

        return parts_counters_map;
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
                        
                        assert( offset_index < part_offsets.size() );
                        assert( offset_index >= 0 );
                        assert( part_offsets[ offset_index  ] < parent_t::buffer.size() );
                        assert( part_offsets[ offset_index  ] >= 0  );

                        parent_t::buffer[ part_offsets[ offset_index ]++ ] = parent_t::data[i];
                    }
                }
            }
        );
    }

    parts_offsets_map_t compute_offsets_map( parts_vector_t parts,  parts_counters_map_t counters_map ) 
    {
        parts_offsets_map_t offsets_map = counters_map; // just a fast creation, or maybe not 

        for ( size_t counter_index = 0; counter_index < mask_values_array_size; counter_index++ ) {
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
    
    virtual size_t find_first_negative( std::vector<T>& data ) {
        size_t first_negative_index = 0;
        size_t index_delta = data.size();
        size_t index = data.size() - 1;

        Tuint* data_uint_ptr = reinterpret_cast<Tuint*>( data.data() );

        for ( index = 0; index < data.size(); index++ ) {
            if ( data[ index ] < 0 ) {
                return index;
            }
        };

        assert( false );
        return index;
    }

    virtual void negative_pass( std::vector<T>& data ) {
        size_t first_negative_index = find_first_negative( data );

        // new indexes for reorder: negative in reversed order, then positive in correct order
        size_t positive_index = data.size() - first_negative_index;
        size_t negative_index = data.size() - first_negative_index - 1;

        Tuint* data_uint_ptr = reinterpret_cast<Tuint*>( data.data() );

        for ( size_t i = 0; i < data.size(); i++ ) {
            if ( data[i] < 0 ) {
                parent_t::buffer[ negative_index-- ] = data[i];
            } else {
                assert( positive_index < data.size() );
                parent_t::buffer[ positive_index++ ] = data[i];
            }
        }
    }

    virtual void sort() {
        parent_t::buffer.resize( parent_t::data.size() );
        auto parts = split_data( parent_t::data );

        for ( size_t mask_step = 0; sizeof(T) * 8 > mask_step * N; mask_step++ ) {
            auto parts_counters_map = compute_counters_tbb( parts, mask_step );
            auto parts_offsets_map = compute_offsets_map( parts, parts_counters_map );
            pass_tbb( parts, parts_offsets_map, mask_step );
            std::swap( parent_t::data, parent_t::buffer );
        }

        negative_pass( parent_t::data );
        std::swap( parent_t::data, parent_t::buffer );
    }
};
#endif
