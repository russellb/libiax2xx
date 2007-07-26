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
 * \brief dialog timer test app
 */

#include <stdlib.h>
#include <stdio.h>

using namespace std;

#include "iax2/iax2_dialog.h"
#include "iax2/iax2_peer.h"
#include "iax2/iax2_frame.h"
#include "iax2/iax2_command.h"
#include "iax2/time.h"

using namespace iax2xx;

class iax2_test_timer : public iax2_peer {
public:
	iax2_test_timer(void) {};
	~iax2_test_timer(void) {};

	int run_test(void);

protected:
	virtual void process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin) {}
	virtual void handle_newcall_command(iax2_command &command) {}
	virtual void handle_lagrq_command(iax2_command &command) {}

private:
};

class iax2_fake_dialog : public iax2_dialog {
public:
	iax2_fake_dialog(iax2_peer *peer, unsigned short num, int sockfd);
		
	virtual ~iax2_fake_dialog(void);

	virtual enum iax2_dialog_result process_frame(iax2_frame &frame, const struct sockaddr_in *sin);
	virtual enum iax2_command_result process_command(iax2_command &command);

	virtual enum iax2_dialog_result timer_callback(void);	

private:
};

iax2_fake_dialog::iax2_fake_dialog(iax2_peer *peer, unsigned short num, int sockfd) :
	iax2_dialog(peer, num, sockfd)
{
}

iax2_fake_dialog::~iax2_fake_dialog(void)
{
}

enum iax2_dialog_result iax2_fake_dialog::process_frame(iax2_frame &frame, const struct sockaddr_in *sin)
{
	return IAX2_DIALOG_RESULT_SUCCESS;
}

enum iax2_command_result iax2_fake_dialog::process_command(iax2_command &command)
{
	return IAX2_COMMAND_RESULT_UNSUPPORTED;
}

enum iax2_dialog_result iax2_fake_dialog::timer_callback(void)
{
	printf("Hello!\n");

	return IAX2_DIALOG_RESULT_SUCCESS;
}

int iax2_test_timer::run_test(void)
{
	int next, remove;
	iax2_fake_dialog fake(NULL, 0, 0);

	remove = start_timer(&fake, tvadd(tvnow(), create_tv(5, 0)));
	start_timer(&fake, tvadd(tvnow(), create_tv(2, 0))); // 3 seconds from now
	start_timer(&fake, tvadd(tvnow(), create_tv(3, 0))); // 2 seconds from now
	start_timer(&fake, tvadd(tvnow(), create_tv(1, 0))); // 1 second from now
	start_timer(&fake, tvsub(tvnow(), create_tv(1, 0))); // 1 second ago
	
	stop_timer(remove);

	while ((next = next_callback_time()) >= 0) {
		if (next > 0)
			usleep(next * 1000);
		run_callbacks();
	}

	return 0;
}

int main(int argc, char *argv[])
{
	class iax2_test_timer test;

	printf("\nThis application tests the code that this library uses for scheduling\n"
		"functions to get called in the future.  This is useful for things like\n"
		"retransmissions or timeouts.\n\n"
		"You should see the following output:\n"
		"Hello!   <---- immediately\n"
		"Hello!   <---- about 1 seconds from now\n"
		"Hello!   <---- about 2 seconds from now\n"
		"Hello!   <---- about 3 seconds from now\n\n\n");

	int res = test.run_test();

	printf("\n");

	exit(res);
}
