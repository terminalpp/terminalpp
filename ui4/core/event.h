#pragma once

namespace ui {

    /** Basic event. 
     */
    template<typename PAYLOAD, typename SENDER> 
    class Event {
    public:
        class Payload {
        public:
            void stopPropagation() {
                stopPropagation_ = true;
            }
            
            void preventDefault() {
                preventDefault_ = true;
            }

        private:
            bool stopPropagation_;
            bool preventDefault_;

        };


    }; 


} // namespace ui