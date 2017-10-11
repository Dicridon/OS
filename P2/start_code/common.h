/*	common.h

	Common definitions and types.
	Best viewed with tabs set to 4 spaces.
*/

#ifndef COMMON_H
#define COMMON_H

#ifndef NULL
#define NULL ((void*) 0)
#endif

//      Physical address of the text mode display 
#define SCREEN_ADDR ((short *) 0xbfe48000)

//      Used in threads and processes as argument for delay
#define DELAY_VAL 2

/*	If the expression p fails, print the source file and 
	line number along with the text s. Then hang the os. 

	Processes should use this macro.
*/
#define ASSERT2(p, s) \
	do { \
	if (!(p)) { \
		print_str(0, 13, "Assertion failure: "); \
		print_str(0, 14, s); \
		print_str(0, 15, "file: "); \
		print_str(0, 16, __FILE__); \
		print_str(0, 17, "line: "); \
		print_int(0, 18, __LINE__); \
		while (1); \
	} \
	} while(0)

/*	The #p tells the compiler to copy the expression p 
	into a text string and use this as an argument. 
	=> If the expression fails: print the expression as a message. 
*/
#define ASSERT(p) ASSERT2(p, #p)

//      Gives us the source file and line number where we decide to hang the os. 
#define HALT(s) ASSERT2(FALSE, s)

//      Typedefs of commonly used types
typedef enum {
    FALSE, TRUE
} bool_t;

typedef signed char int8_t;
typedef short int int16_t;
typedef long int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

/* Processor speed of the fishbowl machines */
enum {
    MHZ = 250,
};

#endif
