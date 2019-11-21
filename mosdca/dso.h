/*
 * DSO.H -- Public definitions for the DSO interfaces.
 */

/* Types. */
typedef void *pointer;

/* Access modes. */
#define DSO_RDONLY		0
#define DSO_RDWR		1
#define DSO_CREATE		2

/* Data types. */
#define	DSO_UBYTE		1
#define	DSO_CHAR		2
#define	DSO_SHORT		3
#define	DSO_USHORT		4
#define	DSO_INT			5
#define	DSO_LONG		6
#define	DSO_REAL		7
#define	DSO_DOUBLE		8

/* Compatibility garbage. */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
