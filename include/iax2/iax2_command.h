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
 *
 * Commands are what the application uses to communicate back to the library.
 */

#ifndef IAX2_COMMAND_H
#define IAX2_COMMAND_H

/*!
 * \brief Commands that can be passed to an iax2_command_handler
 */
enum iax2_command_type {
	/*! Undefined or unknown command */
	IAX2_COMMAND_TYPE_UNKNOWN,
	/*! Start a new call.
	 *  \note This should not be used directly by the application using this
	 *        library.  The new_call() function should be used, instead.
	 */
	IAX2_COMMAND_TYPE_NEW,
	/*! Hangup a call */
	IAX2_COMMAND_TYPE_HANGUP,
	/*! Send an audio frame */
	IAX2_COMMAND_TYPE_AUDIO,
	/*! Send a video frame */
	IAX2_COMMAND_TYPE_VIDEO,
	/*! Send text */
	IAX2_COMMAND_TYPE_TEXT,
	/*! 
	 * \brief Initiate a Lag request.
	 * 
	 * A LAGRQ will be sent to the appropriate peer.  Once the dialog is
	 * complete, the result will be passed back to the application as an event,
	 * with the payload indicating the round trip time of the request in
	 * milliseconds.
	 */
	IAX2_COMMAND_TYPE_LAGRQ,
	/*! Shutdown the peer, causing the run() funciton to return. */
	IAX2_COMMAND_TYPE_SHUTDOWN,
};

/*!
 * \brief The data type of the command payload
 */
enum iax2_command_payload_type {
	/*! There is no payload */
	IAX2_COMMAND_PAYLOAD_TYPE_NONE,
	/*! Raw payload data */
	IAX2_COMMAND_PAYLOAD_TYPE_RAW,
	/*! The payload is of type, const char *, and is available at payload.str */
	IAX2_COMMAND_PAYLOAD_TYPE_STR,
	/*! The payload is of type, unsigned int, and is avaialable at payload.uint */
	IAX2_COMMAND_PAYLOAD_TYPE_UINT,
};

/*!
 * \brief IAX2 command
 */
class iax2_command {
public:
	iax2_command(enum iax2_command_type type, unsigned short call_num);

	/*!
	 * \brief Constructor for a command with a raw payload
	 * \param str The raw payload for the command
	 */
	iax2_command(enum iax2_command_type type, unsigned short call_num, 
		const void *raw, unsigned int raw_len);

	/*!
	 * \brief Constructor for a command with a string payload
	 * \param str The string payload for the command
	 */
	iax2_command(enum iax2_command_type type, unsigned short call_num, 
		const char *str);

	/*!
	 * \brief Constructor for a command with an unsigned integer payload
	 * \param num The unsigned integer payload for the command
	 */
	iax2_command(enum iax2_command_type type, unsigned short call_num, 
		unsigned int num);
	
	/*!
	 * \brief IAX2 command destructor
	 */
	~iax2_command(void);

	/*!
	 * \brief retrieve the call number
	 */
	inline unsigned short get_call_num(void) const 
		{ return call_num; }

	/*!
	 * \brief retrieve the command type
	 */
	inline enum iax2_command_type get_type(void) const 
		{ return type; }

	/*!
	 * \brief retrieve the command payload type
	 */
	inline enum iax2_command_payload_type get_payload_type(void) const 
		{ return payload_type; }

	/*!
	 * \brief retrieve the raw payload
	 */
	inline void *get_payload_raw(void) const 
		{ return payload.raw; }

	/*!
	 * \brief retrieve the string payload
	 */
	inline const char *get_payload_str(void) const 
		{ return payload.str; }

	/*!
	 * \brief retrieve the unsigned int payload
	 */
	inline unsigned int get_payload_uint(void) const 
		{ return payload.uint; }

	/*!
	 * \brief retrieve the raw data length for a raw payload
	 */
	inline unsigned int get_raw_datalen(void) const
		{ return raw_datalen; }

	/*!
	 * \brief Return the type as a string
	 */
	const char *type2str(void) const;

	/*!
	 * \brief Print out the contents of the command to stderr.
	 */
	void print(void) const;

private:
	unsigned short call_num;
	enum iax2_command_type type;
	enum iax2_command_payload_type payload_type;
	union {
		void *raw;
		const char *str;
		unsigned int uint;
	} payload;
	/*!
	 * \brief Payload data length
	 *
	 * \note This is only set for a raw payload
	 */
	unsigned int raw_datalen;
};

/*!
 * \brief Results for sending an iax2_command 
 */
enum iax2_command_result {
	/*! Success */
	IAX2_COMMAND_RESULT_SUCCESS,
	/*! The call for this command was not found */
	IAX2_COMMAND_RESULT_NOCALL,
	/*! Sending this command for this call is not supported,
	 *  either because of the type or the state the call is in. */
	IAX2_COMMAND_RESULT_UNSUPPORTED,
};

#endif /* IAX2_COMMAND_H */
