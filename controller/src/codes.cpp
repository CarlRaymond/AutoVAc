#include "codes.h"

bool isValidCode(Code c) {
    switch (c) {
        //case Code::NONE:
        case Code::TOOL_STARTING:
        case Code::TOOL_RUNNING:
        case Code::START:
        case Code::STOP:
        case Code::CODE_SEQ_TIMEOUT:
        case Code::TOOL_QUIET_TIMEOUT:
        case Code::SHUTOFF_TIMEOUT:
            return true;
    }
    return false;
}