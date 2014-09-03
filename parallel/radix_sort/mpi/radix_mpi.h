#ifndef __RADIX_MPI_H
#define __RADIX_MPI_H

#include <radix_simple.h>
#include <valarray>
#include <atomic>
#include <limits>

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
namespace mpi = boost::mpi;

template <class T, std::size_t N >
class radix_mpi  : public radix_simple<T, N> {
public:

    using parent_t = radix_simple<T,N>;

    virtual void sort() override 
    {
        mpi_sort();
        if ( world_rank != 0 ) {
            exit( 0 );
        }
    }

    virtual void mpi_sort() 
    {
        mpi::environment env;
        mpi::communicator world;
        world_rank = world.rank();
        int root = 0;

        std::cout << "I am process " << world.rank() << " of " << world.size()
                    << "." << std::endl;

        std::vector<T> process_data( parent_t::data.size() / world.size() );
        size_t root_data_shift = parent_t::data.size() - world.size() * process_data.size(); 

        mpi::scatter( world, &parent_t::data[root_data_shift], &process_data.front(), process_data.size(), root );

        if ( world.rank() == root ) {
            std::sort( parent_t::data.begin(), parent_t::data.begin() + root_data_shift );
        }

        std::sort( process_data.begin(), process_data.end() );

        mpi::gather( world, &process_data.front(), process_data.size(), &parent_t::data[root_data_shift], root );

        if ( world.rank() == root ) {
            std::vector< std::pair<size_t,size_t> > gather_indexes;

            gather_indexes.push_back( std::make_pair( 0, root_data_shift ) );

            for ( int i = 0; i < world.size(); i++ ) {
                size_t prev_end = gather_indexes.back().second;
                gather_indexes.push_back( std::make_pair( prev_end, prev_end + process_data.size() ) );
            }

            merge_result( gather_indexes );
        }
    }

    virtual void merge_result( std::vector< std::pair<size_t, size_t> > indexes ) {
        parent_t::buffer.resize( parent_t::data.size() );

        for ( size_t i = 0; i < parent_t::buffer.size(); i++ ) {
            parent_t::buffer[i] = std::numeric_limits<T>::max();
            size_t* index_of_min_ptr = nullptr;

            for ( auto& index : indexes ) {
                if ( index.first != index.second && parent_t::data[ index.first ] < parent_t::buffer[i] ) {
                     parent_t::buffer[i] = parent_t::data[ index.first ];
                     index_of_min_ptr = &index.first;
                }
            }

            *index_of_min_ptr++;
        }

        std::swap( parent_t::buffer, parent_t::data );
    }

    virtual void hello() {
        std::cout << "hello from radix_omp" << std::endl;
    }

    virtual void pass( size_t n ) {
        
    }
        
private:
    size_t world_rank;
};

#endif

