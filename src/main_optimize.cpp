#include "../include/parameters.hpp"
#include "../include/metaoptim.hpp"

int main(int argc, char *argv[]){

    // Initialize the optimization
    PatternSearch PS;
    PS.set_from_file(argv[1]);

    // Solve it all
    PS.solve(25);
    return 1;
}