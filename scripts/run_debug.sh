# !/bin/bash

# Script to build and run the appwikipedia_qt application in debug mode

# Check if the first argument is "rebuild"
if [[ $1 == "rebuild" ]]; then
    echo "Rebuilding the project..."
    rm -rf build
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build || { echo "CMake configuration failed"; exit 1; }
fi

# Navigate to the build directory
cd build || { 
    echo "Failed to navigate to build directory, rebuilding...";
    mkdir -p build && cd build
    cmake .. || { echo "CMake configuration failed"; exit 1; }
    make || { echo "Build failed"; exit 1; }
}

# Navigate to the bin directory
cd bin || { echo "Failed to navigate to bin directory"; exit 1; }

# Run the application without enabling debug logging for all Qt components
echo "Running appwikipedia_qt in debug mode..."
QT_LOGGING_RULES="default.debug=true" ./appwikipedia_qt

# Exit with the status of the application
echo "Application exited with status: $?"
exit $?
