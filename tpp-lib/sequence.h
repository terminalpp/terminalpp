#pragma once

#include <string>

namespace tpp {

    /** A Terminal++ escape sequence. 
     
        Terminal++ escape sequences mask as special OSC sequences so that they are ignored by incompatible terminals. 
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
        static constexpr int Send = 2;
        static constexpr int OpenFile = 3;

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
/*
#if (defined ARCH_UNIX)
        static Sequence Read(int fileno);
        static Sequence WaitAndRead(int fileno, size_t timeout);
#endif
*/

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

        class Send : public Sequence {
        public:
            Send(Sequence && seq);

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

        private:
            int fileId_;
            char const * data_;
            size_t size_;
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

    } // tpp::response

} // namespace tpp
