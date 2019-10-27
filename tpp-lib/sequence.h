#pragma once

#include <string>

namespace tpp {

    /** A Terminal++ escape sequence. 
     
     */
    class Sequence {
    public:

        Sequence(Sequence && from):
            id_(from.id_),
            payload_(std::move(from.payload_)) {
        }

        static constexpr int Invalid = -2;
        static constexpr int Incomplete = -1;
        static constexpr int Capabilities = 0;
        static constexpr int NewFile = 1;
        static constexpr int Data = 2;
        static constexpr int TransferStatus = 3;
        static constexpr int OpenFile = 4;

        bool complete() const {
            return id_ >= 0;
        }

        bool valid() const {
            return id_ != Invalid;
        }

        int id() const {
            return id_;
        }

        std::string const & payload() const {
            return payload_;
        }

        static Sequence Parse(char * & start, char const * end); 

    protected:

        int id_;
        std::string payload_;

    private:

        friend class Terminal;

        Sequence():
            id_{Invalid},
            payload_{} {
        }

        Sequence(int id, std::string && payload):
            id_{id},
            payload_(std::move(payload)) {
        }

        friend std::ostream & operator << (std::ostream & s, Sequence const & seq) {
            s << "T++ sequence id " << seq.id() << ", payload: " << seq.payload();
            return s;
        } 

    };

    namespace request {
        
        class NewFile : public Sequence {
        public:
            NewFile(Sequence &&);

            bool valid() const {
                return size_ > 0;
            }

            size_t size() const {
                return size_;
            }

            std::string const & hostname() const {
                return hostname_;
            }

            std::string const & filename() const {
                return filename_; 
            }

            std::string const & remotePath() const {
                return remotePath_;
            }

        private:
            size_t size_;
            std::string filename_;
            std::string hostname_;
            std::string remotePath_;
        }; 

        class Data : public Sequence {
        public:
            Data(Sequence && seq);

            bool valid() const {
                return fileId_ != -1;
            }

            int fileId() const {
                return fileId_;
            }

            /** Raw encoded data. 
             */
            char const * data() const {
                return data_;
            }

            size_t size() const {
                return size_;
            }

            /** Offset at which the data should be stored. 
             */
            size_t offset() const {
                return offset_;
            }

        private:
            int fileId_;
            char const * data_;
            size_t size_;
            size_t offset_;
        };

        class TransferStatus : public Sequence {
        public:
            TransferStatus(Sequence && seq);

            bool valid() const {
                return fileId_ >= 0;
            }

            int fileId() const {
                return fileId_;
            }

        private:
            int fileId_;
        }; 

        class OpenFile : public Sequence {
        public:
            OpenFile(Sequence && seq);

            bool valid() const {
                return fileId_ >= 0;
            }

            int fileId() const {
                return fileId_;
            }

        private:
            int fileId_;
        }; 

    } // tpp::request

    namespace response {

        class Capabilities : public Sequence {
        public:

            Capabilities(Sequence && from);

            bool valid() const {
                return version_ >= 0;
            }

            int version() const {
                return version_;
            }

        private:
            int version_;
        };

        class NewFile : public Sequence {
        public:
            NewFile(Sequence && from);

            bool valid() const {
                return fileId_ >= 0;
            }

            int fileId() const {
                return fileId_;
            }

        private:
            int fileId_;
        };

        class TransferStatus : public Sequence {
        public:
            TransferStatus(Sequence && from);

            bool valid() const {
                return fileId_ >= 0;
            }

            int fileId() const {
                return fileId_;
            }

            size_t transmittedBytes() const {
                return transmittedBytes_;
            }
        private:
            int fileId_;
            size_t transmittedBytes_;

        };

    } // tpp::response

} // namespace tpp
