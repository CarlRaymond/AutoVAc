
enum class Code : unsigned char {

    // Codes corresponding to pin inputs
    NONE     = 0b0000,
    A        = 0b1000,
    START    = 0b1000,
    B        = 0b0100,
    STOP     = 0b0100,
    C        = 0b0100,
    D        = 0b1000,
    STARTING = 0b1011,
    RUNNING  = 0b1001,
    MASK     = 0b1111,

    // Pseudo-codes corresponding to timer events
    TIMEOUT  = 0b00010000
};

bool isValidCode(Code c);