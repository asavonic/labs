#ifndef __RADIX_OMP_H
#define __RADIX_OMP_H

#include <radix_simple.h>
#include <valarray>

template <class T, std::size_t N >
class radix_omp  : public radix_simple<T, N> {
public:

    typedef typename radix_simple<T, N>::Tuint Tuint;

    virtual void pass( size_t n ) {
        std::valarray<T> counters ( T(0), 1 << N );

//#pragma omp parallel private( counters ) 
        for ( size_t i = 0; i < this->array.size(); i++ ) {
            Tuint* int_ptr = reinterpret_cast<Tuint*>( &( this->array[i] ) );
            Tuint int_val = *int_ptr;
            int_val >>= N * n;
            int_val &= ~( ( ~0u ) << N );
            counters[ int_val ] += 1;
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
            this->offset_table[0] = 0;
            for ( size_t i = 1; i < this->offset_table.size(); i++ ) {
                this->offset_table[i] = this->offset_table[i - 1] + counters[i - 1];
            }
        } 
        else {
            this->offset_table[0] = negative_num;     

            // positive
            for ( size_t i = 1; i < first_negative; i++ ) {
                this->offset_table[i] = this->offset_table[i - 1] + counters[i - 1];
            }

            //negative with inverted order
            this->offset_table[ last_negative ] = 0;     
            for( size_t i = 0; i < first_negative - 1 ; i++) {
                this->offset_table[ last_negative -1 - i ] = this->offset_table[ last_negative - i ] + counters[ last_negative - i ];
            }
        } 

//#pragma omp parallel for
        for ( size_t i = 0; i < this->array.size(); i++ ) {
            Tuint* int_ptr = reinterpret_cast<Tuint*>( &this->array[i] );
            Tuint int_val = *int_ptr;
            int_val >>= N * n;
            int_val &= ~( ( ~0u ) << N );
            
            this->buffer[ this->offset_table[ int_val ]++ ] = this->array[i];
        }

        std::swap( this->array, this->buffer );
    }
        
};

#endif

