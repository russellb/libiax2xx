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
 * \brief IAX2 client definitions
 */

#ifndef IAX2_CLIENT_H
#define IAX2_CLIENT_H

#include "iax2/iax2_peer.h"

/*!
 * \brief Implementation of an IAX client
 */
class iax2_client : public iax2_peer {
public:
	/*!
	 * \brief Default constructor for an iax2_client
	 */
	iax2_client(void);

	/*!
	 * \brief Constructor for an iax2_client
	 *
	 * \param local_port  Local port to bind to
	 */
	iax2_client(unsigned short local_port);
		
	/*!
	 * \brief Destructor for an iax2_client
	 */
	~iax2_client(void);

protected:
	virtual void process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin);

	virtual void handle_newcall_command(iax2_command &command);

	virtual void handle_lagrq_command(iax2_command &command);
};

#endif /* IAX2_CLIENT_H */
