#include "helpers/helpers.h"
#include "helpers/filesystem.h"

#include "remote_files.h"

namespace tpp {

    // RemoteFile

    RemoteFile::RemoteFile(std::string const & localDir, int id, std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size):
        id_{id},
        hostname_{hostname},
        remotePath_{remotePath},
        size_{size} {
        // determine the filename and extension
        size_t lastPeriod = filename.rfind('.');
        if (lastPeriod == std::string::npos)
            lastPeriod = filename.size();
        std::string fname = filename.substr(0, lastPeriod);
        std::string ext = (lastPeriod + 1 < filename.size()) ? filename.substr(lastPeriod + 1) : "";
        // determine the local path 
        localPath_ = helpers::JoinPath(localDir, hostname + "-" + fname + "." + ext);
        int i = 0;
        while (true) {
            if (! helpers::PathExists(localPath_))
                break;
            ++i;
            localPath_ = helpers::JoinPath(localDir, STR(hostname << "-" << fname << "(" << i << ")." << ext));
        }
        // reset the file for writing
        reset(size);
    }

    void RemoteFile::appendData(char const * data, size_t numBytes) {
        ASSERT(written_ + numBytes <= size_);
        writer_.write(data, numBytes);
        written_ += numBytes;
        if (written_ == size_) {
            writer_.flush();
            writer_.close();
        }
    }

    // RemoteFiles

    RemoteFile * RemoteFiles::newFile(std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size) {
        std::string fullName{hostname + ";" + remotePath};
        auto i = map_.find(fullName);
        if (i != map_.end()) {
            i->second->reset(size);
        } else {
            // if the remote files is empty, create new temporary directory
            if (remoteFilesFolder_.empty()) {
                helpers::TemporaryFolder tf("tpp-",false);
                remoteFilesFolder_ = tf.path();
            }
            RemoteFile * rf = new RemoteFile(remoteFilesFolder_, static_cast<int>(remoteFiles_.size()), hostname, filename, remotePath, size);
            remoteFiles_.push_back(rf);
            i = map_.insert(std::make_pair(fullName, rf)).first;
        }
        return i->second;
    }

}