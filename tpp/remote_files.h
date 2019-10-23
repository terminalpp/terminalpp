#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>


namespace tpp {

    /** Describes a locally stored remote file. 
     */
    class RemoteFile {
    public:

        int id() const {
            return id_;
        }

        std::string const & localPath() const {
            return localPath_;
        }

        size_t size() const {
            return size_;
        }

        bool available() const {
            return size_ > 0 && written_ == size_;
        }

        void appendData(char const * data, size_t numBytes);

        size_t transferredBytes() const {
            return written_;
        }

    private:
        friend class RemoteFiles;

        RemoteFile(std::string const & localDir, int id, std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size);

        void reset(size_t size) {
            size_ = size;
            written_ = 0;
            writer_ = std::ofstream(localPath_, std::ios::binary);
            ASSERT(writer_.good());
        }

        int id_;
        std::string hostname_;
        std::string remotePath_;
        std::string localPath_;
        size_t size_;
        std::ofstream writer_;
        size_t written_;
    }; // tpp::RemoteFile

    class RemoteFiles {
    public:

        /** Returns the file descriptor for a new file request. 
         
            Same files (same host & path can be recycled).
         */
        RemoteFile * newFile(std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size);

        RemoteFile * get(int fileId) {
            if (fileId < 0 || fileId >= static_cast<int>(remoteFiles_.size()))
                THROW(helpers::Exception()) << "Remote file id " << fileId << " not found.";
            return remoteFiles_[fileId];
        }

    private:
        std::unordered_map<std::string, RemoteFile *> map_;

        std::vector<RemoteFile *> remoteFiles_;

        std::string remoteFilesFolder_;

    }; // tpp::RemoteFiles

} // namespace tpp