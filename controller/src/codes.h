
enum class Code : unsigned char {

    // Codes corresponding to pin inputs from the RF receiver
    NONE          = 0b0000,
    BUTTON_A      = 0b0001,  // Keyfob button A
    START         = 0b0001,
    BUTTON_B      = 0b0010,  // Keyfob button B
    STOP          = 0b0010,
    BUTTON_C      = 0b0100,  // Keyfob button C
    BUTTON_D      = 0b1000,  // Keyfob button D
    TOOL_STARTING = 0b1011,  // Tool starting to run
    TOOL_RUNNING  = 0b1010,  // Tool continuing to run
    MASK          = 0b1111,  

    // Pseudo-codes corresponding to timer events
    CODE_SEQ_TIMEOUT   = 0b00010000, // No code received for a short interval
    TOOL_QUIET_TIMEOUT = 0b00010001, // No STARTING or RUNNING code received for a while
    SHUTOFF_TIMEOUT    = 0b00010010, // Motor has been running a long time
};

bool isValidCode(Code c);