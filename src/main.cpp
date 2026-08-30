#include "SnesApp.hpp"

#include <iostream>

int main(int argc, char** argv) {
    SnesApp app {argv[1]};

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}