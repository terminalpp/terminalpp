#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

#include "helpers/helpers.h"

#include "stamp.h"

#include "tpp-lib/local_pty.h"

#include "ansi_renderer.h"

#include "ui/widgets/panel.h"
#include "ui/widgets/label.h"

void PrintVersion() {
    std::cout << "Terminal++ Server, version " << stamp::version << std::endl;
    std::cout << "    commit:   " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
    std::cout << "              " << stamp::build_time << std::endl;
    std::cout << "    platform: " << ARCH << " " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char * argv[]) {
    CheckVersion(argc, argv, PrintVersion);
    using namespace ui;
    using namespace tpp;
//    try {
        AnsiRenderer renderer{new LocalPTYSlave()};
        Panel * p = new Panel();
        p->setBackground(Color::Blue);
        Label * p2 = new Label();
        p2->setText("Hello world! P2");
        p2->setBackground(Color::Red);
        Label * p3 = new Label();
        p3->setText("Hello world! P3");
        p3->setBackground(Color::Green);
        //p2->resize(Size{10, 10});
        //p2->move(Point{2,2});
        p->attach(p2);
        p->attach(p3);
        renderer.setRoot(p);
        p->setLayout(new Layout::Row{HorizontalAlign::Center, VerticalAlign::Middle});
        p2->setWidthHint(SizeHint::AutoSize());
        p2->setHeightHint(SizeHint::AutoSize());
        p2->setText("Hello all folks and other people\nwho have come here!");
        p3->setHeightHint(SizeHint::AutoSize());
        p3->setHAlign(HorizontalAlign::Center);
        p3->setText("Lorem ipsum and some stuff and some here and here and also here and one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen twenty\n1 2 3 4 5 6 7 8 9 0");
        p3->setWordWrap(true);
        /*
        p2->setWidthHint(SizeHint::Percentage(50));
        p2->setHeightHint(SizeHint::Percentage(50)); */

        renderer.onKeyDown.setHandler([&](Renderer::KeyEvent::Payload & e) {
            p3->setText(STR(*e));
        });
        renderer.onMouseMove.setHandler([&](Renderer::MouseMoveEvent::Payload &e) {
            p2->setText(STR("move: " << e->coords.x() << "; " << e->coords.y()));
        });

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
