#include <iostream>
#include <memory>
#include <boost/program_options.hpp>
#include <sorter.h>
#include <radix_simple.h>
#include <radix_omp.h>
#include <radix_tbb.h>
#include <radix_mpi.h>

using sort_type = double;

void run_sort( sorter<sort_type>* sort, size_t size ) {
    if ( sort->rank() == 0 ) { 
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_real_distribution<sort_type> dis(-1000.f, 1000.f);
        auto rand_float = std::bind(dis, gen);

        sort->data.reserve( size );
        for ( size_t i = 0; i < sort->data.capacity(); i++ ) {
            sort->data.push_back( rand_float() );
        }
    } 

    sort->run();

    if ( std::is_sorted( sort->data.begin(), sort->data.end() ) ) {
        std::cout << "SUCCESS" << std::endl;
    }
    else {
        std::cout << "FAIL" << std::endl;
        sort->write_to_file("output.log");
    }

    std::cout << "time elapsed = " << sort->time_spent.count() << " ms" << std::endl;
}

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    constexpr int radix_step  = 8;

    try {
        size_t array_size = 0;
        std::string input_file_path;
        std::string output_file_path;
        std::string parallel;
        bool no_verification = 0;

        po::options_description desc("This is radix sort runner.\n\nAllowed options:");
        desc.add_options()
            ("help", "produce help message")
            ("size", po::value<size_t>( &array_size )->required(), "number of elements in array")
            ("input,i", po::value< std::string >( &input_file_path ), "path to input file")
            ("output,o", po::value< std::string >( &output_file_path ), "path to output file")
            ("parallel", po::value< std::string >( &parallel )->required(), "set to omp, tbb, mpi_omp or none")
            ("no-verification", "disable verification of result data")
        ;


        po::variables_map vm;
        po::store( po::command_line_parser( argc, argv).options(desc).run(), vm );

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        if ( vm.count("no-verification") ) {
            no_verification = true;
        }

        po::notify(vm);

        std::shared_ptr< sorter<sort_type> > sort;

        if ( parallel == "none" ) {
            sort.reset( new radix_simple<sort_type, radix_step> );
        }
        else if ( parallel == "omp" ) {
            sort.reset( new radix_omp<sort_type, radix_step> );
        } else if ( parallel == "tbb" ) {
            sort.reset( new radix_tbb<sort_type, radix_step> );
        } else if ( parallel == "mpi_omp" ) {
            sort.reset( new radix_mpi<sort_type, radix_step> );
        } else {
            throw std::runtime_error( "parallel mode was not defined properly" );
        }

        run_sort( sort.get(), array_size );
    }
    catch ( boost::program_options::error& po_error ) {
        std::cerr << po_error.what() << std::endl; 
    }
    catch ( std::exception& ex ) {
        std::cerr << "Unknown exception: " << ex.what() << std::endl;
    }
    return 0;
}

