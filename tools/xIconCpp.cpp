#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>

/** Very simple tool which takes a RGBA image file containing raw pixel data, and produces a C++ file which embeds the definition in a format usable for the X11. 

    The RGBA file may contain multiple icon sizes, but since it only contains raw pixel values, the sizes of the icons within it must be provided as an argument to the script as well. 

	The format of the output array is:

	- total size of the array (not counting this first item)
	- for each icon size contained the following:
	    - width
		- height
		- pixel data [ width x height of them ]

	The icon width and height must be identical. 
 */
int main(int argc, char * argv[]) {
    if (argc < 5) {
        std::cerr << "Invalid arguments, usage:" << std::endl << std::endl;
        std::cerr << "xIconCpp varName output.cpp input.rgba SIZE1 SIZE2 ... SIZEN" << std::endl << std::endl;
        std::cerr << "Where sizes are dimensions of the icons in the rgba file" << std::endl;
        std::exit(-1);
    }
    std::string varName = argv[1];
    std::string outputCpp = argv[2];
    std::string inputRgba = argv[3];
    std::vector<unsigned> sizes;
    for (int i = 4; i < argc; ++i)
        sizes.push_back(std::stoi(argv[i]));
    std::ofstream o(outputCpp);
    std::ifstream input(inputRgba, std::ios::binary);
    o << "/* AUTOGENERATED FILE, DO NOT EDIT! \n\n";
    o << "   This file was produced by the following command:\n\n";
    o << "   xIconCpp " << varName << " " << outputCpp << " " << inputRgba;
    size_t numElements = 0;
    for (unsigned size: sizes) {
        o << " " << size;
        numElements += 2 + (size * size);
    }
    o << "\n */\n\n";
    o << "namespace tpp {\n\n";
    o << "    unsigned long " << varName << "[] = {\n";
    o << "        // number of elements (not counting this one)\n";
    o << "        " << numElements << ",";
    for (unsigned size : sizes) {
        o << "\n        // icon size " << size << " (width x height)\n";
        o << "        " << size << "," << size << ",\n";
        o << "        // icon data (rgba)" << std::hex;
        for (unsigned i = 0, e = size * size; i != e; ++i) {
            if (i % size == 0)
            o << "\n        ";
            uint32_t x;
            input.read(reinterpret_cast<char *>(&x), 4);
            o << "0x" << x << ",";
        }
        o << std::dec;
    }
    o << "\n    };\n";
    o << "} // namespace tpp\n";
    std::cerr << "Done. " << numElements << " elements created" << std::endl;
}