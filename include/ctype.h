// Copyright 2019 @htz. Released under the MIT license.

#ifndef __CTYPE_H
#define __CTYPE_H

/* iscntrl
 * control character
 */
#define iscntrl(c) \
       (((c >= 0) && (c <= 0x1f)) || (c == 0x7f))

/* isdigit
 * decimal character
 */
#define isdigit(c) \
       ((c >= 0x30) && (c <= 0x39))

/* isgraph
 * printing characters except
 * space
 */
#define isgraph(c) \
       ((c > 0x20) && (c <= 0x7e))

/* islower
 * lower-case letter
 */
#define islower(c) \
       ((c >= 0x61) && (c <= 0x7a))

/* isprint
 * printing character including space
 */
#define isprint(c) \
       ((c >= 0x20) && (c <= 0x7e))

/* ispunct
 * printing character except space or letter
 * or digit
 */
#define ispunct(c) \
       (((c > 0x20) && (c <= 0x7e)) && !isalnum(c))

/* isspace
 * space, formfeed, newline, carriage return
 * tab, vertical tab
 */
#define isspace(c) \
       ((c == 0x20) || ((c >= 0x09) && (c <= 0x0d)))

/* isupper
 * upper-case letter
 */
#define isupper(c) \
       ((c >= 0x41) && (c <= 0x5a))

/* isxdigit
 * hexadecimal digit
 */
#define isxdigit(c) \
       (isdigit(c) || (((c >= 0x41) && (c <= 0x46)) || ((c >= 0x61) && (c <= 0x66))))


/* isalnum
 * isalpha or isdigit == true
 */
#define isalnum(c) \
      (isalpha(c) || isdigit(c))

/* isalpha
 * isupper or islower == true
 */
#define isalpha(c) \
      (isupper(c) || islower(c))

/* tolower
 * convert c to lower case
 */
#define tolower(c) \
       (isupper(c) ? (c + 0x20) : (c))

/* toupper
 * convert c to upper case
 */
#define toupper(c) \
       (islower(c) ? (c - 0x20) : (c))

#endif
