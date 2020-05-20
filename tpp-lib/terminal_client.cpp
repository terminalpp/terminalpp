#include <chrono>

#include "helpers/char.h"

#include "terminal_client.h"

namespace tpp {

    using Char = helpers::Char;
    using Log = helpers::Log;

    // TerminalClient::Async

    TerminalClient::TerminalClient(PTYSlave * pty):
        pty_{pty},
        buffer_{new char[DEFAULT_BUFFER_SIZE]},
        bufferSize_{DEFAULT_BUFFER_SIZE},
        bufferUnprocessed_{0} {
        reader_ = std::thread{[this](){
            while (true) {
                size_t read = pty_->receive(buffer_ + bufferUnprocessed_, bufferSize_ - bufferUnprocessed_);
                if (read == 0)
                    break;
                // process the input
                processInput(buffer_, buffer_ + read + bufferUnprocessed_);
                // TODO grow the buffer if needs be
            }
        }};
    }

    void TerminalClient::processInput(char * start, char const * end) {
        char * i = start;
        size_t unprocessed = 0;
        while (i < end) {
            char const * tppStart = Sequence::FindSequenceStart(i, end);
            // process the data received before tpp sequence found
            if (tppStart != i)
                unprocessed = (tppStart - i) - received(i, tppStart);
            // determine the end of the sequence
            char const * tppEnd = Sequence::FindSequenceEnd(tppStart, end);
            // if there is entire sequence, parse it
            if (tppEnd < end) {
                char const * payloadStart = tppStart + 3;
                tpp::Sequence::Kind kind = tpp::Sequence::ParseKind(payloadStart, end);
                receivedSequence(kind, payloadStart, tppEnd);
                ++tppEnd; // move past the bell character
                // if we are at the end of the input, copy the unprocessed characters to the beginning and return
                if (tppEnd == end) {
                    memmove(buffer_, tppStart - unprocessed, unprocessed);
                    break;
                // otherwise copy the unprocessed characters before the end of the sequence and start analysis from this new beginning. 
                // this will reanalyze the unprocessed characters so is not exactly super effective
                } else {
                    i = const_cast<char*>(tppEnd - unprocessed);
                    memmove(i, tppStart - unprocessed, unprocessed);
                    continue;
                }
            // if there is not entire sequence available, then copy unprocessed data to the beginning of the buffer, then copy the beginning of the tpp sequence, if any and exit
            } else {
                memmove(buffer_, tppStart - unprocessed, unprocessed);
                memmove(buffer_ + unprocessed, tppStart, end - tppStart);
                unprocessed += end - tppStart;
                break;
            }
        }
        bufferUnprocessed_ = unprocessed;
    }

    // TerminalClient::Sync

    Sequence::Capabilities TerminalClient::Sync::getCapabilities(size_t timeout, size_t attempts) {
        Sequence::Capabilities result{0};
        transmit(Sequence::GetCapabilities{}, result, timeout, attempts);
        return result;
    }

    size_t TerminalClient::Sync::openFileTransfer(std::string const & host, std::string const & filename, size_t size, size_t timeout, size_t attempts) {
        Sequence::OpenFileTransfer req{host, filename, size};
        Sequence::Ack result{req, 0};
        transmit(req, result, timeout, attempts);
        return result.id();
    }

    Sequence::TransferStatus TerminalClient::Sync::getTransferStatus(size_t id, size_t timeout, size_t attempts) {
        Sequence::TransferStatus result{id,0,0};
        transmit(Sequence::GetTransferStatus{id}, result, timeout, attempts);
        return result;
    }

    void TerminalClient::Sync::viewRemoteFile(size_t id, size_t timeout, size_t attempts) {
        Sequence::ViewRemoteFile req{id};
        Sequence::Ack result{req, 0};
        transmit(req, result, timeout, attempts);
    }

    void TerminalClient::Sync::receivedSequence(Sequence::Kind kind, char const * payload, char const * payloadEnd) {
        std::lock_guard<std::mutex> g{mSequences_};
        if (responseCheck(kind, payload, payloadEnd)) {
            if (result_->kind() != Sequence::Kind::Nack)
                result_ = nullptr;
            sequenceReady_.notify_one();
        } else {
            // raise the event
            NOT_IMPLEMENTED;
        }
    }

    void TerminalClient::Sync::transmit(Sequence const & send, Sequence & receive, size_t timeout, size_t attempts) {
        std::unique_lock<std::mutex> g{mSequences_};
        ASSERT(result_ == nullptr) << "Only one thread is allowed to transmit t++ sequences";
        result_ = & receive;
        request_ = & send;
        while (attempts_ > 0) {
            this->send(send);
            auto timeoutTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
            while (true) {
                if (timeout > 0) {
                    if (sequenceReady_.wait_until(g, timeoutTime) == std::cv_status::timeout) {
                        if (--attempts == 0)
                            THROW(TimeoutError());
                        LOG(Log::Verbose) << "Request timeout, remaining attempts: " << attempts;
                        break;
                    }
                } else {
                    sequenceReady_.wait(g);
                }
                if (result_ == nullptr) {
                    return;
                } else if (result_->kind() == Sequence::Kind::Nack) {
                    std::string reason = dynamic_cast<Sequence::Nack*>(result_)->reason();
                    delete result_;
                    result_ = nullptr;
                    THROW(NackError()) << reason;
                }
            }
        }
    }

    bool TerminalClient::Sync::responseCheck(Sequence::Kind kind, char const * payload, char const * payloadEnd) {
        if (result_ != nullptr && (result_->kind() == kind || kind == Sequence::Kind::Nack)) {
            switch (kind) {
                case Sequence::Kind::Ack: {
                    Sequence::Ack * result = dynamic_cast<Sequence::Ack*>(result_);
                    Sequence::Ack x{payload, payloadEnd};
                    if (result->request() != x.request())
                        return false;
                    (*result) = x;
                    return true;
                }
                case Sequence::Kind::Nack: {
                    Sequence::Nack * x = new Sequence::Nack{payload, payloadEnd};
                    std::string request = STR(*request_);
                    if (request != x->request())
                        return false;
                    result_ = x;
                    return true;
                }
                case Sequence::Kind::Capabilities: {
                    Sequence::Capabilities * result = dynamic_cast<Sequence::Capabilities*>(result_);
                    (*result) = Sequence::Capabilities{payload, payloadEnd};
                    return true;
                }
                case Sequence::Kind::TransferStatus: {
                    Sequence::TransferStatus * result = dynamic_cast<Sequence::TransferStatus*>(result_);
                    Sequence::TransferStatus x{payload, payloadEnd};
                    if (result->id() != x.id())
                        return false;
                    (*result) = x;
                    return true;
                }
                default:
                    return false;
            }
        } else {
            return false;
        }
    }

} // namespace tpp