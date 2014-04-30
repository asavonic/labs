#ifndef __RADIX_OMP_H
#define __RADIX_OMP_H

#include <radix_simple.h>
#include <valarray>
#include <atomic>
#include <omp.h>


template <class T, std::size_t N >
class radix_omp  : public radix_simple<T, N> {
public:

    typedef typename radix_simple<T, N>::Tuint Tuint;

    virtual void hello() {
        std::cout << "hello from radix_omp" << std::endl;
    }

    virtual void pass( size_t n ) {
        std::valarray<size_t> counters ( T(0), 1 << N );
        std::array< std::atomic_size_t, 1 << N> offset_table = {};
        
        std::vector< decltype(counters) > thread_counters;
#pragma omp parallel 
        {
            unsigned int num_threads = omp_get_num_threads();
            unsigned int tid = omp_get_thread_num();
#pragma omp master
            thread_counters.resize( num_threads );
#pragma omp barrier
            auto local_counters = counters;
            auto borders = parallel_for_get_borders( num_threads, tid, 0, this->array.size(), 1 );
            for ( size_t i = borders.first; i < borders.second; i++ ) {
                Tuint* int_ptr = reinterpret_cast<Tuint*>( &( this->array[i] ) );
                Tuint int_val = *int_ptr;
                int_val >>= N * n;
                int_val &= ~( ( ~0u ) << N );
                local_counters[ int_val ] += 1;
            }
#pragma omp barrier
#pragma omp critical
            counters += local_counters;

            thread_counters[ tid ] = std::move( local_counters );
        }

        // if this is the last step we need to properly deal with negative 
        size_t negative_num = 0;
        bool last_step =  ( sizeof(T) * 8 == (n + 1) * N );
        constexpr size_t first_negative = std::pow( 2, N - 1 );
        constexpr size_t last_negative = std::pow( 2, N ) - 1;

        if ( last_step ){ 
            for ( size_t i = first_negative; i < last_negative; i++ ) {
                negative_num += counters[i]; 
            }

        }

        if ( !last_step ) {
            offset_table[0] = 0;
            for ( size_t i = 1; i < offset_table.size(); i++ ) {
                offset_table[i] = offset_table[i - 1] + counters[i - 1];
            }
        } 
        else {
            offset_table[0] = negative_num;     

            // positive
// #pragma omp parallel for
            for ( size_t i = 1; i < first_negative; i++ ) {
                offset_table[i] = offset_table[i - 1] + counters[i - 1];
            }

            //negative goes to front
            offset_table[ last_negative ] = counters[ last_negative ];     

            for( size_t i = 0; i < first_negative - 1 ; i++) {
                offset_table[ last_negative - 1 - i ] = offset_table[ last_negative - i ] + counters[ last_negative - 1 - i ];
            }
        } 

//#pragma omp parallel for
        for ( size_t i = 0; i < this->array.size(); i++ ) {
            Tuint* int_ptr = reinterpret_cast<Tuint*>( &this->array[i] );
            Tuint int_val = *int_ptr;
            int_val >>= N * n;
            int_val &= ~( ( ~0u ) << N );
            

            size_t index = 0;
            // negative must be reverted
            if ( last_step && int_val >= first_negative ) {
                index = --offset_table[ int_val ];
            }
            else {
                index = offset_table[ int_val ]++; 
            }
            this->buffer[ index ] = this->array[i];
        }

        std::swap( this->array, this->buffer );
    }
        
private:
    std::pair<size_t, size_t> parallel_for_get_borders( unsigned int num_threads, unsigned int tid, size_t begin, size_t end, int inc ) {
        size_t iterations_num = ( end - begin ) / inc;

        size_t first = ( iterations_num / num_threads ) * inc * tid;
        size_t second = ( tid == num_threads - 1 ) ? end : ( iterations_num / num_threads ) * inc * ( tid + 1 ); 

        return std::pair<size_t, size_t> { first, second };
    }
};

#endif

