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
 * \brief IAX2 server
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>

using namespace std;

#include "iax2/iax2_server.h"
#include "iax2/iax2_frame.h"
#include "iax2/iax2_dialog.h"
#include "iax2/iax2_lag.h"

using namespace iax2xx;

iax2_server::iax2_server(void) :
	iax2_peer()
{
}

iax2_server::iax2_server(unsigned short local_port) : 
	iax2_peer(local_port)
{
	registrations.clear();
}

iax2_server::~iax2_server(void)
{
	while (!registrations.empty()) {
		iax2_registration *reg = registrations.front();
		registrations.pop_front();
		delete reg;
	}
}

void iax2_server::process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin)
{
	iax2_dialog *dialog = NULL;
	if (frame.get_shell() == IAX2_FRAME_FULL &&
	    frame.get_type() == IAX2_FRAME_TYPE_IAX2 &&
	    frame.get_subclass() == IAX2_SUBCLASS_REGREQ) {
		if (!(dialog = new iax2_registrar_dialog(this, get_next_call_num(), sockfd)))
			return;
		dialogs[dialog->get_call_num()] = dialog;
	}
	else if (frame.get_shell() == IAX2_FRAME_FULL &&
	         frame.get_type() == IAX2_FRAME_TYPE_IAX2 &&
	         frame.get_subclass() == IAX2_SUBCLASS_LAGRQ) {
	        if (!(dialog = new iax2_lag_dialog(this, get_next_call_num(), sockfd, sin)))
	                return;
	        dialogs[dialog->get_call_num()] = dialog;
	}
	else {
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

void iax2_server::handle_newcall_command(iax2_command &command)
{
	const char *uri = command.get_payload_str();

	// XXX This function should probably provide some feedback about success/failure ...

	if (strncasecmp("iax2:", uri, 5))
		return;
	uri += 5;
	
	// XXX This only supports the uri begin iax2:blah, where blah is a registered peer name

	iax2_registration *reg = NULL;
	for (iax2_reg_iterator i = registrations.begin();
	     i != registrations.end(); i++) {
		if (strcasecmp(uri, (*i)->get_username()))
			continue;
		reg = *i;
		break;	
	}
	if (!reg)
		return;
	
	iax2_call_dialog *call;
	if (!(call = new iax2_call_dialog(this, command.get_call_num(), sockfd, reg->get_addr())))
		return;
	dialogs[call->get_call_num()] = call;

	call->start();
}


void iax2_server::handle_lagrq_command(iax2_command &command)
{
	const char *uri = command.get_payload_str();

	// XXX This function should probably provide some feedback about success/failure ...

	if (strncasecmp("iax2:", uri, 5))
		return;
	uri += 5;

	// XXX This only supports the uri begin iax2:blah, where blah is a registered peer name

	iax2_registration *reg = NULL;
	for (iax2_reg_iterator i = registrations.begin();
	     i != registrations.end(); i++) {
		if (strcasecmp(uri, (*i)->get_username()))
			continue;
		reg = *i;
		break;	
	}
	if (!reg)
		return;
	
	iax2_lag_dialog *lag;
	if (!(lag = new iax2_lag_dialog(this, command.get_call_num(), sockfd, reg->get_addr())))
		return;
	dialogs[lag->get_call_num()] = lag;

	lag->start();
}

void iax2_server::register_peer(const char *username, const struct sockaddr_in *sin)
{
	for (iax2_reg_iterator i = registrations.begin();
	     i != registrations.end(); i++) {
		if (strcasecmp(username, (*i)->get_username()))
			continue;
		
		(*i)->refresh();
		return;
	}

	registrations.push_back(new iax2_registration(this, 0, sockfd, username, sin));
}

void iax2_server::expire_peer(iax2_registration *reg)
{
	registrations.remove(reg);
}

///////////////////////////////////////////////////////////////////////////////

iax2_registration::iax2_registration(iax2_server *server, unsigned short num, int sockfd, 
	const char *un, const struct sockaddr_in *s) :
	iax2_dialog(server, num, sockfd)
{
	memcpy(&sin, s, sizeof(sin));

	username = strdup(un);

	server->queue_event(new iax2_event(IAX2_EVENT_TYPE_REGISTRATION_NEW, 
		call_num, un));
	timer_id = server->start_timer(this, tvadd(tvnow(), create_tv(IAX2_DEFAULT_REFRESH, 0)));
}

iax2_registration::~iax2_registration(void)
{
	iax2_server *server = (iax2_server *) parent_peer;

	server->expire_peer(this);

	server->queue_event(new iax2_event(IAX2_EVENT_TYPE_REGISTRATION_EXPIRED, 
		call_num, username));

	if (username)
		free((void *) username);
}

enum iax2_dialog_result iax2_registration::timer_callback(void)
{
	return IAX2_DIALOG_RESULT_DELETE;
}

void iax2_registration::refresh(void)
{
	iax2_server *server = (iax2_server *) parent_peer;

	printf("refreshing registration for peer '%s'\n", username);

	server->stop_timer(timer_id);
	timer_id = server->start_timer(this, tvadd(tvnow(), create_tv(IAX2_DEFAULT_REFRESH, 0)));
}
