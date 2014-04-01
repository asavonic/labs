#include <radix_omp.h>
#include <omp.h>

int main( int argc, char** argv ) {
    
    omp_set_dynamic(0);
    omp_set_num_threads(2);

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dis(-1000.f, 1000.f);
    auto rand_float = std::bind(dis, gen);

    radix_omp<double, 8> sort;
    sort.array.reserve( 10000000 );
    for ( size_t i = 0; i < sort.array.capacity(); i++ ) {
        sort.array.push_back( rand_float() );
    }

    sort.run();

    if ( std::is_sorted( sort.array.begin(), sort.array.end() ) ) {
        std::cout << "SUCCESS" << std::endl;
    }
    else {
        std::cout << "FAIL" << std::endl;
    }

    std::cout << "time elapsed = " << sort.time_spent.count() << " ms" << std::endl;
}
