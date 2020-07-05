#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

#include "helpers/helpers.h"

#include "stamp.h"

#include "tpp-lib/local_pty.h"

#include "ansi_renderer.h"

#include "ui3/widgets/panel.h"

void PrintVersion() {
    std::cout << "Terminal++ Server, version " << stamp::version << std::endl;
    std::cout << "    commit:   " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
    std::cout << "              " << stamp::build_time << std::endl;
    std::cout << "    platform: " << ARCH << " " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char * argv[]) {
    CheckVersion(argc, argv, PrintVersion);
    using namespace ui3;
    using namespace tpp;
//    try {
        AnsiRenderer renderer{new LocalPTYSlave()};
        CustomPanel * p = new CustomPanel();
        renderer.setRoot(p);

        renderer.mainLoop();

        return EXIT_SUCCESS;
/*    } catch (NackError const & e) {
        std::cerr << "t++ terminal error: " << e.what() << "\033[0K\r\n";
    } catch (TimeoutError const & e) {
        std::cerr << "t++ terminal timeout.\033[0K\r\n"; 
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\033[0K\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\033[0K\r\n";
    }
    */
    return EXIT_FAILURE;
}
