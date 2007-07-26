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
 * \brief IAX2 commands
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#include "iax2/iax2_command.h"

iax2_command::iax2_command(enum iax2_command_type t, unsigned short num) :
	call_num(num), type(t), payload_type(IAX2_COMMAND_PAYLOAD_TYPE_NONE)
{
}
	
iax2_command::iax2_command(enum iax2_command_type t, unsigned short num, 
	const void *raw, unsigned int raw_len) : 
	call_num(num), type(t), payload_type(IAX2_COMMAND_PAYLOAD_TYPE_RAW),
	raw_datalen(raw_len)
{
	if (!(payload.raw = malloc(raw_len))) {
		raw_datalen = 0;
		return;
	}
	memcpy(payload.raw, raw, raw_len);
}

iax2_command::iax2_command(enum iax2_command_type t, unsigned short num, 
	const char *s) : 
	call_num(num), type(t), payload_type(IAX2_COMMAND_PAYLOAD_TYPE_STR)
{
	payload.str = s ? strdup(s) : NULL;
}

iax2_command::iax2_command(enum iax2_command_type t, unsigned short num, 
	unsigned int load) : 
	call_num(num), type(t), payload_type(IAX2_COMMAND_PAYLOAD_TYPE_UINT)
{
	payload.uint = load;
}

iax2_command::~iax2_command(void)
{
	if (payload_type == IAX2_COMMAND_PAYLOAD_TYPE_RAW && payload.raw)
		free(payload.raw);
	else if (payload_type == IAX2_COMMAND_PAYLOAD_TYPE_STR && payload.str)
		free((void *) payload.str);
}

const char *iax2_command::type2str(void) const
{
	const char *str;
#define ST(x) case x: str = # x; break;
	switch (type) {
	ST(IAX2_COMMAND_TYPE_UNKNOWN)
	ST(IAX2_COMMAND_TYPE_NEW)
	ST(IAX2_COMMAND_TYPE_HANGUP)
	ST(IAX2_COMMAND_TYPE_AUDIO)
	ST(IAX2_COMMAND_TYPE_VIDEO)
	ST(IAX2_COMMAND_TYPE_TEXT)
	ST(IAX2_COMMAND_TYPE_LAGRQ)
	ST(IAX2_COMMAND_TYPE_SHUTDOWN)
	default:
		str = "Unknown Type, this is bad.";
	}
#undef ST
	return str;
}

void iax2_command::print(void) const
{
	printf("[IAX2-Command] Type: %s  Payload: ", type2str());

	switch (payload_type) {
	case IAX2_COMMAND_PAYLOAD_TYPE_NONE:
		printf("(none)\n\n");
		break;
	case IAX2_COMMAND_PAYLOAD_TYPE_STR:
		printf("%s\n\n", payload.str);
		break;
	case IAX2_COMMAND_PAYLOAD_TYPE_UINT:
		printf("%u\n\n", payload.uint);
		break;
	}
}
