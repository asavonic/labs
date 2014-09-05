#include <iostream>
#include <boost/program_options.hpp>
#include <sorter.h>
#include <radix_simple.h>
#include <radix_omp.h>
#include <radix_tbb.h>
#include <radix_mpi.h>

using sort_type = double;

int run_sort( sorter<sort_type>* sort, size_t size, bool force_dump, bool validate ) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<sort_type> dis( -1000.f, 1000.f);
    auto rand_float = std::bind(dis, gen);

    sort->data.reserve( size );
    for ( size_t i = 0; i < sort->data.capacity(); i++ ) {
        sort->data.push_back( rand_float() );
    }
    
    std::vector<sort_type> gold_result;
    if ( validate ) { 
        gold_result = sort->data;
        std::sort( gold_result.begin(), gold_result.end() );
    }

    if ( force_dump ) {
        sort->write_to_file("input.log");
    }

    sort->run();

    std::cout << "time elapsed = " << sort->time_spent.count() << " ms" << std::endl;

    if ( force_dump ) {
        sort->write_to_file("output.log");
    }

    if ( validate ) {
        for ( size_t i = 0; i < gold_result.size(); i++ ) {
            if ( gold_result[i] != sort->data[i] ) {
                std::cerr << "Verification failed at index " <<  i << ":" << std::endl <<
                            "\tgold_result[i] = " << gold_result[i] << std::endl <<
                            "\tsort->data[i] = " << sort->data[i] << std::endl;
                return 1;
            }
        }
    }

    return 0;

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

        po::options_description desc("This is radix sort runner.\n\nAllowed options:");
        desc.add_options()
            ("help", "produce help message")
            ("size", po::value<size_t>( &array_size ), "number of elements in array")
            ("input,i", po::value< std::string >( &input_file_path ), "path to input file")
            ("output,o", po::value< std::string >( &output_file_path ), "path to output file")
            ("parallel", po::value< std::string >( &parallel )->required(), "set to omp, tbb, mpi_omp or none")
            ("force-dump", "for dump data to input.log and output.log")
            ("validate", "compare sorting with std::sort")
        ;


        po::variables_map vm;
        po::store( po::command_line_parser( argc, argv).options(desc).run(), vm );

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        po::notify(vm);

        std::shared_ptr< sorter<sort_type> > sort;

        if ( parallel == "none" ) {
            sort.reset( new radix_simple<sort_type, radix_step> );
        } else if ( parallel == "omp" ) {
            sort.reset( new radix_omp<sort_type, radix_step> );
        } else if ( parallel == "tbb" ) {
            constexpr size_t split_parts_count = 256; // split array to 256 parts and let workers process them
            sort.reset( new radix_tbb<sort_type, radix_step, split_parts_count> );
        } else if ( parallel == "mpi_omp" ) {
            sort.reset( new radix_mpi<sort_type, radix_step> );
        } else {
            throw std::runtime_error( "parallel mode was not defined properly" );
        }

        bool force_dump = vm.count("force-dump");
        bool validate = vm.count("validate");

        int retcode = run_sort( sort.get(), array_size, force_dump, validate );
        std::cout << ( retcode ? "FAIL" : "SUCCESS" ) << std::endl;

        return retcode;
    }
    catch ( boost::program_options::error& po_error ) {
        std::cerr << po_error.what() << std::endl; 
    }
    catch ( std::exception& ex ) {
        std::cerr << "Unknown exception: " << ex.what() << std::endl;
    }
    return 0;
}

