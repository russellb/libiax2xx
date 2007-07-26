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
 * \brief IAX2 client
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

using namespace std;

#include "iax2/iax2_client.h"
#include "iax2/iax2_frame.h"
#include "iax2/iax2_lag.h"

iax2_client::iax2_client(void) : 
	iax2_peer()
{
}

iax2_client::iax2_client(unsigned short local_port) :
	iax2_peer(local_port)
{
}

iax2_client::~iax2_client(void)
{
	if (sockfd > -1)
		close(sockfd);
}

void iax2_client::process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin)
{
	iax2_dialog *dialog = NULL;

 	if (frame.get_shell() == IAX2_FRAME_FULL &&
	    frame.get_type() == IAX2_FRAME_TYPE_IAX2 &&
	    frame.get_subclass() == IAX2_SUBCLASS_NEW) {
		if (!(dialog = new iax2_call_dialog(this, get_next_call_num(), sockfd, sin)))
			return;
		dialogs[dialog->get_call_num()] = dialog;
	}
        // Create a new dialog for the LAG request just received	
	else if (frame.get_shell() == IAX2_FRAME_FULL &&
		 frame.get_type() == IAX2_FRAME_TYPE_IAX2 &&
		 frame.get_subclass() == IAX2_SUBCLASS_LAGRQ) {
		if (!(dialog = new iax2_lag_dialog(this, get_next_call_num(), sockfd, sin)))
			return;
		dialogs[dialog->get_call_num()] = dialog;
	} else {
		// destined for an existing dialog, we hope.
		if (frame.get_shell() == IAX2_FRAME_FULL)
			dialog = dialogs[frame.get_dest_call_num()];
		else
			dialog = find_dialog_media(frame, sin);

		if (!dialog) {
			printf("Didn't find a dialog for the packet.  :(\n");
			// XXX Send an INVAL packet
			return;
		}
	}

 	switch (dialog->process_incoming_frame(frame, sin)) {
 	case IAX2_DIALOG_RESULT_SUCCESS:
 		break;
 	case IAX2_DIALOG_RESULT_DESTROY:
 		dialogs.erase(dialog->get_call_num());
	case IAX2_DIALOG_RESULT_DELETE:
		delete dialog;
		break;
 	case IAX2_DIALOG_RESULT_INVAL:
 		// XXX Send an INVAL packet
 		break;
 	}
}

void iax2_client::handle_newcall_command(iax2_command &command)
{
	printf("Client newcall command ... shouldn't happen!\n");
}


void iax2_client::handle_lagrq_command(iax2_command &command)
{
	printf("Client lag request command ... shouldn't happen!\n");
}
