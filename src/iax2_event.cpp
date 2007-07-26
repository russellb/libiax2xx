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
 * \brief IAX2 events
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

using namespace std;

#include "iax2/iax2_event.h"

iax2_event::iax2_event(enum iax2_event_type t, unsigned short num) :
	type(t), payload_type(IAX2_EVENT_PAYLOAD_TYPE_NONE), call_num(num)
{
	payload.raw = NULL;
}

iax2_event::iax2_event(enum iax2_event_type t, unsigned short num,
	const void *data, unsigned int data_len) : 
	type(t), payload_type(IAX2_EVENT_PAYLOAD_TYPE_RAW), call_num(num),
	raw_payload_len(data_len)
{
	if (!(payload.raw = malloc(data_len)))
		return;
	
	memcpy(payload.raw, data, data_len);
}

iax2_event::iax2_event(enum iax2_event_type t, unsigned short num,
	const char *s) : 
	type(t), payload_type(IAX2_EVENT_PAYLOAD_TYPE_STR), call_num(num)
{
	payload.str = s ? strdup(s) : NULL;
}

iax2_event::iax2_event(enum iax2_event_type t, unsigned short num,
	unsigned int u) : 
	type(t), payload_type(IAX2_EVENT_PAYLOAD_TYPE_UINT), call_num(num)
{
	payload.uint = u;
}

iax2_event::iax2_event(enum iax2_event_type t, unsigned short num,
	struct iax2_video_event_payload *vid) : 
	type(t), payload_type(IAX2_EVENT_PAYLOAD_TYPE_VIDEO), call_num(num)
{
	payload.video_frame = vid;
}

iax2_event::~iax2_event(void)
{
	if (payload_type == IAX2_EVENT_PAYLOAD_TYPE_RAW && payload.raw)
		free(payload.raw);
	else if (payload_type == IAX2_EVENT_PAYLOAD_TYPE_STR && payload.str)
		free((void *) payload.str);
	else if (payload_type == IAX2_EVENT_PAYLOAD_TYPE_VIDEO && payload.video_frame)
		delete payload.video_frame;
}

const char *iax2_event::type2str(void) const
{
	const char *str;
#define ST(x) case x: str = # x; break;
	switch (type) {
	ST(IAX2_EVENT_TYPE_UNDEFINED)
	ST(IAX2_EVENT_TYPE_REGISTRATION_NEW)
	ST(IAX2_EVENT_TYPE_REGISTRATION_EXPIRED)
	ST(IAX2_EVENT_TYPE_REGISTRATION_RETRANSMITTED)
	ST(IAX2_EVENT_TYPE_CALL_ESTABLISHED)
	ST(IAX2_EVENT_TYPE_CALL_HANGUP)
	ST(IAX2_EVENT_TYPE_AUDIO)
	ST(IAX2_EVENT_TYPE_VIDEO)
	ST(IAX2_EVENT_TYPE_TEXT)
	ST(IAX2_EVENT_TYPE_LAG)
	default:
		str = "Unknown Type, this is bad.";
	}
#undef ST
	return str;
}

void iax2_event::print(void) const
{
	printf("[IAX2-Event] Type: %s  Payload: ", type2str());

	switch (payload_type) {
	case IAX2_EVENT_PAYLOAD_TYPE_NONE:
		printf("(none)\n\n");
		break;
	case IAX2_EVENT_PAYLOAD_TYPE_RAW:
		printf("Raw payload of length '%d'\n\n", get_raw_payload_len());
		break;
	case IAX2_EVENT_PAYLOAD_TYPE_STR:
		printf("%s\n\n", payload.str);
		break;
	case IAX2_EVENT_PAYLOAD_TYPE_UINT:
		printf("%u\n\n", payload.uint);
		break;
	case IAX2_EVENT_PAYLOAD_TYPE_VIDEO:
		printf("Video frame, Len: %u, Timestamp: %u\n\n", 
			payload.video_frame->m_frame_len, payload.video_frame->m_timestamp);
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////

iax2_video_event_payload::iax2_video_event_payload(const void *frame, size_t frame_len,
	unsigned short timestamp)
{
	m_frame = malloc(frame_len);
	memcpy((void *) m_frame, frame, frame_len);
	m_frame_len = frame_len;
	m_timestamp = timestamp;
}

iax2_video_event_payload::~iax2_video_event_payload()
{
	if (m_frame)
		free((void *) m_frame);
}
