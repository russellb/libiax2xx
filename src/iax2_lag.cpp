/*
 * Copyright (C) 2006, Rahul Amin <ramin@clemson.edu>, Thomas Brown <brown5@clemson.edu>
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
 * \author Rahul Amin, Thomas Brown
 *
 * \brief IAX2 lag
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#include "iax2/iax2_lag.h"
#include "iax2/iax2_dialog.h"
#include "iax2/iax2_event.h"
#include "iax2/iax2_peer.h"
#include "iax2/iax2_server.h"

using namespace iax2xx;

iax2_lag_dialog::iax2_lag_dialog(iax2_peer *peer, unsigned short num, int sock,
	const struct sockaddr_in *sin) :
	iax2_dialog(peer, num, sock), state(IAX2_LAG_STATE_NONE)
{
	memcpy(&remote_addr, sin, sizeof(remote_addr));
}

iax2_lag_dialog::~iax2_lag_dialog(void)
{
	// If there is a timer running, it needs to be stopped.
	if (timer_id)
		parent_peer->stop_timer(timer_id);
}

enum iax2_dialog_result iax2_lag_dialog::process_frame(iax2_frame &frame_in,
	const struct sockaddr_in *rcv_addr)
{
	if (state == IAX2_LAG_STATE_NONE) {         
		if (frame_in.get_shell() == IAX2_FRAME_FULL             
			&& frame_in.get_type() == IAX2_FRAME_TYPE_IAX2
			&& frame_in.get_subclass() == IAX2_SUBCLASS_LAGRQ) { // Is a LAG Request received
		    
			iax2_frame frame;
			frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
				set_type(IAX2_FRAME_TYPE_IAX2). \
				set_subclass(IAX2_SUBCLASS_LAGRP). \
				set_source_call_num(call_num). \
				set_dest_call_num(frame_in.get_source_call_num()). \
				set_in_seq_num(in_seq_num). \
				set_out_seq_num(out_seq_num++). \
				set_timestamp(frame_in.get_timestamp()). \
				send(&remote_addr, sockfd);

			state = IAX2_LAG_STATE_LAGRP_SENT;

			// Start the timer to be the refresh time, to make sure that it is successful
			// by the time it expires, in case there has to be retransmissions.
			timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(IAX2_DEFAULT_REFRESH, 0)));
		       
			return IAX2_DIALOG_RESULT_SUCCESS;
		}
		else {                                                // ignore any unexpected subclass (i.e. LAGRP)
			return IAX2_DIALOG_RESULT_INVAL;              // Return INVAL result
		}
	}
	else if (state == IAX2_LAG_STATE_LAGRP_SENT) {
			
		if (frame_in.get_shell() == IAX2_FRAME_FULL
			&& frame_in.get_type() == IAX2_FRAME_TYPE_IAX2
			&& frame_in.get_subclass() == IAX2_SUBCLASS_ACK) {  // Is an ACK received
		  
			state = IAX2_LAG_STATE_NONE;
			return IAX2_DIALOG_RESULT_DESTROY;
		}
		else
		{
			state = IAX2_LAG_STATE_NONE;
			return IAX2_DIALOG_RESULT_INVAL;
		}

	}

	else if (state == IAX2_LAG_STATE_LAGRQ_SENT) {
                 
		if (frame_in.get_shell() == IAX2_FRAME_FULL
			&& frame_in.get_type() == IAX2_FRAME_TYPE_IAX2
			&& frame_in.get_subclass() == IAX2_SUBCLASS_LAGRP) {// Is a LAG reply received

			iax2_frame frame;
			frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
				set_type(IAX2_FRAME_TYPE_IAX2). \
				set_subclass(IAX2_SUBCLASS_ACK). \
				set_source_call_num(call_num). \
				set_dest_call_num(frame_in.get_source_call_num()). \
				set_in_seq_num(in_seq_num). \
				set_out_seq_num(out_seq_num++). \
				set_timestamp(frame_in.get_timestamp()).send(rcv_addr, sockfd);

			state = IAX2_LAG_STATE_NONE;
                        
			// Stop the timer
			if (timer_id) {
				parent_peer->stop_timer(timer_id);
				timer_id = 0;
			}

			parent_peer->queue_event(new iax2_event(IAX2_EVENT_TYPE_LAG, call_num,
					tvdiff_ms(tvnow(), parent_peer->get_reference_time()) - 
					frame_in.get_timestamp()));
			return IAX2_DIALOG_RESULT_DESTROY;
		}
		else {
			return IAX2_DIALOG_RESULT_INVAL;
		}
	}
}

int iax2_lag_dialog::start(void)
{
	state = IAX2_LAG_STATE_LAGRQ_SENT;
	start_time = tvnow();

	//Send the initial LAG request
	iax2_frame frame;
	frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_LAGRQ). \
		set_source_call_num(call_num). \
		set_in_seq_num(in_seq_num).set_out_seq_num(out_seq_num++). \
		set_timestamp(tvdiff_ms(start_time, parent_peer->get_reference_time()));

	// Packet needs to be retransmitted
	timer_id = parent_peer->start_timer(this, tvadd(tvnow(),create_tv(5,0)));

	if (frame.send(&remote_addr, sockfd))
		return -1;
	return 0;
}

enum iax2_command_result iax2_lag_dialog::process_command(iax2_command &command)
{
	return IAX2_COMMAND_RESULT_UNSUPPORTED; // still needs to be implemented
}

enum iax2_dialog_result iax2_lag_dialog::timer_callback(void)
{
	if (state == IAX2_LAG_STATE_LAGRP_SENT)
	{
		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2). \
			set_subclass(IAX2_SUBCLASS_LAGRP). \
			set_source_call_num(call_num). \
			set_retransmission(true). \
			set_timestamp(tvdiff_ms(start_time, parent_peer->get_reference_time())). \
			send(&remote_addr, sockfd);

		// Start the timer to be the refresh time, to make sure that it is successful
		// by the time it expires, in case there has to be retransmissions.
		timer_id = parent_peer->start_timer(this, tvadd(tvnow(), create_tv(IAX2_DEFAULT_REFRESH, 0)));
	
		return IAX2_DIALOG_RESULT_SUCCESS;
	}
	else if (state == IAX2_LAG_STATE_LAGRQ_SENT)
	{
		iax2_frame frame;
		frame.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
			set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_LAGRQ). \
			set_source_call_num(call_num). \
			set_retransmission(true). \
			set_timestamp(tvdiff_ms(start_time, parent_peer->get_reference_time())). \
			send(&remote_addr, sockfd);

		// Packet needs to be retransmitted
		timer_id = parent_peer->start_timer(this, tvadd(tvnow(),create_tv(5,0)));

		return IAX2_DIALOG_RESULT_SUCCESS;
	}

	return IAX2_DIALOG_RESULT_INVAL; // some case that should not have been present while callback
}
