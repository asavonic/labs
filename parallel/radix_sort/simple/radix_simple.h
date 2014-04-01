#ifndef __RADIX_SIMPLE_H
#define __RADIX_SIMPLE_H

#include <sorter.h>
#include <array>
#include <algorithm>
#include <cmath>

template <class T, std::size_t N >
class radix_simple  : public sorter<T> {
public:
    std::array<T, 1 << N >   offset_table;
    std::vector<T>     buffer;

    // here goes magic. dont know how to make it better.
    typedef struct {} not_supported_type;

    typedef typename std::conditional< sizeof(T) == 1, uint8_t, not_supported_type >::type uint_le_8;
    typedef typename std::conditional< sizeof(T) == 2, uint16_t, uint_le_8 >::type uint_le_16;
    typedef typename std::conditional< sizeof(T) == 4, uint32_t, uint_le_16 >::type uint_le_32;
    typedef typename std::conditional< sizeof(T) == 8, uint64_t, uint_le_32 >::type Tuint;
    
    radix_simple() {
        static_assert( std::is_arithmetic<T>::value,
                                  "Radix sort works only with arithmetic types" );
        static_assert( ! std::is_same<Tuint, not_supported_type>::value,
                                  "Can`t find uint representation of T" );
    }

    virtual void sort() {
        // constexpr uint32_t highest_bit = 1 << ( sizeof(T) * 8 - 1 );

        this->buffer.resize( this->array.size() );
        for ( size_t step = 0; sizeof(T) * 8 > step * N; step++ ) {
            pass( step );
        }
    }

    virtual void pass( size_t n ) {
        std::array< T, 1 << N >   counters = {};
        for ( T val : this->array ) {
            Tuint* int_ptr = reinterpret_cast<Tuint*>( &val );
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

            //negative goes to front
            this->offset_table[ last_negative ] = counters[ last_negative ];     
            for( size_t i = 0; i < first_negative - 1 ; i++) {
                this->offset_table[ last_negative - 1 - i ] = this->offset_table[ last_negative - i ] + counters[ last_negative - 1 - i ];
            }
        } 

        for ( size_t i = 0; i < this->array.size(); i++ ) {
            Tuint* int_ptr = reinterpret_cast<Tuint*>( &this->array[i] );
            Tuint int_val = *int_ptr;
            int_val >>= N * n;
            int_val &= ~( ( ~0u ) << N );
            

            // negative must be reverted
            if ( last_step && int_val >= first_negative ) {
                this->buffer[ --this->offset_table[ int_val ] ] = this->array[i];
            }
            else {
                this->buffer[ this->offset_table[ int_val ]++ ] = this->array[i];
            }
        }

        std::swap( this->array, this->buffer );
    }
        
};

#endif
