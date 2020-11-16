#pragma once

#include "helpers/helpers.h"

namespace ui {

    /** Simple & fast fsm that matches urls. 
     
        Matches urls on character by character basis. Does not keep the matched url, just its length as the url is reconstructed from the terminal. The matcher starts in a ready state, if an invalid character is matched, it goes to either invalid state, or to ready state back. 

        The invalid state denotes that new url can't be matched, but separator must be matched first, i.e. things like `foohttp://` won't be matched. 

        Http, https, port, hostnames and ip addresses, path on the host and url arguments. The matcher is not exact and should match anything that looks like an url. 
     */
    class UrlMatcher {
    public:

        /** Resets the matcher's state to ready.
         
            If the matcher was in valid state, returns the length of the matched url. Returning 0 means that there was no valid url. 
         */
        size_t reset() {
            size_t result = isStateValid() ? matchSize_ : 0;
            matchSize_ = 0;
            state_ = State::ready;
            return result;
        }

        /** Matches next character. 
         
            If the next character invalidates a valid current state, returns the length of the url that was matched so far. Otherwise returns 0. 
         */
        size_t next(char32_t c) {
            size_t result = isStateValid() ? matchSize_ : 0;
            transition(c);
            // match longest valid sequence
            if (state_ != State::invalid && state_ != State::ready) 
                result = 0;
            return result;
        }

        /** Returns true if given string is a valid url. 
         */
        static bool IsValid(std::string const & text) {
            UrlMatcher matcher;
            for (char c : text)
                matcher.next(c);
            return matcher.reset() == text.size();
        }

        /** Matches for first url in given string and returns the position of the first character of the url and the character after it. 
         
            If no url is found, returns a pair of two lengths of the input string.
         */
        static std::pair<size_t, size_t> Match(std::string const & text) {
            UrlMatcher matcher;
            for (size_t i = 0, e = text.size(); i != e; ++i) {
                size_t size = matcher.next(text[i]);
                if (size != 0)
                    return std::make_pair(i - size, i);
            }
            return std::make_pair(text.size(), text.size());
        }

    private:
        /** Internal state. 
         */
        enum class State {
            ready, // ready state where we can start matching http
            h,
            ht,
            htt,
            http,
            https,
            protocol1, // http[s]:
            protocol2, // http[s]:/
            domain1, // http[s]:// domain_letters
            domain2, // https[s]:// {valid_char}
            domainSeparator, // https[s]:// {{valid_char} .} 
            port1, // hostname: 
            percentEscape1, // after %
            percentEscape2, // after % and hexadecimal

            invalid, // state where we wait for separator to move to ready
            valid, // sentinel, any state after this one is in fact valid

            validDomain,
            validPort, // at least one nunber after port valid
            validAddress,
            validArguments,
        }; 

        bool isStateValid() const {
            return state_ >= State::valid;
        }

        /** \name Character Groups
         
            The character group functios return the char itself so that they can be used in the TRANSITION macro and compared agains the char itself. They return the character if the character belongs to the group and a value different from the character otherwise. 
         */

        char32_t domainLetter(char32_t c) {
            if (c >= 'a' && c <= 'z')
                return c;
            else if (c >= '0' && c <= '9')
                return c;
            else if (c == '-')
                return c;
            else 
                return c + 1;
        }

        char32_t number(char32_t c) {
            if (c >= '0' && c <= '9')
                return c;
            else 
                return c + 1;
        }

        char32_t hexadecimal(char32_t c) {
            if (c >= '0' && c <= '9')
                return c;
            else if (c >= 'a' && c <= 'f')
                return c;
            else if (c >= 'A' && c <= 'F')
                return c;
            else 
                return c + 1;
        }

        char32_t addressLetter(char32_t c) {
            if (c >= 'a' && c <= 'z')
                return c;
            else if (c >= '0' && c <= '9')
                return c;
            else if (c == '-' || c == '~' || c == '_')
                return c;
            else 
                return c + 1;
        }

        char32_t argsLetter(char32_t c) {
            if (c >= 'a' && c <= 'z')
                return c;
            else if (c >= 'A' && c <= 'Z')
                return c;
            else if (c >= '0' && c <= '9')
                return c;
            else if (c == '-' || c == '~' || c == '+' || c == '_' || c == '!' || c == '*' || c == '\'' || c == '(' || c == ')' || c == '=')
                return c;
            else 
                return c + 1;
        }

        /** Separators of urls. 
         
            The url must start after a separator character. 
         */
        bool isSeparator(char32_t c) {
            switch (c) {
                case ' ':
                case ',':
                case '{':
                case '}':
                case '[':
                case ']':
                case '|':
                case ':':
                case ';':
                case '-':
                case '=':
                case '!':
                case '?':
                case '\t':
                case '\"':
                case '\'':
                    return true;
                default:
                    return false;
            }
        }

        //@}

        /** Performs a transition from current state, given the input character. 
         */
        #define TRANSITION(INPUT, STATE) if (c == INPUT) { state_ = STATE; ++matchSize_; return; }
        void transition(char32_t c) {
            switch (state_) {
                case State::ready: {
                    TRANSITION('h', State::h);
                    break;
                }
                case State::h: {
                    TRANSITION('t', State::ht);
                    break;
                }
                case State::ht: {
                    TRANSITION('t', State::htt);
                    break;
                }
                case State::htt: {
                    TRANSITION('p', State::http);
                    break;
                }
                case State::http: {
                    TRANSITION('s', State::https);
                    TRANSITION(':', State::protocol1);
                    break;
                }
                case State::https: {
                    TRANSITION(':', State::protocol1);
                    break;
                }
                case State::protocol1: {
                    TRANSITION('/', State::protocol2);
                    break;
                }
                case State::protocol2: {
                    TRANSITION('/', State::domain1);
                    break;
                }
                // this is the first domain state, at least one valid domain character required
                case State::domain1: {
                    TRANSITION(domainLetter(c), State::domain2);
                    break;
                }
                // first domain after first character, others may follow
                case State::domain2: {
                    TRANSITION(domainLetter(c), State::domain2);
                    TRANSITION('.', State::domainSeparator);
                    break;
                }
                // domain separator was detected, valid domain character after it is validDomain
                case State::domainSeparator: {
                    TRANSITION(domainLetter(c), State::validDomain);
                    break;
                }
                case State::port1: {
                    TRANSITION(number(c), State::validPort);
                    break;
                }
                case State::percentEscape1: {
                    TRANSITION(hexadecimal(c), State::percentEscape2);
                    break;
                }
                case State::percentEscape2: {
                    TRANSITION(hexadecimal(c), State::validArguments);
                    break;
                }
                // valid states
                case State::validDomain: {
                    TRANSITION(domainLetter(c), State::validDomain);
                    TRANSITION('.', State::domainSeparator);
                    TRANSITION('?', State::validArguments);
                    TRANSITION('/', State::validAddress);
                    TRANSITION(':', State::port1);
                    break;
                }
                case State::validPort: {
                    TRANSITION(number(c), State::validPort);
                    TRANSITION('?', State::validArguments);
                    TRANSITION('/', State::validAddress);
                    break;
                }
                case State::validAddress: {
                    TRANSITION(addressLetter(c), State::validAddress);
                    TRANSITION('/', State::validAddress);
                    break;
                }
                case State::validArguments: {
                    TRANSITION(argsLetter(c), State::validArguments);
                    TRANSITION('%', State::percentEscape1);
                    TRANSITION('&', State::validArguments); // there does not have to be anything after &
                    break;
                }
                default:
                    break;
            }
            state_ = isSeparator(c) ? State::ready : State::invalid;
            matchSize_ = 0;
        }
        #undef TRANSITION

        State state_ = State::ready;
        size_t matchSize_ = 0;

    }; // ui::UrlMatcher

} // namespace ui