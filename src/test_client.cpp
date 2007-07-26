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
 * \brief App to test iax2_client
 *
 * \note ECE 453 -- Fall 2006, Clemson University
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "iax2/iax2_client.h"
#include "iax2/iax2_event.h"
#include "iax2/iax2_lag.h"

static unsigned short call_num = 0;

static void iax2_event_dispatcher(iax2_event &event)
{
	event.print();

	if (event.get_type() == IAX2_EVENT_TYPE_CALL_ESTABLISHED)
		call_num = event.get_call_num();
}

struct run_client_args {
	pthread_cond_t cond;
	pthread_mutex_t cond_lock;
	iax2_client *client;
	int res;
};

static void *run_client(void *data)
{
	run_client_args *args = (run_client_args *) data;

	iax2_client client(DEFAULT_IAX2_PORT + 1);
	args->client = &client;
	client.register_event_handler(iax2_event_dispatcher);
	client.add_outbound_registration("test_client", "127.0.0.1", DEFAULT_IAX2_PORT);
	client.set_capabilities(IAX2_FORMAT_SLINEAR | IAX2_FORMAT_ULAW | IAX2_FORMAT_ALAW);
	args->res = client.run(&args->cond, &args->cond_lock);
}

int main(int argc, char *argv[])
{
	run_client_args args;
	pthread_t client_thread;
	
	args.res = 0;
	args.client = NULL;

	pthread_cond_init(&args.cond, NULL);
	pthread_mutex_init(&args.cond_lock, NULL);

	pthread_mutex_lock(&args.cond_lock);
	pthread_create(&client_thread, NULL, run_client, &args);
	pthread_cond_wait(&args.cond, &args.cond_lock);
	pthread_mutex_unlock(&args.cond_lock);
	usleep(3000000); // 3 seconds
	
	unsigned char fake_image[] = { 0x00, 0x01, 0x02, 0x03 };
	args.client->send_command(new iax2_command(IAX2_COMMAND_TYPE_VIDEO, call_num,
		fake_image, sizeof(fake_image)));

	pthread_join(client_thread, NULL);

	exit(args.res);
}
