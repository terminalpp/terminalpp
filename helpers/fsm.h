#pragma once

#include <unordered_map>

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

    template<typename T, typename IT>
    class MatchingFSM {
    public:

        bool empty() {
            return start_.final_ == false && start_.next_.empty();
        }

        void addMatch(IT const * begin, IT const * end, T const & result, bool overwrite = false) {
            start_.addMatch(begin, end, result, overwrite);
        }

        void addMatch(IT const * begin, T const & result, bool overwrite = false) {
            start_.addMatch(begin, result, overwrite);
        }

        bool match(IT const * & begin, IT const * end, T & result) const {
            return start_.match(begin, end, result, begin, false);
        }

    private:
        class Node {
            friend class MatchingFSM;
        public:

            Node() = default;

            Node(Node const & other):
                final_{other.final},
                result_{other.result_} {
                for (auto i : other.next_)
                    next_.insert(std::make_pair(i.first, new Node(*i->second)));
            }

            Node(Node && other) noexcept:
                next_{std::move(other.next_)},
                final_{other.final_},
                result_{std::move(other.result_)} {
            }

            ~Node() {
                for (auto i : next_)
                    delete i.second;
            }

            void addMatch(IT const * begin, IT const * end, T const & result, bool overwrite) {
                if (begin == end) {
                    ASSERT(! final_ || overwrite) << "Ambiguous match";
                    final_ = true;
                    result_ = result;
                } else {
                    auto i = next_.find(*begin);
                    if (i == next_.end())
                        i = next_.insert(std::make_pair(*begin, new Node{})).first;
                    i->second->addMatch(begin + 1, end, result, overwrite);
                }
            }

            void addMatch(IT const * begin, T const & result, bool overwrite) {
                if (*begin == 0) {
                    ASSERT(! final_ || overwrite) << "Ambiguous match";
                    final_ = true;
                    result_ = result;
                } else {
                    auto i = next_.find(*begin);
                    if (i == next_.end())
                        i = next_.insert(std::make_pair(*begin, new Node{})).first;
                    i->second->addMatch(begin + 1, result, overwrite);
                }
            }

            bool match(IT const * begin, IT const * end, T & match, IT const * & matchEnd, bool result) const {
                if (final_) {
                    match = result_;
                    matchEnd = begin;
                    result = true;
                }
                auto i = next_.find(*begin);
                if (i == next_.end())
                    return result;
                else
                    return i->second->match(begin + 1, end, match, matchEnd, result);
            }

        private:

            std::unordered_map<IT, Node*> next_;
            bool final_ = false;
            T result_;

        }; // MatchingFSM::Node

        Node start_;

    }; // MatchingFSM

HELPERS_NAMESPACE_END