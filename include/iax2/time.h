/*
 * Copyright (C) 2006, Russell Bryant <russell@russellbryant.net> 
 *
 * This file is part of LibIAX2xx.
 *
 * LibIAX2xx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LibIAX2xx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibIAX2xx; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*!
 * \file
 * \author Russell Bryant <russell@russellbryant.net>
 *
 * \brief timeval helpers
 *
 * This timeval handling code were borrowed from Asterisk (http://www.asterisk.org). 
 * The relevant code has the following copyright information: 
 * Copyright (C) 1999 - 2006, Digium, Inc. 
 */

#ifndef IAX2_TIME_H
#define IAX2_TIME_H

#include <sys/time.h>

namespace iax2xx {

/* We have to let the compiler learn what types to use for the elements of a
   struct timeval since on linux, it's time_t and suseconds_t, but on *BSD,
   they are just a long. */
extern struct timeval _tv;
typedef typeof(_tv.tv_sec) iax2xx_time_t;
typedef typeof(_tv.tv_usec) iax2xx_suseconds_t;
 
/*!
 * \brief Returns current timeval. Meant to replace calls to gettimeofday().
 */
static inline struct timeval tvnow(void)
{
        struct timeval t;
        gettimeofday(&t, NULL);
        return t;
}   

/*!
 * \brief Computes the difference (in milliseconds) between two \c struct \c timeval instances.
 *
 * \param end the beginning of the time period
 * \param start the end of the time period
 *
 * \return the difference in milliseconds
 */  
static inline int tvdiff_ms(struct timeval end, struct timeval start)
{
        /* the offset by 1,000,000 below is intentional...
           it avoids differences in the way that division
           is handled for positive and negative numbers, by ensuring
           that the divisor is always positive
        */
        return  ((end.tv_sec - start.tv_sec) * 1000) +
                (((1000000 + end.tv_usec - start.tv_usec) / 1000) - 1000);
}    

/*!
 * \brief Returns a timeval from sec, usec
 */
static inline struct timeval create_tv(iax2xx_time_t sec, iax2xx_suseconds_t usec)
{
	struct timeval t;
	t.tv_sec = sec;
	t.tv_usec = usec;
	return t;
} 

/*!
 * \brief Returns a timeval corresponding to the duration of n samples at rate r.
 * Useful to convert samples to timevals, or even milliseconds to timevals
 * in the form ast_samp2tv(milliseconds, 1000)
 */
static inline struct timeval samp2tv(unsigned int _nsamp, unsigned int _rate)
{
        return create_tv(_nsamp / _rate, (_nsamp % _rate) * (1000000 / _rate));
}

/*!
 * \brief Returns the sum of two timevals a + b
 */
struct timeval tvadd(struct timeval a, struct timeval b);

/*!
 * \brief Returns the difference of two timevals a - b
 */
struct timeval tvsub(struct timeval a, struct timeval b);

}; // namespace iax2xx

#endif /* IAX2_TIME_H */
