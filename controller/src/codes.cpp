#include "codes.h"

bool isValidCode(Code c) {
    switch (c) {
        //case Code::NONE:
        case Code::STARTING:
        case Code::RUNNING:
        case Code::START:
        case Code::STOP:
        case Code::STARTING_TIMEOUT:
        case Code::RUNNING_TIMEOUT:
            return true;
    }
    return false;
}