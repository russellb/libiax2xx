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
 * \brief IAX2 dialog definitions
 */

#ifndef IAX2_DIALOG_H
#define IAX2_DIALOG_H

#include <netinet/in.h>

#include <list>

using namespace std;

#include "iax2/iax2_frame.h"
#include "iax2/iax2_command.h"

class iax2_peer;
class iax2_server;

/*!
 * \brief Return values for process_frame()
 */
enum iax2_dialog_result {
	/*! The frame was successfully processed */
	IAX2_DIALOG_RESULT_SUCCESS,
	/*! The frame was invalid for this dialog, send back an INVAL */
	IAX2_DIALOG_RESULT_INVAL,
	/*! The frame ended this dialog, delete it and remove it from the dialogs list */
	IAX2_DIALOG_RESULT_DESTROY,
	/*! Delete the dialog, but it is not in the dialogs list */
	IAX2_DIALOG_RESULT_DELETE,
};

/*!
 * \brief A base class for an iax2 dialog
 *
 * This should be used as a base class to implement handling of a specific
 * type of IAX2 dialog.
 */
class iax2_dialog {
public:
	iax2_dialog(iax2_peer *peer, unsigned short num, int sockfd);

	/*!
	 * \brief virtual iax2_dialog destructor
	 */	
	virtual ~iax2_dialog(void);

	/*!
	 * \brief Retrieve the call number for this dialog
	 *
	 * \return the call number for this dialog
	 */
	inline unsigned short get_call_num(void) const
		{ return call_num; }

	inline const struct sockaddr_in *get_remote_addr(void) const
		{ return &remote_addr; }

	inline unsigned short get_remote_call_num(void) const
		{ return dest_call_num; }

	enum iax2_dialog_result process_incoming_frame(iax2_frame &frame,
		const struct sockaddr_in *rcv_addr);

	virtual enum iax2_command_result process_command(iax2_command &command) = 0;

	virtual enum iax2_dialog_result timer_callback(void) = 0;

protected:
	/*!
	 * \brief process an incoming frame for this call number
	 *
	 * This is a pure virtual function that gets implemented by the actual
	 * implementation of a specific type of iax2_dialog.  
	 */
	virtual enum iax2_dialog_result process_frame(iax2_frame &frame, 
		const struct sockaddr_in *rcv_addr) = 0;

	struct sockaddr_in remote_addr;
	/*! This number uniquely identifies the session locally */
	unsigned short call_num;
	/*! This number uniquely identifies the session on the remote side */
	unsigned short dest_call_num;
	unsigned char out_seq_num;
	unsigned char in_seq_num;
	/*! The socket to use for sending frames */
	int sockfd;
	/*! The parent event generator */
	iax2_peer *parent_peer;
	/*! ID of registered timer */
	unsigned int timer_id;
};

/*!
 * \brief Possible states for iax2_register_dialog
 */
enum iax2_register_state {
	/*! Base state */
	IAX2_REGISTER_STATE_NONE,
	/*! REGREQ sent, waiting for REGACK */
	IAX2_REGISTER_STATE_REGREQ_SENT,
	/* Once REGACK is received, an ACK is sent */
};

/*!
 * \brief Registration dialog
 *
 * This is the dialog when this peer is registering to another peer.  The
 * dialog for processing an incoming registration is iax2_registrar_dialog.
 */
class iax2_register_dialog : public iax2_dialog {
public:
	iax2_register_dialog(iax2_peer *peer, unsigned short call_num, int sockfd,
		const struct sockaddr_in *sin);
	virtual ~iax2_register_dialog(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame, 
		const struct sockaddr_in *rcv_addr);

	virtual enum iax2_command_result process_command(iax2_command &command);

	virtual enum iax2_dialog_result timer_callback(void);

	int start(const char *username);

private:
	int send_register(void *d);

	enum iax2_register_state state;
	unsigned int retransmissions;

	const char *username;
};

/*!
 * \brief Possible states for iax2_registrar_dialog
 */
enum iax2_registrar_state {
	/*! Base state */	
	IAX2_REGISTRAR_STATE_NONE,
	/*! REGREQ received, REGACK or REGREJ sent */
	IAX2_REGISTRAR_STATE_REGREQ_RCVD,
	/*! Once ACK is received, the dialog is over */
};

#define IAX2_DEFAULT_REFRESH 10

/*!
 * \brief Registrar dialog
 *
 * This is the dialog when another peer is registering to this peer.  The
 * dialog for sending a registration is iax2_register_dialog.
 */
class iax2_registrar_dialog : public iax2_dialog {
public:
	iax2_registrar_dialog(iax2_server *server, unsigned short call_num, int sockfd);
	virtual ~iax2_registrar_dialog(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame, 
		const struct sockaddr_in *rcv_addr);

	virtual enum iax2_command_result process_command(iax2_command &command);
	
	virtual enum iax2_dialog_result timer_callback(void);

private:
	enum iax2_registrar_state state;

	/*! The username of the peer that is requesting registration */
	const char *username;
};

// XXX This is only the receiving side right now
enum iax2_call_state {
	/*! Base state */
	IAX2_CALL_STATE_DOWN,
	/*! NEW sent */
	IAX2_CALL_STATE_NEW_SENT,
	/*! ACCEPT sent */
	IAX2_CALL_STATE_ACCEPT_SENT,
	/*! REJECT sent */
	IAX2_CALL_STATE_REJECT_SENT,
	/*! Call is up */
	IAX2_CALL_STATE_UP,
	/*! Hangup has been sent */
	IAX2_CALL_STATE_HANGUP_SENT,
};

class iax2_call_dialog : public iax2_dialog {
public:
	iax2_call_dialog(iax2_peer *peer, unsigned short call_num, int sockfd,
		const struct sockaddr_in *sin);
	virtual ~iax2_call_dialog(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame, 
		const struct sockaddr_in *rcv_addr);

	virtual enum iax2_command_result process_command(iax2_command &command);
	
	virtual enum iax2_dialog_result timer_callback(void);

	int start(void);

	void retransmit_frame_queue(void);

private:
	enum iax2_call_state state;
	unsigned int retransmissions;
	struct timeval start_time;
	u_int32_t peer_capabilities;
	u_int32_t actual_formats;

	list<iax2_frame *> frame_queue;
	typedef list<iax2_frame *>::const_iterator frame_queue_iterator;
};

#endif /* IAX2_DIALOG_H */
