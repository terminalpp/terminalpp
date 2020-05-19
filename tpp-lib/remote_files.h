#pragma once
#include <mutex>
#include <map>
#include <string>
#include <filesystem>

#include "sequence.h"

namespace tpp {

    /** Remote files manager. 
     
        Manages the remote files on the terminal++ server. 

     */ 
    class RemoteFiles {
    public:
        /** Information about local copy of the remote file. 
          
            TODO should this really be public, or is it only internal bookkeeping.
         */
        class File {
        public:
            std::string const & remoteHost() const {
                return remoteHost_;
            }

            std::string const & remotePath() const {
                return remotePath_;
            }

            std::string const & localPath() const {
                return localPath_;
            }

            size_t size() const {
                return size_;
            }

        private:
            friend class RemoteFiles;

            File(std::string const & remoteHost, std::string const & remotePath, std::string const & localPath, size_t size):
                remoteHost_{remoteHost},
                remotePath_{remotePath},
                localPath_{localPath},
                size_{size},
                received_{0} {
            }

            std::string remoteHost_;
            std::string remotePath_;
            std::string localPath_;
            size_t size_;
            size_t received_;
            std::ofstream f_;
        }; // RemoteFiles::File

        RemoteFiles():
            localRoot_{"c:/delete/tpp-remote-files"} {
        }

        Sequence::Ack openFileTransfer(Sequence::OpenFileTransfer const & req);

        bool transfer(Sequence::Data const & data);

        Sequence::TransferStatus getTransferStatus(Sequence::GetTransferStatus const & req);

    private:

        File * get(size_t id) {
            std::lock_guard<std::mutex> g(mFiles_);
            auto i = files_.find(id);
            return i == files_.end() ? nullptr : i->second;
        }


        /** Path to where the remote files are stored. 
         */
        std::filesystem::path localRoot_;

        /** Active file transfers. */
        std::map<size_t, File *> files_;

        /** Mutex to guard the files map. */
        std::mutex mFiles_;

    }; 


} // namespace tpp