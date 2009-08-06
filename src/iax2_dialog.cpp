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
 * \brief IAX2 dialog
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#include "iax2/iax2_dialog.h"
#include "iax2/iax2_event.h"
#include "iax2/iax2_peer.h"
#include "iax2/iax2_server.h"
#include "iax2/iax2_frame.h"

using namespace iax2xx;

iax2_dialog::iax2_dialog(iax2_peer *peer, unsigned short num, int sock) :
	call_num(num), dest_call_num(0), out_seq_num(0), in_seq_num(0),
	sockfd(sock), parent_peer(peer), timer_id(0)
{
}

iax2_dialog::~iax2_dialog(void)
{
	// If there is a timer running, it needs to be stopped.  If the parent
	// dialog timer tries to call the timer_callback function of this object
	// after it is gone, it will go BOOM!
	if (timer_id)
		parent_peer->stop_timer(timer_id);
}

enum iax2_dialog_result iax2_dialog::process_incoming_frame(iax2_frame &frame_in,
	const struct sockaddr_in *rcv_addr)
{
	if (frame_in.get_shell() == IAX2_FRAME_FULL) {
		// XXX Special handling for sequence numbers on an ACK perhaps?

		if (frame_in.get_out_seq_num() < in_seq_num) {
			// This frame has already been received.  Silently ignore it.
			printf("Duplicate frame received for call_num '%u'\n", call_num);
			return IAX2_DIALOG_RESULT_SUCCESS;
		} else if (frame_in.get_out_seq_num() > in_seq_num) {
			// This frame arrived out of order.  We're still waiting for a previous frame
			// to arrive.  For now, it is just dropped.  However, this should be queued
			// and processed after the preceeding frames are receivied.
			printf("Frame received out of order for call num '%u'.  Got '%u', expecting '%u'\n", 
				call_num, frame_in.get_out_seq_num(), in_seq_num);
			return IAX2_DIALOG_RESULT_SUCCESS;
		}

		// Increment the counter for the next sequence number we expect to receive
		in_seq_num++;
	}

	return process_frame(frame_in, rcv_addr);
}

///////////////////////////////////////////////////////////////////////////////

iax2_register_dialog::iax2_register_dialog(iax2_peer *peer, unsigned short num, int sock,
	const struct sockaddr_in *sin) :
	iax2_dialog(peer, num, sock), state(IAX2_REGISTER_STATE_NONE), username(NULL)
{
	memcpy(&remote_addr, sin, sizeof(remote_addr));
}

iax2_register_dialog::~iax2_register_dialog(void)
{
	// If there is a timer running, it needs to be stopped.  If the parent
	// dialog timer tries to call the timer_callback function of this object
	// after it is gone, it will go BOOM!
	if (timer_id)
		parent_peer->stop_timer(timer_id);

	if (username)
		free((void *) username);
}

enum iax2_dialog_result iax2_register_dialog::process_frame(iax2_frame &frame_in,
	const struct sockaddr_in *rcv_addr)
{
	if (state != IAX2_REGISTER_STATE_REGREQ_SENT || frame_in.get_shell() != IAX2_FRAME_FULL
	    || frame_in.get_type() != IAX2_FRAME_TYPE_IAX2 
	    || frame_in.get_subclass() != IAX2_SUBCLASS_REGACK)
		return IAX2_DIALOG_RESULT_INVAL;

	// Remove the timer for retransmission of the REGREQ
	parent_peer->stop_timer(timer_id);
	timer_id = 0;

	// Send an ACK, which then completes this dialog.
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_ACK). \
		set_source_call_num(call_num). \
		set_dest_call_num(frame_in.get_source_call_num()). \
		set_in_seq_num(in_seq_num). \
		set_out_seq_num(out_seq_num++). \
		set_timestamp(frame_in.get_timestamp()). \
		send(&remote_addr, sockfd);

	state = IAX2_REGISTER_STATE_NONE;
	
	// Start the timer to be half the refresh time, to make sure that it is successful
	// by the time it expires, in case there has to be retransmissions.
	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(IAX2_DEFAULT_REFRESH / 2, 0)));

	return IAX2_DIALOG_RESULT_SUCCESS;
}

enum iax2_command_result iax2_register_dialog::process_command(iax2_command &command)
{
	return IAX2_COMMAND_RESULT_UNSUPPORTED;
}

enum iax2_dialog_result iax2_register_dialog::timer_callback(void)
{
	if (state == IAX2_REGISTER_STATE_NONE) {
		start(NULL);
		return IAX2_DIALOG_RESULT_SUCCESS;
	} else if (state != IAX2_REGISTER_STATE_REGREQ_SENT) {
		printf("timer_callback in iax2_registrar_dialog called in weird state '%d'!!!\n", state);
		return IAX2_DIALOG_RESULT_SUCCESS;
	}

	// Retransmit the reigstration request.
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_REGREQ). \
		set_source_call_num(call_num).add_ie_string(IAX2_IE_USERNAME, username). \
		set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num - 1). \
		set_retransmission(true).send(&remote_addr, sockfd);

	parent_peer->queue_event(new iax2_event(
		IAX2_EVENT_TYPE_REGISTRATION_RETRANSMITTED, call_num));

	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(1, 0)));
	
	return IAX2_DIALOG_RESULT_SUCCESS;
}

int iax2_register_dialog::start(const char *un)
{
	state = IAX2_REGISTER_STATE_REGREQ_SENT;

	// The sequence numbers get reset here, because a single instance of this
	// dialog object persists in memory to handle refreshing the registration.
	// However, each time the registration restarts for a refresh, the sequence
	// numbers start at the beginning.
	in_seq_num = 0;
	out_seq_num = 0;

	if (!username && !(username = strdup(un)))
		return -1;

	// Send the initial reigstration request.
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_REGREQ). \
		set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num++). \
		set_source_call_num(call_num).add_ie_string(IAX2_IE_USERNAME, username);

	// just in case the packet must be retransmitted
	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(1, 0)));
	
	if (frame.send(&remote_addr, sockfd))
		return -1;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

iax2_registrar_dialog::iax2_registrar_dialog(iax2_server *server, 
	unsigned short num, int sock) :
	iax2_dialog((iax2_peer *) server, num, sock), 
	state(IAX2_REGISTRAR_STATE_NONE), username(NULL)
{
}

iax2_registrar_dialog::~iax2_registrar_dialog(void)
{
	// If there is a timer running, it needs to be stopped.  If the parent
	// dialog timer tries to call the timer_callback function of this object
	// after it is gone, it will go BOOM!
	if (timer_id)
		parent_peer->stop_timer(timer_id);

	if (username)
		free((void *) username);
}

enum iax2_dialog_result iax2_registrar_dialog::process_frame(iax2_frame &frame_in, 
	const struct sockaddr_in *rcv_addr)
{
	enum iax2_dialog_result res = IAX2_DIALOG_RESULT_INVAL;

	if (state == IAX2_REGISTRAR_STATE_NONE) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
	    	|| frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
			|| frame_in.get_subclass() != IAX2_SUBCLASS_REGREQ)
			return res;

		if (!(username = frame_in.get_ie_string(IAX2_IE_USERNAME)))
			return res;
		username = strdup(username);

		dest_call_num = frame_in.get_source_call_num();

		memcpy(&remote_addr, rcv_addr, sizeof(remote_addr));

		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_REGACK). \
			set_source_call_num(call_num). \
			set_dest_call_num(dest_call_num). \
			set_in_seq_num(in_seq_num). \
			set_out_seq_num(out_seq_num++). \
			set_timestamp(frame_in.get_timestamp()). \
			add_ie_unsigned_short(IAX2_IE_REFRESH, (unsigned short) IAX2_DEFAULT_REFRESH). \
			send(rcv_addr, sockfd);

		state = IAX2_REGISTRAR_STATE_REGREQ_RCVD;
		res = IAX2_DIALOG_RESULT_SUCCESS;
	} else if (state == IAX2_REGISTRAR_STATE_REGREQ_RCVD) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
			|| frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
			|| frame_in.get_subclass() != IAX2_SUBCLASS_ACK)
			return res;

		iax2_server *server = (iax2_server *) parent_peer;
		server->register_peer(username, rcv_addr);

		res = IAX2_DIALOG_RESULT_DESTROY;
	}

	return res;
}

enum iax2_command_result iax2_registrar_dialog::process_command(iax2_command &command)
{
	return IAX2_COMMAND_RESULT_UNSUPPORTED;
}

enum iax2_dialog_result iax2_registrar_dialog::timer_callback(void)
{
	if (state != IAX2_REGISTRAR_STATE_REGREQ_RCVD) {
		printf("timer_callback in iax2_registrar_dialog called in weird state '%d'!!!\n", state);
		return IAX2_DIALOG_RESULT_SUCCESS;
	}

	// Retransmit the reigstration request.
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_REGACK). \
		set_source_call_num(call_num).set_dest_call_num(dest_call_num). \
		add_ie_unsigned_short(IAX2_IE_REFRESH, (unsigned short) IAX2_DEFAULT_REFRESH). \
		set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num - 1). \
		set_retransmission(true).send(&remote_addr, sockfd);

	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(1, 0)));
	
	return IAX2_DIALOG_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

iax2_call_dialog::iax2_call_dialog(iax2_peer *peer, unsigned short num, int sock,
	const struct sockaddr_in *sin) :
	iax2_dialog(peer, num, sock), state(IAX2_CALL_STATE_DOWN),
	peer_capabilities(0), actual_formats(0)
{
	memcpy(&remote_addr, sin, sizeof(remote_addr));
}

iax2_call_dialog::~iax2_call_dialog(void)
{
	// If there is a timer running, it needs to be stopped.  If the parent
	// dialog timer tries to call the timer_callback function of this object
	// after it is gone, it will go BOOM!
	if (timer_id)
		parent_peer->stop_timer(timer_id);
}

enum iax2_dialog_result iax2_call_dialog::process_frame(iax2_frame &frame_in, 
	const struct sockaddr_in *rcv_addr)
{
	enum iax2_dialog_result res = IAX2_DIALOG_RESULT_INVAL;

	if (state == IAX2_CALL_STATE_DOWN) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
			|| frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
			|| frame_in.get_subclass() != IAX2_SUBCLASS_NEW)
			return res;

		start_time = tvnow();
		dest_call_num = frame_in.get_source_call_num();
		peer_capabilities = frame_in.get_ie_unsigned_long(IAX2_IE_CAPABILITY);
		u_int32_t our_cap = parent_peer->get_capabilities();
		u_int32_t common_cap = peer_capabilities & our_cap;
		actual_formats = parent_peer->choose_formats(peer_capabilities);
		printf("our capabilities: %u peer_capabilities: %u  common formats: %u  actual formats: %u\n", 
			our_cap, peer_capabilities, common_cap, actual_formats);

		memcpy(&remote_addr, rcv_addr, sizeof(remote_addr));

		iax2_frame frame;
		if (actual_formats) {
			frame.set_subclass(IAX2_SUBCLASS_ACCEPT);
			state = IAX2_CALL_STATE_ACCEPT_SENT;
		} else {
			frame.set_subclass(IAX2_SUBCLASS_REJECT);
			state = IAX2_CALL_STATE_REJECT_SENT;	
		}
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2). \
			set_source_call_num(call_num). \
			set_dest_call_num(dest_call_num). \
			set_in_seq_num(in_seq_num). \
			set_out_seq_num(out_seq_num++). \
			set_timestamp(0). \
			add_ie_unsigned_long(IAX2_IE_FORMAT, actual_formats). \
			send(&remote_addr, sockfd);

		if (timer_id) {
			parent_peer->stop_timer(timer_id);
			timer_id = 0;
		}

		res = IAX2_DIALOG_RESULT_SUCCESS;
	} else if (state == IAX2_CALL_STATE_NEW_SENT) {
		// Check for ACCEPT or REJECT
		if (frame_in.get_shell() != IAX2_FRAME_FULL
		    || frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
		    || ( frame_in.get_subclass() != IAX2_SUBCLASS_ACCEPT 
				&& frame_in.get_subclass() != IAX2_SUBCLASS_REJECT) ) {
			return res;
		}

		dest_call_num = frame_in.get_source_call_num();

		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_ACK). \
			set_source_call_num(call_num). \
			set_dest_call_num(dest_call_num). \
			set_in_seq_num(in_seq_num). \
			set_out_seq_num(out_seq_num++). \
			set_timestamp(tvdiff_ms(tvnow(), start_time)). \
			send(&remote_addr, sockfd);

		if (timer_id) {
			parent_peer->stop_timer(timer_id);
			timer_id = 0;
		}

		if (frame_in.get_subclass() == IAX2_SUBCLASS_ACCEPT) {
			res = IAX2_DIALOG_RESULT_SUCCESS;
			state = IAX2_CALL_STATE_UP;
		} else { // REJECT
			res = IAX2_DIALOG_RESULT_DESTROY;
			state = IAX2_CALL_STATE_DOWN;
		}
	} else if (state == IAX2_CALL_STATE_ACCEPT_SENT) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
		    || frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
		    || frame_in.get_subclass() != IAX2_SUBCLASS_ACK)
			return res;

		if (timer_id) {
			parent_peer->stop_timer(timer_id);
			timer_id = 0;
		}

		parent_peer->queue_event(new iax2_event(IAX2_EVENT_TYPE_CALL_ESTABLISHED, 
			call_num, inet_ntoa(remote_addr.sin_addr)));
	
		res = IAX2_DIALOG_RESULT_SUCCESS;
		state = IAX2_CALL_STATE_UP;
	} else if (state == IAX2_CALL_STATE_REJECT_SENT) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
		    || frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
		    || frame_in.get_subclass() != IAX2_SUBCLASS_ACK)
			return res;

		if (timer_id) {
			parent_peer->stop_timer(timer_id);
			timer_id = 0;
		}

		res = IAX2_DIALOG_RESULT_DESTROY;
		state = IAX2_CALL_STATE_DOWN;
	} else if (state == IAX2_CALL_STATE_HANGUP_SENT) {
		if (frame_in.get_shell() != IAX2_FRAME_FULL
			|| frame_in.get_type() != IAX2_FRAME_TYPE_IAX2
			|| frame_in.get_subclass() != IAX2_SUBCLASS_ACK)
			return res;

		res = IAX2_DIALOG_RESULT_DESTROY;
	} else if (state == IAX2_CALL_STATE_UP) {
		if (frame_in.get_shell() == IAX2_FRAME_FULL
		    && frame_in.get_type() == IAX2_FRAME_TYPE_TEXT) {
			unsigned int len = frame_in.get_raw_data_len() + 1;
			char *str = (char *) alloca(len);
			memcpy(str, frame_in.get_raw_data(), len - 1);
			str[len - 1] = '\0';
			parent_peer->queue_event(new iax2_event(IAX2_EVENT_TYPE_TEXT, 
				call_num, str));

			retransmit_frame_queue();

			iax2_frame frame;
			frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
				set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_ACK). \
				set_source_call_num(call_num). \
				set_dest_call_num(dest_call_num). \
				set_in_seq_num(in_seq_num). \
				set_out_seq_num(out_seq_num++). \
				set_timestamp(tvdiff_ms(tvnow(), start_time)). \
				send(&remote_addr, sockfd);

			res = IAX2_DIALOG_RESULT_SUCCESS;
		} else if (frame_in.get_shell() == IAX2_FRAME_FULL
				&& frame_in.get_type() == IAX2_FRAME_TYPE_IAX2
				&& frame_in.get_subclass() == IAX2_SUBCLASS_HANGUP) {
			iax2_frame frame;
			frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
				set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_ACK). \
				set_source_call_num(call_num). \
				set_dest_call_num(dest_call_num). \
				set_in_seq_num(in_seq_num). \
				set_out_seq_num(out_seq_num++). \
				set_timestamp(tvdiff_ms(tvnow(), start_time)). \
				send(&remote_addr, sockfd);

			parent_peer->queue_event(new iax2_event(IAX2_EVENT_TYPE_CALL_HANGUP,
				call_num, inet_ntoa(remote_addr.sin_addr)));

			res = IAX2_DIALOG_RESULT_DESTROY;
		} else if (frame_in.get_shell() == IAX2_FRAME_FULL
				&& frame_in.get_type() == IAX2_FRAME_TYPE_IAX2
				&& frame_in.get_subclass() == IAX2_SUBCLASS_ACK) {
			// Delete all queued full frames that have been ACKed
			while (!frame_queue.empty()) {
				iax2_frame *frame = frame_queue.front();
				if (frame->get_out_seq_num() >= frame_in.get_in_seq_num())
					break;
				frame_queue.pop_front();
				delete frame;
			}
			retransmit_frame_queue();
		} else if (frame_in.get_shell() == IAX2_FRAME_META
				&& frame_in.get_meta_type() == IAX2_META_VIDEO) {
			parent_peer->queue_event(new iax2_event(IAX2_EVENT_TYPE_VIDEO, call_num,
				new iax2_video_event_payload(frame_in.get_raw_data(), 
					frame_in.get_raw_data_len(), frame_in.get_timestamp())));
			res = IAX2_DIALOG_RESULT_SUCCESS;
		}
	}

	return res;
}

enum iax2_command_result iax2_call_dialog::process_command(iax2_command &command)
{
	enum iax2_command_result res = IAX2_COMMAND_RESULT_UNSUPPORTED;

	if (command.get_type() == IAX2_COMMAND_TYPE_HANGUP) {
		retransmit_frame_queue();

		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_HANGUP). \
			set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num++). \
			set_source_call_num(call_num). \
			set_dest_call_num(dest_call_num). \
			set_timestamp(tvdiff_ms(tvnow(), start_time)). \
			send(&remote_addr, sockfd);

		state = IAX2_CALL_STATE_HANGUP_SENT;
		res = IAX2_COMMAND_RESULT_SUCCESS;
	} else if (state == IAX2_CALL_STATE_UP 
		&& command.get_type() == IAX2_COMMAND_TYPE_TEXT) {
		retransmit_frame_queue();

		iax2_frame *frame = new iax2_frame();;
		frame->set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_TEXT). \
			set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num++). \
			set_source_call_num(call_num). \
			set_dest_call_num(dest_call_num). \
			set_timestamp(tvdiff_ms(tvnow(), start_time)). \
			set_raw_data(command.get_payload_str(), strlen(command.get_payload_str())). \
			send(&remote_addr, sockfd);
		
		frame_queue.push_back(frame);

		res = IAX2_COMMAND_RESULT_SUCCESS;
	} else if (state == IAX2_CALL_STATE_UP
		&& command.get_type() == IAX2_COMMAND_TYPE_VIDEO) {

		// XXX Check for timestamp wraparound and send a FULL frame to resync

		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_META). \
			set_meta_type(IAX2_META_VIDEO).set_source_call_num(call_num). \
			set_timestamp(tvdiff_ms(tvnow(), start_time)). \
			set_raw_data(command.get_payload_raw(), command.get_raw_datalen()). \
			send(&remote_addr, sockfd);

		res = IAX2_COMMAND_RESULT_SUCCESS;
	}

	return res;
}

void iax2_call_dialog::retransmit_frame_queue(void)
{
	for (frame_queue_iterator i = frame_queue.begin(); 
		i != frame_queue.end(); i++) {
		(*i)->set_retransmission(true).send(&remote_addr, sockfd);
	}
}

enum iax2_dialog_result iax2_call_dialog::timer_callback(void)
{
	if (state == IAX2_CALL_STATE_NEW_SENT) {
		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_NEW). \
			set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num - 1). \
			set_source_call_num(call_num).add_ie_unsigned_short(IAX2_IE_VERSION, 2). \
			add_ie_unsigned_long(IAX2_IE_CAPABILITY, parent_peer->get_capabilities()). \
			add_ie_unsigned_long(IAX2_IE_FORMAT, parent_peer->get_preferred_format()). \
			set_retransmission(true).send(&remote_addr, sockfd);
	} else if (state == IAX2_CALL_STATE_HANGUP_SENT) {
		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_HANGUP). \
			set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num - 1). \
			set_source_call_num(call_num). \
			set_retransmission(true).send(&remote_addr, sockfd);
	} else if (state == IAX2_CALL_STATE_UP) {
		retransmit_frame_queue();
	} else {
		printf("timer_callback for call dialog in weird state '%d'\n", state);
		// return early so that the timer is not restarted
		return IAX2_DIALOG_RESULT_SUCCESS;
	}

	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(1, 0)));
	
	return IAX2_DIALOG_RESULT_SUCCESS;
}

int iax2_call_dialog::start(void)
{
	state = IAX2_CALL_STATE_NEW_SENT;
	
	// just in case the packet must be retransmitted
	timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(1, 0)));

	start_time = tvnow();
	
	// Send the initial NEW request
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num++). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_NEW). \
		set_source_call_num(call_num).add_ie_unsigned_short(IAX2_IE_VERSION, 2). \
		add_ie_unsigned_long(IAX2_IE_CAPABILITY, parent_peer->get_capabilities()). \
		add_ie_unsigned_long(IAX2_IE_FORMAT, parent_peer->get_preferred_format());
	
	if (frame.send(&remote_addr, sockfd))
		return -1;

	return 0;
}
