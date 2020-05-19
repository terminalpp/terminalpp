#include <filesystem>

#include "helpers/log.h"

#include "remote_files.h"

namespace tpp {

    // Remote Files

    Sequence::Ack RemoteFiles::openFileTransfer(Sequence::OpenFileTransfer const & req) {
        // find if the file has already been registered
        std::string remoteHost = req.remoteHost().empty() ? "unknown" : req.remoteHost();
        std::filesystem::path remotePath{req.remotePath()};
        std::string remoteFilename = remotePath.filename().string();
        std::filesystem::path localPath = localRoot_ / remoteHost / remoteFilename;
        // if the local path exists, look if there is existing connection id
        if (std::filesystem::exists(localPath)) {

        }
        // if not found, create new id and file record
        size_t id = 1;
        for (auto i : files_) {
            if (i.first > id)
                break;
            id = i.first + 1;
        }
        // make sure the path exists
        std::filesystem::create_directories(localRoot_ / remoteHost);
        // create the file and open its stream
        File * file = new File{remoteHost, req.remotePath(), localPath.string(), req.size()};
        file->f_.open(file->localPath_, std::ios::binary);
        if (!file->f_.good())
            THROW(helpers::IOError()) << "Unable to open local file for writing: " << file->localPath();
        files_.insert(std::make_pair(id, file));
        // return the acknowledgement
        return Sequence::Ack{id, req};
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

    Sequence::TransferStatus RemoteFiles::getTransferStatus(Sequence::GetTransferStatus const & req) {
        File * f = get(req.id());
        return Sequence::TransferStatus{req.id(), f->size_, f->received_};
    }

} // namespace tpp