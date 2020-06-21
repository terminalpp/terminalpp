#pragma once

#include "ui/widgets/window.h"
#include "ui/widgets/pager.h"
#include "ui-terminal/ansi_terminal.h"
#include "tpp-lib/local_pty.h"
#include "tpp-lib/bypass_pty.h"
#include "tpp-lib/remote_files.h"

#include "../config.h"
#include "../window.h"

namespace tpp {

    /** New Version Dialog. 
     */
    class NewVersionDialog : public ui::Dialog::Cancel {
    public:
        NewVersionDialog(std::string const & message):
            Dialog::Cancel{"New Version", /* deleteOnDismiss */ true},
            contents_{new Label{message}} {
            setBody(contents_);
            setSemanticStyle(SemanticStyle::Info);
        }

    private:
        Label * contents_;
    };

    /** The terminal window. 
     
        
     */
    class TerminalWindow : public ui::Window {
    public:

        TerminalWindow(tpp::Window * window):
            window_{window},
            main_{new PublicContainer{}},
            pager_{new Pager{}} {
            Config const & config = Config::Instance();


            main_->setLayout(new ColumnLayout{VerticalAlign::Top});
            main_->add(pager_);
            setContents(main_);
            window_->setRootWidget(this);
    
            remoteFiles_ = new RemoteFiles(config.remoteFiles.dir());

            if (config.renderer.window.fullscreen())
                window_->setFullscreen(true);
            
            versionChecker_ = std::thread{[this](){
                std::string channel = Config::Instance().version.checkChannel();
                // don't check if empty channel
                if (channel.empty())
                    return;
                std::string newVersion = Application::Instance()->checkLatestVersion(channel);
                if (!newVersion.empty()) {
                    sendEvent([this, newVersion]() {
                        NewVersionDialog * d = new NewVersionDialog{STR("New version " << newVersion << " is available")};
                        showModal(d);
                    });
                }
            }};

            setName("TerminalWindow");
        }

        ~TerminalWindow() override {
            versionChecker_.join();
            delete remoteFiles_;
        }

        void newSession(Config::sessions_entry const & session);


    private:

        class SessionInfo {
        public:
            std::string name;
            PTYMaster * pty;
            AnsiTerminal * terminal;
            AnsiTerminal::Palette palette;

            SessionInfo(Config::sessions_entry const & session):
                name{session.name()},
                pty{nullptr},
                terminal{nullptr},
                palette{session.palette()} {
            }
        }; 

        tpp::Window * window_;

        ui::PublicContainer * main_;
        ui::Pager * pager_;

        std::unordered_map<AnsiTerminal *, SessionInfo *> sessions_; 

        RemoteFiles * remoteFiles_;

        std::thread versionChecker_;

        

    };

} // namespace tpp