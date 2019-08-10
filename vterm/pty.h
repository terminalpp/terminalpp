#pragma once 

#include "helpers/process.h"

namespace vterm {

    /** The pseudoterminal connection interface. 

        A pseudoterminal connection provides the simplest possible interface to the target process. 
     */
    class PTY {
    public:
        /** Sends the given buffer to the target process. 
         */
        virtual void send(char const * buffer, size_t bufferSize) = 0;

        /** Receives up to bufferSize bytes which are stored in the provided buffer.
         
            Returns the number of bytes received. If there are no data available, blocks until the data is ready and then returns them. 

            If the attached process is terminated, should return immediately with 0 bytes read. 
         */
        virtual size_t receive(char * buffer, size_t bufferSize) = 0;

        /** Terminates the target process and returns immediately. 
         
            If the target process has already terminated, simply returns. 
         */
        virtual void terminate() = 0;

        /** Waits for the attached process to terminate and returns its exit code. 
         */
        virtual helpers::ExitCode waitFor() = 0;

        virtual void resize(int cols, int rows) = 0;

		/** Virtual destructor. 
		 */
		virtual ~PTY() {
		}


    }; // vterm::PTY



} // namespace vterm