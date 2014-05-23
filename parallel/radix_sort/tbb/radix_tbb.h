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

    std::vector<T> buffer;

    struct ComputeChunk {
        typedef typename radix_simple<T, N>::Tuint Tuint;

        std::vector< std::pair<int, int> > chunks;
        std::reference_wrapper< std::vector<T> > data;
        std::reference_wrapper< std::vector<T> > buffer;

        int final_swap; // counter, need to correctly swap back to data after join stage

        ComputeChunk ( const ComputeChunk& parent, tbb::split ) : data(parent.data), buffer(parent.buffer) {
            final_swap = parent.final_swap;
        }

        ComputeChunk ( radix_tbb<T, N>* sorter ) : data(sorter->data), buffer(sorter->buffer) {
            final_swap = 0;
        }

        struct {
            using chunk_iterator =  typename decltype(chunks)::iterator;
            size_t pos;
            chunk_iterator begin;
            chunk_iterator end;
            chunk_iterator chunk;

            void init( chunk_iterator _begin, chunk_iterator _end ) {
                begin = _begin;
                end   = _end;
                chunk = begin;
                pos   = chunk->first;
            }

            int get_next() {
                if ( chunk != end ) {
                    if ( pos == chunk->second ) {
                        chunk++;
                        if ( chunk != end ) {
                            pos = chunk->first;
                        } 
                        else {
                            return -1;
                        }
                    }

                    return pos++;
                }
                else {
                    return -1;
                }
            }
        } merge;


        void operator()( tbb::blocked_range<int> r ) {
            chunks.push_back( std::pair<int, int>( r.begin(), r.end() ) );

            std::sort( data.get().begin() + r.begin(), data.get().begin() + r.end() );

            merge.init( chunks.begin(), chunks.end() );
        }

        void join( ComputeChunk& other ) {
            auto new_chunks = chunks;
            new_chunks.insert( new_chunks.end(), other.chunks.begin(), other.chunks.end() );

            int my_pos = merge.get_next();
            int other_pos = other.merge.get_next();

            for ( auto& chunk : new_chunks ) {
                for ( size_t i = chunk.first; i < chunk.second; i++ ) {
                    if ( other_pos == -1 || data.get()[my_pos] < other.data.get()[other_pos] ) {
                        buffer.get()[i] = data.get()[my_pos];
                        my_pos = merge.get_next();
                    } 
                    else {
                        if ( my_pos == -1 || other.data.get()[other_pos] < data.get()[my_pos] ) {
                            buffer.get()[i] = other.data.get()[other_pos];
                            other_pos = other.merge.get_next();
                        }
                    }
                }
            }

            chunks = new_chunks;
            std::swap( buffer, data );
            final_swap += other.final_swap + 1;
        }
    };

    friend struct ComputeChunk;


    virtual void sort() {
        buffer.resize( this->data.size() );
        ComputeChunk compute(this);

        tbb::parallel_reduce( tbb::blocked_range<int>( 0, this->data.size() ),
                              compute);

        if ( std::is_sorted( buffer.begin(), buffer.end() ) ) {
            std::swap( buffer, this->data );
            std::cout << "swapped" << std::endl;
        }
        std::cout << "final swap = " << compute.final_swap << std::endl;
    } 
    
    
};
#endif
