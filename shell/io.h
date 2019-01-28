#define BOARD_SIZE   100
#define MAX_POSITION (BOARD_SIZE-1)

void write_position(int x, int y);

int parse_position(char *buffer, int *x, int *y);

void read_position(const char *who, int *x, int *y);

int read_guidance(const char *who);
#define HOTTER 1
#define COLDER (-1)
#define STEADY 0
#define WINNER 100
