#ifndef __RADIX_OMP_H
#define __RADIX_OMP_H

#include <radix_simple.h>
#include <valarray>
#include <atomic>

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

#pragma omp parallel 
        {
            auto local_counters = counters;
#pragma omp for
            for ( size_t i = 0; i < this->array.size(); i++ ) {
                Tuint* int_ptr = reinterpret_cast<Tuint*>( &( this->array[i] ) );
                Tuint int_val = *int_ptr;
                int_val >>= N * n;
                int_val &= ~( ( ~0u ) << N );
                local_counters[ int_val ] += 1;
            }
#pragma omp critical
            counters += local_counters;
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

#pragma omp parallel for
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
        
};

#endif

