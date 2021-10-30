#ifndef SCAN_H
#define SCAN_H

#include "token-list.h"
#include "scanner.h"

#define SCAN_SUCCESS 0
#define SCAN_FAILURE -1

/**
 * Open the file and initiate the scanner.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return 0 on successful initiation or a negative number on failure.
 */
int init_scan(char* filename);

/**
 * Reads the current file and gets the code of next token.
 * Refer to token-list.h to see which value will be returned.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return The code of the next token or -1 when failed to scan.
 */
int scan(void);

/**
 * This variable is a part of the specification of Task 1.
 */
extern int num_attr;

/**
 * This variable is a part of the specification of Task 1.
 */
extern char string_attr[MAXSTRSIZE];

/**
 * Gets the number of lines that the last read token lies on.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return The number of lines that the last read token lies on
 * or 0 if scan() is not called before.
 */
int get_linenum(void);

/**
 * Close the current file and terminate the scaner.
 *
 * This function is a part of the specification of Task 1.
 */
void end_scan(void);

const scanner_loc_t *get_location();

typedef enum {
    SCAN_WARNING,
    SCAN_ERROR
} scan_message_t;

void print_message(const scanner_loc_t *loc, const scan_message_t type, const char *format, ...);
void print_warning(const scanner_loc_t *loc, const char *format, ...);
void print_error(const scanner_loc_t *loc, const char *format, ...);

void print_token_message(const scanner_loc_t *begin, const scanner_loc_t *end, const scan_message_t type, const char *format, ...);
void print_token_warning(const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...);
void print_token_error(const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...);

#endif