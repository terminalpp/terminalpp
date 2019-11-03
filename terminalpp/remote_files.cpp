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
        getAvailableLocalPath();
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

    void RemoteFile::getAvailableLocalPath() {
        size_t lastPeriod = localPath_.rfind('.');
        if (lastPeriod == std::string::npos)
            lastPeriod = localPath_.size();
        std::string fname = localPath_.substr(0, lastPeriod);
        std::string ext = (lastPeriod + 1 < localPath_.size()) ? localPath_.substr(lastPeriod + 1) : "";
        int i = 1;
        while (true) {
            if (! helpers::PathExists(localPath_))
                break;
            ++i;
            localPath_ = STR(fname << "(" << i << ")." << ext);
        }
    }

    // RemoteFiles

    RemoteFile * RemoteFiles::newFile(std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size) {
        std::string fullName{hostname + ";" + remotePath};
        auto i = map_.find(fullName);
        if (i != map_.end()) {
            i->second->reset(size);
        } else {
            ASSERT(!remoteFilesFolder_.empty());
            RemoteFile * rf = new RemoteFile(remoteFilesFolder_, static_cast<int>(remoteFiles_.size()), hostname, filename, remotePath, size);
            remoteFiles_.push_back(rf);
            i = map_.insert(std::make_pair(fullName, rf)).first;
        }
        return i->second;
    }

}