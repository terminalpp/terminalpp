#include "helpers/log.h"
#include "helpers/filesystem.h"

#include "remote_files.h"

namespace tpp {

    // Remote Files

    Sequence::Ack::Response RemoteFiles::openFileTransfer(Sequence::OpenFileTransfer const & req) {
        // find if the file has already been registered
        std::string remoteHost = req.remoteHost().empty() ? "unknown" : req.remoteHost();
        std::filesystem::path remotePath{req.remotePath()};
        std::string remoteFilename = remotePath.filename().string();
        std::filesystem::path localPath = localRoot_ / remoteHost / remoteFilename;
        // if the local path exists, look if there is existing connection id
        File * file = getOrCreateFile(remoteHost, req.remotePath(), localPath, req.size());
        // create the file and open its stream
        if (file->f_.good())
            file->f_.close();
        file->f_.open(file->localPath_, std::ios::binary);
        // if the file can't be opened, maybe it is locked by existing viewer, rename and try again
        if (!file->f_.good()) {
            std::pair<std::string, std::string> fext = SplitFilenameExt(remotePath);
            std::string filename = UniqueNameIn(localRoot_ / remoteHost, fext.first, fext.second);
            localPath = localRoot_ / remoteHost / filename;
            file->localPath_ = localPath.string();
            file->f_.open(file->localPath_, std::ios::binary);
            if (!file->f_.good())
                THROW(IOError()) << "Unable to open local file for writing: " << file->localPath();
        }
        // return the acknowledgement
        return Sequence::Ack::Response{Sequence::Ack{req, file->id_}};
    }

    bool RemoteFiles::transfer(Sequence::Data const & data) {
        File * f = get(data.id());
        // only accept the transfer if the data is from valid offset
        if (f->received_ != data.packet())
            return false;
        // store the file
        f->f_.write(data.payload(), data.size());
        f->received_ += data.size();
        // if all has been received, close the file
        if (f->received_ == f->size_)
            f->f_.close();
        return true;
    }

    Sequence::TransferStatus::Response RemoteFiles::getTransferStatus(Sequence::GetTransferStatus const & req) {
        File * f = get(req.id());
        if (f == nullptr)
            return Sequence::TransferStatus::Response::Deny(req, "Not found");
        else
            return Sequence::TransferStatus::Response{Sequence::TransferStatus{req.id(), f->size_, f->received_}};
    }

    RemoteFiles::File * RemoteFiles::getOrCreateFile(std::string const & remoteHost, std::string const & remotePath, std::filesystem::path const & localPath, size_t size) {
        if (std::filesystem::exists(localPath)) {
            for (auto i : files_) {
                if (i.second->remoteHost() == remoteHost && i.second->remotePath() == remotePath) {
                    i.second->size_ = size;
                    i.second->received_ = 0;
                    return i.second;
                }
            }
        }
        // if not found, create new id and file record and make sure the path exists
        std::filesystem::create_directories(localRoot_ / remoteHost);
        size_t id = 1;
        for (auto i : files_) {
            if (i.first > id)
                break;
            id = i.first + 1;
        }
        File * f = new File(remoteHost, remotePath, localPath.string(), size, id);
        files_.insert(std::make_pair(id, f));
        return f;
    }

} // namespace tpp