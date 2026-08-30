#include "SnesApp.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "ERROR: No ROM path detected." << std::endl;
        return 1;
    }

    try {
        SnesApp app {argv[1]};
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}