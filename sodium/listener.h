#ifndef _SODIUM_LISTENER_H_
#define _SODIUM_LISTENER_H_

#include <functional>

namespace sodium {

    class Listener {
    public:
        Listener(std::function<void()> k): k(k) {}

        void unlisten() {
            this->k();
        }
    private:
        std::function<void()> k;
    };
    
}

#endif // _SODIUM_LISTENER_H_
