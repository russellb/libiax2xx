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
 * \brief Test IAX2 Server
 *
 * \note ECE 453 -- Fall 2006, Clemson University
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#include "iax2/iax2_server.h"
#include "iax2/iax2_lag.h"

static void iax2_event_dispatcher(iax2_event &event)
{
	event.print();

	if (event.get_type() == IAX2_EVENT_TYPE_LAG) {
		printf("Lag Data: %u milliseconds (Total Round Trip Time)\n\n", 
			event.get_payload_uint());
	}
}

struct run_server_args {
	iax2_server *server;
	int res;
	pthread_cond_t cond;
	pthread_mutex_t cond_lock;
};

static void *run_server(void *data)
{
	run_server_args *args = (run_server_args *) data;

	iax2_server server(DEFAULT_IAX2_PORT);
	args->server = &server;
	server.register_event_handler(iax2_event_dispatcher);
	args->res = server.run(&args->cond, &args->cond_lock);

	return NULL;
}

int main(int argc, char *argv[])
{
	run_server_args args = { NULL, };
	pthread_t server_thread;

	pthread_cond_init(&args.cond, NULL);
	pthread_mutex_init(&args.cond_lock, NULL);

	/* First, a thread is created for the IAX2 server to run in.  A thread
	 * condition is used to ensure that we don't continue and try to use the
	 * server for anything until it has signaled back to here that it is up
	 * and running.
	 */
	pthread_mutex_lock(&args.cond_lock);
	pthread_create(&server_thread, NULL, run_server, &args);
	pthread_cond_wait(&args.cond, &args.cond_lock);
	pthread_mutex_unlock(&args.cond_lock);

	/* In the next few seconds, the test_client application should be started.
	 * Then, this application starts a new call with the registered client */
	usleep(3000000);
	unsigned short call_num = args.server->new_call("iax2:test_client");

	/* Test sending an IAX2 TEXT frame */	
	usleep(3000000);
	args.server->send_command(new iax2_command(IAX2_COMMAND_TYPE_TEXT, 
		call_num, "Testing text frame"));

	/* Test initiating a lag request to measure round trip processing time */
	usleep(3000000);
	unsigned short lag_num = args.server->new_lag("iax2:test_client");

	/* Finally, wait for the server thread to exit */
	pthread_join(server_thread, NULL);

	pthread_cond_destroy(&args.cond);
	pthread_mutex_destroy(&args.cond_lock);

	exit(args.res);
}
