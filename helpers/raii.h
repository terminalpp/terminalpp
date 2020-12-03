#include <vector>
#include <functional>

#include "helpers.h"


HELPERS_NAMESPACE_BEGIN

    /** Scoped cleaner for situations where no simple RAII solutions exists. 
     
        Contains a simple list of cleanup handlers that are associated with the cleaner. When the cleaner goes out of scope, these handlers are executed in reverese order. 
     */
    class RAIICleaner {
    public:

        /** Creates the cleaner with no handlers. 
         */
        RAIICleaner() = default;

        /** Creates the cleaner and adds a handler. 
         */
        explicit RAIICleaner(std::function<void()> task) {
            add(task);
        }

        /** Deletes the cleaner and executes the associated handlers. 
         */
        ~RAIICleaner() {
            for (auto i = tasks_.rbegin(), e = tasks_.rend(); i != e; ++i) {
                (*i)();
            }
        }

        /** Adds cleanup handler. 
         */
        RAIICleaner & add(std::function<void()> task) {
            tasks_.push_back(task);
            return *this;
        }

    private:
        std::vector<std::function<void()>> tasks_;
    };


HELPERS_NAMESPACE_END