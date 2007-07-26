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
 *
 * When an application creates an instance of some derivative of an iax2_peer,
 * such as an iax2_client or iax2_server, it registers an event handler
 * function.  Passing events to the registered event handlers is the means
 * of communication from the library back to the application.
 */

#ifndef IAX2_EVENT_H
#define IAX2_EVENT_H

enum iax2_event_type {
	/*! \brief Undefined event type */
	IAX2_EVENT_TYPE_UNDEFINED,
	/*! 
	 * \brief A peer has registered.
	 *
	 * Payload type: str, the username
	 */
	IAX2_EVENT_TYPE_REGISTRATION_NEW,
	/*! 
	 * \brief A peer's registration has expired
	 *
	 * Payload type: str, the username
	 */
	IAX2_EVENT_TYPE_REGISTRATION_EXPIRED,
	/*!
	 * \brief Notify the application that a registration had to be retransmitted
	 */
	IAX2_EVENT_TYPE_REGISTRATION_RETRANSMITTED,
	/*!
	 * \brief A call has been established with this peer
	 *
	 * Payload type: str, the remote IP address and port
	 */
	IAX2_EVENT_TYPE_CALL_ESTABLISHED,
	/*!
	 * \brief A call has been hung up
	 *
	 * Payload type: uint, the hangup cause
	 */
	IAX2_EVENT_TYPE_CALL_HANGUP,
	/*!
	 * \brief An audio frame has been received
	 *
	 * Payload type: raw, the audio frame data
	 */
	IAX2_EVENT_TYPE_AUDIO,
	/*!
	 * \brief A video frame has been received
	 *
	 * Payload type: iax2_video_event_payload, the video frame data
	 */
	IAX2_EVENT_TYPE_VIDEO,
	/*!
	 * \brief A text frame for an active call
	 *
	 * Payload type: str, the text
	 */
	IAX2_EVENT_TYPE_TEXT,
	/*!
	 * \brief LAG time has been calculated
	 *
	 * Payload type: uint, LAG time in milliseconds
	 */
	IAX2_EVENT_TYPE_LAG,
};

/*!
 * \brief The data type of the event payload
 */
enum iax2_event_payload_type {
	/*! There is no payload */
	IAX2_EVENT_PAYLOAD_TYPE_NONE,
	/*! Raw data */
	IAX2_EVENT_PAYLOAD_TYPE_RAW,
	/*! The payload is of type, const char *, and is available at payload.str */
	IAX2_EVENT_PAYLOAD_TYPE_STR,
	/*! The payload is of type, unsigned int, and is avaialable at payload.uint */
	IAX2_EVENT_PAYLOAD_TYPE_UINT,
	IAX2_EVENT_PAYLOAD_TYPE_VIDEO,
};

struct iax2_video_event_payload {
	iax2_video_event_payload(const void *frame, size_t frame_len, unsigned short timestamp);
	~iax2_video_event_payload();

	unsigned short m_timestamp;
	size_t m_frame_len;
	const void *m_frame;
};

/*!
 * \brief IAX2 event
 */
class iax2_event {
public:
	iax2_event(enum iax2_event_type t, unsigned short call_num);

	/*!
	 * \brief iax2_event constructor for string type payload
	 *
	 * \param t the event type
	 * \param call_num the call number for this event
	 * \param data the raw data for the payload
	 * \param data_len the raw data length
	 */
	iax2_event(enum iax2_event_type t, unsigned short call_num, 
		const void *data, unsigned int data_len);

	/*!
	 * \brief iax2_event constructor for string type payload
	 *
	 * \param t the event type
	 * \param call_num the call number for this event
	 * \param s the string for the payload
	 */
	iax2_event(enum iax2_event_type t, unsigned short call_num,
		const char *s);

	/*!
	 * \brief iax2_event constructor for unsigned int type payload
	 *
	 * \param t the event type
	 * \param call_num the call number for this event
	 * \param u the unsigned int for the payload
	 */
	iax2_event(enum iax2_event_type t, unsigned short call_num,
		unsigned int u);
	
	iax2_event(enum iax2_event_type t, unsigned short call_num,
		struct iax2_video_event_payload *vid);

	/*!
	 * \brief iax2_event destructor
	 */
	~iax2_event(void);

	/*!
	 * \brief retrieve the event type
	 */
	inline enum iax2_event_type get_type(void) const 
		{ return type; }

	/*!
	 * \brief retrieve the event payload type
	 */
	inline enum iax2_event_payload_type get_payload_type(void) const 
		{ return payload_type; }

	/*!
	 * \brief retrieve the string payload
	 *
	 * \note If this data needs to be saved for later processing, then it must
	 *       be copied instead of simply saving this pointer.  As soon as the
	 *       event handler returns back to the library, the event object will
	 *       be deleted, and its contents will no longer be accessible.
	 */
	inline void *get_payload_raw(void) const 
		{ return payload.raw; }

	/*!
	 * \brief Get the length of the raw payload
	 */
	inline unsigned int get_raw_payload_len(void) const
		{ return raw_payload_len; }

	/*!
	 * \brief retrieve the string payload
	 * 
	 * \note If this data needs to be saved for later processing, then it must
	 *       be copied instead of simply saving this pointer.  As soon as the
	 *       event handler returns back to the library, the event object will
	 *       be deleted, and its contents will no longer be accessible.
	 */
	inline const char *get_payload_str(void) const 
		{ return payload.str; }

	/*!
	 * \brief retrieve the unsigned int payload
	 */
	inline unsigned int get_payload_uint(void) const 
		{ return payload.uint; }

	inline const iax2_video_event_payload *get_payload_video(void) const
		{ return payload.video_frame; }

	/*!
	 * \brief get the call number for the event
	 */
	inline unsigned short get_call_num(void) const 
		{ return call_num; }

	/*!
	 * \brief Return the type as a string
	 */
	const char *type2str(void) const;

	/*!
	 * \brief Print out the contents of the event to stdout.
	 *
	 * This function is for debugging purposes.
	 */
	void print(void) const;

private:
	/*! The event type */
	enum iax2_event_type type;
	/*! The payload type */
	enum iax2_event_payload_type payload_type;
	/*! The call number for this event */
	unsigned short call_num;
	/*! The associated data payload for the event */
	union {
		void *raw;
		const char *str;
		unsigned int uint;
		struct iax2_video_event_payload *video_frame;
	} payload;

	/*!
	 * \brief The length of the raw payload
	 *
	 * \note This is only set when a raw payload is used
	 */
	unsigned int raw_payload_len;
};

#endif /* IAX2_EVENT_H */
