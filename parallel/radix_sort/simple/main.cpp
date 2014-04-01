#include <radix_simple.h>


int main( int argc, char** argv ) {
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dis(-1000.f, 1000.f);
    auto rand_float = std::bind(dis, gen);

    radix_simple<double, 8> sort;
    sort.array.reserve( 100 );
    for ( size_t i = 0; i < sort.array.capacity(); i++ ) {
        sort.array.push_back( rand_float() );
    }

    std::vector<double> array = sort.array;

    sort.write_to_file("input.log");
    sort.run();

    if ( std::is_sorted( sort.array.begin(), sort.array.end() ) ) {
        std::cout << "SUCCESS" << std::endl;
    }
    else {
        std::cout << "FAIL" << std::endl;
        sort.write_to_file("output_actual.log");
    }

    std::cout << "time elapsed = " << sort.time_spent.count() << " ms" << std::endl;
}
