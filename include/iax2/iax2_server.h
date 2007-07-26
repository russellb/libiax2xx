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
 * \brief IAX2 server definitions
 */

#ifndef IAX2_SERVER_H
#define IAX2_SERVER_H

#include "iax2/iax2_peer.h"
#include "iax2/iax2_dialog.h"
#include "iax2/iax2_command.h"

class iax2_registration : public iax2_dialog {
public:
	iax2_registration(iax2_server *server, unsigned short num, 
		int sockfd, const char *username, const struct sockaddr_in *sin);
	virtual ~iax2_registration(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame,
		const struct sockaddr_in *rcv_addr)
		{ return IAX2_DIALOG_RESULT_SUCCESS; }

	virtual enum iax2_command_result process_command(iax2_command &command)
		{ return IAX2_COMMAND_RESULT_UNSUPPORTED; }

	virtual enum iax2_dialog_result timer_callback(void);

	void refresh(void);

	const char *get_username(void) const { return username; }
	const struct sockaddr_in *get_addr(void) const { return &sin; }

private:
	struct sockaddr_in sin;
	const char *username;
};

/*!
 * \brief Implementation of an IAX2 server
 */
class iax2_server : public iax2_peer {
public:
	/*!
	 * \brief Default constructor for an iax2_server
	 */
	iax2_server(void);

	/*!
	 * \brief Constructor for an iax2_server
	 *
	 * \param remote_ipaddr Remote address to register to
	 * \param remote_port Remote port to send registration to
	 * \param local_port  Local port to bind to
	 */
	iax2_server(unsigned short local_port);
	
	/*!
	 * \brief Destructor for an iax2_server
	 */
	~iax2_server(void);

	void register_peer(const char *username, const struct sockaddr_in *sin);
	
	void expire_peer(iax2_registration *reg);

protected:
	virtual void process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin);

	virtual void handle_newcall_command(iax2_command &command);
        virtual void handle_lagrq_command(iax2_command &command);
private:
	list<iax2_registration *> registrations;
	typedef list<iax2_registration *>::const_iterator iax2_reg_iterator;
};

#endif /* IAX2_SERVER_H */
