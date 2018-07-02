/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef _CSKY_BYTEORDER_H
#define _CSKY_BYTEORDER_H

#if defined(__cskyBE__)
#include <linux/byteorder/big_endian.h>
#elif defined(__cskyLE__)
#include <linux/byteorder/little_endian.h>
#else
# error "csky, but neither __cskyBE__, nor __cskyLE__???"
#endif

#endif /* _CSKY_BYTEORDER_H */
