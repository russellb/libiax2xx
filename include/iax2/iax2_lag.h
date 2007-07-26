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
 * \brief IAX2 lag definitions
 */


#ifndef IAX2_LAG_H
#define IAX2_LAG_H

#include "iax2/iax2_dialog.h"
#include "iax2/iax2_frame.h"

class iax2_peer;
class iax2_server;

/*!
 * \brief Possible states for iax2_lag_dialog
 */
enum iax2_lag_state {
	/*! Base state */
	IAX2_LAG_STATE_NONE,
	/*! LAGRP sent, wait for ACK */
	IAX2_LAG_STATE_LAGRP_SENT,
	/*! Once LAGRQ is received, a LAGRP is sent */
	IAX2_LAG_STATE_LAGRQ_RCVD,
	/*! Once LAGRQ is sent, wait for LAGRP */
	IAX2_LAG_STATE_LAGRQ_SENT,
	/*! Once LAGRP is received, an ACK is sent */
	IAX2_LAG_STATE_LAGRP_RCVD,
};

/*!
 * \brief LAG dialog
 *
 * This is the dialog when this peer is trying to find RTT (Round Trip Time)
 * to another peer.
 */
class iax2_lag_dialog : public iax2_dialog {
public:
	iax2_lag_dialog(iax2_peer *peer, unsigned short call_num, int sockfd,
		const struct sockaddr_in *sin);
	virtual ~iax2_lag_dialog(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame,
		const struct sockaddr_in *rcv_addr);

	virtual enum iax2_command_result process_command(iax2_command &command);

	virtual enum iax2_dialog_result timer_callback(void);

	int start(void);

private:
	enum iax2_lag_state state;
	struct sockaddr_in remote_addr;
	unsigned int retransmissions;
	struct timeval start_time;
};

#endif /* IAX2_LAG_H */
