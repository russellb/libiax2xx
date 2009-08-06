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
 * \brief IAX2 peer
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef POLL_COMPAT
#include "poll-compat.h"
#else
#include <poll.h>
#endif

using namespace std;

#include "iax2/iax2_peer.h"
#include "iax2/iax2_frame.h"
#include "iax2/iax2_dialog.h"

using namespace iax2xx;

/* Borrowed from Asterisk - http://www.asterisk.org/
 * Licensed under the GPL.
 * Copyright (C) 1999 - 2006, Digium, Inc.
 * Mark Spencer <markster@digium.com>
 */
static int iax2_audio_prefs[] = {
	/*! Okay, ulaw is used by all telephony equipment, so start with it */
	IAX2_FORMAT_ULAW,
	/*! Unless of course, you're a silly European, so then prefer ALAW */
	IAX2_FORMAT_ALAW,
	/*! Okay, well, signed linear is easy to translate into other stuff */
	IAX2_FORMAT_SLINEAR,
	/*! G.726 is standard ADPCM, in RFC3551 packing order */
	IAX2_FORMAT_G726,
	/*! G.726 is standard ADPCM, in AAL2 packing order */
	IAX2_FORMAT_G726_AAL2,
	/*! ADPCM has great sound quality and is still pretty easy to translate */
	IAX2_FORMAT_ADPCM,
	/*! Okay, we're down to vocoders now, so pick GSM because it's small and easier to
    	translate and sounds pretty good */
	IAX2_FORMAT_GSM,
	/*! iLBC is not too bad */
	IAX2_FORMAT_ILBC,
	/*! Speex is free, but computationally more expensive than GSM */
	IAX2_FORMAT_SPEEX,
	/*! Ick, LPC10 sounds terrible, but at least we have code for it, if you're tacky enough
    	to use it */
	IAX2_FORMAT_LPC10,
	/*! G.729a is faster than 723 and slightly less expensive */
	IAX2_FORMAT_G729A,
	/*! Down to G.723.1 which is proprietary but at least designed for voice */
	IAX2_FORMAT_G723_1,
};

// XXX This is arbitrary right now!
static int iax2_video_prefs[] = {
	/*! JPEG Images */
	IAX2_FORMAT_JPEG,
	/*! PNG Images */
	IAX2_FORMAT_PNG,
	/*! H.261 Video */
	IAX2_FORMAT_H261,
	/*! H.263 Video */
	IAX2_FORMAT_H263,
	/*! H.263+ Video */
	IAX2_FORMAT_H263_PLUS,
	/*! H.264 Video */
	IAX2_FORMAT_H264,
};

iax2_peer::iax2_peer(void) : 
	sockfd(-1), next_call_num(1), next_timer_id(1), event_dispatch(true),
	capabilities(IAX2_FORMAT_SLINEAR), preferred_format(IAX2_FORMAT_SLINEAR)
{
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(DEFAULT_IAX2_PORT);

	common_init();
}

iax2_peer::iax2_peer(unsigned short local_port) : 
	sockfd(-1), next_call_num(1), next_timer_id(1), event_dispatch(true),
	capabilities(IAX2_FORMAT_SLINEAR), preferred_format(IAX2_FORMAT_SLINEAR)
{
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(local_port);

	common_init();
}

iax2_peer::~iax2_peer(void)
{
	if (sockfd > -1)
		close(sockfd);

	if (command_alert_pipe[0] > -1)
		close(command_alert_pipe[0]);
	if (command_alert_pipe[1] > -1)
		close(command_alert_pipe[1]);

	// Shut down the event_dispatcher thread.
	event_dispatch = false;
	// The event_dispatcher thread may be sleeping at this point, so it must be
	// signalled to wake up.
	pthread_mutex_lock(&event_cond_lock);
	pthread_cond_signal(&event_cond);
	pthread_mutex_unlock(&event_cond_lock);
	// Now, wait for the thread to actually exit.
	pthread_join(event_dispatch_thread, NULL);

	pthread_mutex_destroy(&next_call_num_lock);
	pthread_mutex_destroy(&event_queue_lock);
	pthread_mutex_destroy(&command_queue_lock);
	pthread_mutex_destroy(&event_handlers_lock);
	pthread_mutex_destroy(&event_cond_lock);
	
	pthread_cond_destroy(&event_cond);
}

void iax2_peer::common_init(void)
{
	pthread_mutex_init(&next_call_num_lock, NULL);
	pthread_mutex_init(&event_queue_lock, NULL);
	pthread_mutex_init(&command_queue_lock, NULL);
	pthread_mutex_init(&event_handlers_lock, NULL);
	pthread_mutex_init(&event_cond_lock, NULL);

	pthread_cond_init(&event_cond, NULL);

	pthread_mutex_lock(&event_cond_lock);
	pthread_create(&event_dispatch_thread, NULL, event_dispatcher, this);
	pthread_cond_wait(&event_cond, &event_cond_lock);
	pthread_mutex_unlock(&event_cond_lock);	

	outbound_registrations.clear();

	command_alert_pipe[0] = command_alert_pipe[1] = -1;
	if (pipe(command_alert_pipe))
		printf("Failed to create command alert pipe! (%s)\n", strerror(errno));

	reference_time = tvnow();
}

unsigned short iax2_peer::get_next_call_num(void)
{
	unsigned short num;

	pthread_mutex_lock(&next_call_num_lock);
	num = next_call_num++;
	pthread_mutex_unlock(&next_call_num_lock);

	return num;
}

void iax2_peer::recv_packet(void)
{
	unsigned char buf[4096];
	ssize_t res;
	struct sockaddr_in sin;
	socklen_t len = (socklen_t) sizeof(sin);

	res = recvfrom(sockfd, static_cast<void *>(&buf), sizeof(buf), 0, 
			(struct sockaddr *) &sin, &len);

	if (res < 0) {
		printf("recv error (%d): %s\n", errno, strerror(errno));
		return;
	}
	
	iax2_frame frame(buf, res);
	frame.print(&sin);

	process_incoming_frame(frame, &sin);
}

int iax2_peer::network_init(void)
{
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Unable to create socket: %s\n", strerror(errno));
		return -1;
	}

	if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr))) {
		printf("Unable to bind socket to port '%d': %s\n", ntohs(local_addr.sin_port), strerror(errno));
		return -1;
	}

	return 0;
}

void iax2_peer::start_registrations(void)
{
	while (!outbound_registrations.empty()) {
		iax2_outbound_registration *reg = outbound_registrations.front();
		outbound_registrations.pop_front();
		iax2_register_dialog *dialog = new iax2_register_dialog(this, 
			get_next_call_num(), sockfd, reg->get_sin());
 		dialogs[dialog->get_call_num()] = dialog;
 		dialog->start(reg->get_username());
		delete reg;
	}
}

int iax2_peer::run(pthread_cond_t *cond, pthread_mutex_t *cond_lock)
{
	if (network_init())
		return -1;

	start_registrations();

	struct pollfd pollfds[3]; // Need an extra for swapping the order
	bool switched = false;
	// Wait for commands from the application
	pollfds[0].fd = command_alert_pipe[0];
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;	

	// Wait for input on the socket
	pollfds[1].fd = sockfd;
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;	

	// Signal back to the application that the peer is up and running
	if (cond && cond_lock) {
		pthread_mutex_lock(cond_lock);
		pthread_cond_signal(cond);
		pthread_mutex_unlock(cond_lock);
	}

	for (;;) {
		int res;
		int timeout = next_callback_time();
		if (!timeout) {
			run_callbacks();
			continue;
		}
		if ((res = poll(pollfds, sizeof(pollfds) / sizeof(pollfds[0]), 
			timeout)) >= 1) {
			// There is input on the socket and/or command pipe
			if (pollfds[(switched ? 1 : 0)].revents > 0) {
				if (handle_command()) 
					break; // IAX2_COMMAND_TYPE_SHUTDOWN
			}
			if (pollfds[(switched ? 0 : 1)].revents > 0)
				recv_packet();
		} else if (!res) {
			// poll() timed out, meaning a timer has expired
			run_callbacks();
		} else if (res != EINTR) {
			// poll() returned some kind of error.  However, we ignore it if
			// it was just an interrupted system call.
			printf("iax2_peer::run() - poll returned error: %s\n", 
				strerror(errno));
		}

		// Swap poll() priority.
		pollfds[2].fd = pollfds[0].fd;
		pollfds[0].fd = pollfds[1].fd;
		pollfds[1].fd = pollfds[2].fd;
		switched = !switched;
	}

	return 0;
}

int iax2_peer::handle_command(void)
{
	pthread_mutex_lock(&command_queue_lock);
	while (!command_queue.empty()) {
		int alert;
		read(command_alert_pipe[0], &alert, sizeof(alert));

		iax2_command *command = command_queue.front();
		command_queue.pop();

		// Leave the command queue unlocked while the current command is processed
		pthread_mutex_unlock(&command_queue_lock);

		if (command->get_type() == IAX2_COMMAND_TYPE_NEW) {
			handle_newcall_command(*command);
			delete command;
			pthread_mutex_lock(&command_queue_lock);
			continue;
		} else if (command->get_type() == IAX2_COMMAND_TYPE_LAGRQ) {
			handle_lagrq_command(*command);
			delete command;
			pthread_mutex_lock(&command_queue_lock);
			continue;
		} else if (command->get_type() == IAX2_COMMAND_TYPE_SHUTDOWN) {
			delete command;
			return -1;
		}

		iax2_dialog *dialog = dialogs[command->get_call_num()];
		if (!dialog) {
			printf("Found no dialog for command with call_num '%u'\n", 
				command->get_call_num());
			delete command;
			pthread_mutex_lock(&command_queue_lock);
			continue;
		}

		dialog->process_command(*command);
		delete command;
		pthread_mutex_lock(&command_queue_lock);
	}
	pthread_mutex_unlock(&command_queue_lock);

	return 0;
}

unsigned int iax2_peer::start_timer(iax2_dialog *dialog, struct timeval tv)
{
	int res;

	callback_queue.push(iax2_timer_event(dialog, tv, next_timer_id));
	res = next_timer_id++;

	return res;
}

int iax2_peer::stop_timer(unsigned int id)
{
	int res = -1;
	priority_queue<iax2_timer_event> tmp_queue;
	iax2_timer_event event;

	while (!callback_queue.empty()) {
		event = callback_queue.top();
		callback_queue.pop();
		if (event.get_id() == id) {
			res = 0;
			break;
		}
		tmp_queue.push(event);
	}

	while (!tmp_queue.empty()) {
		callback_queue.push(tmp_queue.top());
		tmp_queue.pop();
	}

	return res;
}

int iax2_peer::next_callback_time(void)
{
	struct timeval now, next;
	int res;

	if (callback_queue.empty())
		return -1;

	now = tvnow();

	next = callback_queue.top().get_time_to_run();

	return ((res = tvdiff_ms(next, now)) < 0) ? 0 : res;
}

void iax2_peer::run_callbacks(void)
{
	iax2_timer_event event;	

	while (!callback_queue.empty()) {
		if (next_callback_time() > 0)
			return;
		
		event = callback_queue.top();
		callback_queue.pop();

		switch (event.get_dialog()->timer_callback()) {
		case IAX2_DIALOG_RESULT_SUCCESS:
			break;
		case IAX2_DIALOG_RESULT_DESTROY:
 			dialogs.erase(event.get_dialog()->get_call_num());
	 		delete event.get_dialog();
 			break;
		case IAX2_DIALOG_RESULT_DELETE:
			delete event.get_dialog();
			break;
		default:
			break;			
		}
	}
}

int iax2_peer::register_event_handler(iax2_event_handler handler)
{
	pthread_mutex_lock(&event_handlers_lock);
	event_handlers.push_back(handler);
	pthread_mutex_unlock(&event_handlers_lock);

	return 0;
}

void iax2_peer::queue_event(iax2_event *event)
{
	if (!event)
		return;

	pthread_mutex_lock(&event_queue_lock);
	event_queue.push(event);
	pthread_mutex_unlock(&event_queue_lock);

	pthread_mutex_lock(&event_cond_lock);
	pthread_cond_signal(&event_cond);
	pthread_mutex_unlock(&event_cond_lock);
}

void *iax2_peer::event_dispatcher(void *data)
{
	iax2_peer *_this = (iax2_peer *) data;

	// Notify the constructor that the thread is running
	pthread_mutex_lock(&_this->event_cond_lock);
	pthread_cond_signal(&_this->event_cond);
	pthread_mutex_unlock(&_this->event_cond_lock);

	while (_this->event_dispatch) {
		pthread_mutex_lock(&_this->event_queue_lock);
		while (!_this->event_queue.empty()) {
			iax2_event *event = _this->event_queue.front();
			_this->event_queue.pop();
			pthread_mutex_unlock(&_this->event_queue_lock);
			// Call the event handlers without the event queue locked so that it doesn't
			// block queueing more events
			pthread_mutex_lock(&_this->event_handlers_lock);
			for (iax2_event_handler_iterator i = _this->event_handlers.begin(); 
			     i != _this->event_handlers.end(); i++) {
				(*i)(*event);
			}
			pthread_mutex_unlock(&_this->event_handlers_lock);
			delete event;
			pthread_mutex_lock(&_this->event_queue_lock);
		}
		pthread_mutex_unlock(&_this->event_queue_lock);

		// Sleep until there is another event to dispatch, or the thread
		// needs to stop.
		pthread_mutex_lock(&_this->event_cond_lock);
		pthread_cond_wait(&_this->event_cond, &_this->event_cond_lock);
		pthread_mutex_unlock(&_this->event_cond_lock);
	}

	return NULL;
}

void iax2_peer::add_outbound_registration(const char *username, 
	const char *ip, unsigned short port)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	inet_aton(ip, &sin.sin_addr);

	outbound_registrations.push_back(new 
		iax2_outbound_registration(username, ip, &sin));	
}

unsigned short iax2_peer::new_call(const char *uri)
{
	unsigned short num = get_next_call_num();

	send_command(new iax2_command(IAX2_COMMAND_TYPE_NEW, num, uri));

	return num;
}

unsigned short iax2_peer::new_lag(const char *uri)
{
	unsigned short num = get_next_call_num();

	send_command(new iax2_command(IAX2_COMMAND_TYPE_LAGRQ, num, uri));

	return num;
}

enum iax2_command_result iax2_peer::send_command(iax2_command *command)
{
	int alert = 0;

	pthread_mutex_lock(&command_queue_lock);
	command_queue.push(command);	
	write(command_alert_pipe[1], &alert, sizeof(alert));
	pthread_mutex_unlock(&command_queue_lock);
}

void iax2_peer::set_capabilities(unsigned int cap)
{
	capabilities = cap;

    /* Find the first preferred codec in the format given */
	for (unsigned int x = 0; (capabilities & IAX2_FORMAT_AUDIO_MASK) &&
		x < (sizeof(iax2_audio_prefs) / sizeof(iax2_audio_prefs[0]) ); x++) {
		if (cap & iax2_audio_prefs[x]) {
			preferred_format = iax2_audio_prefs[x];
			break;
		}
	}

	/* Find the first preferred codec in the format given */
	for (unsigned int x = 0; (capabilities & IAX2_FORMAT_VIDEO_MASK) &&
		x < (sizeof(iax2_video_prefs) / sizeof(iax2_video_prefs[0]) ); x++) {
		if (cap & iax2_video_prefs[x]) {
			preferred_format |= iax2_video_prefs[x];
			break;
		}
	}
}

u_int32_t iax2_peer::choose_formats(u_int32_t peer_capabilities)
{
	u_int32_t common_capabilities = capabilities & peer_capabilities;
	u_int32_t res = 0;

    /* Find the first preferred codec in the format given */
	for (unsigned int x = 0; (common_capabilities & IAX2_FORMAT_AUDIO_MASK) &&
		x < (sizeof(iax2_audio_prefs) / sizeof(iax2_audio_prefs[0]) ); x++) {
		if (common_capabilities & iax2_audio_prefs[x]) {
			res = iax2_audio_prefs[x];
			break;
		}
	}

	/* Find the first preferred codec in the format given */
	for (unsigned int x = 0; (common_capabilities & IAX2_FORMAT_VIDEO_MASK) &&
		x < (sizeof(iax2_video_prefs) / sizeof(iax2_video_prefs[0]) ); x++) {
		if (common_capabilities & iax2_video_prefs[x]) {
			res |= iax2_video_prefs[x];
			break;
		}
	}

	return res;
}

iax2_dialog *iax2_peer::find_dialog_media(iax2_frame &frame, const struct sockaddr_in *sin)
{
	// BEGIN RANT:  IAX2 media frames (mini and meta) carry the
	// *source* call number instead of the destination call number, and
	// that really annoys me.  If it was the destination call number,
	// then we would be able to immediately directly access the right
	// dialog since we'd have the unique key.  Instead, we have to do
	// a more complicated lookup.  The only reason we can't change it
	// is because it's been like this in the protocol for too long.
	// Oh well.  END RANT.

	// XXX \todo Optimize this search, since it is a VERY frequent operation.

	iax2_dialog *d = NULL;
	
	for (dialogs_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
		// Match by source call number, IP address, and port number
		d = i->second;
		if (!d)
			continue;
		if (d->get_remote_call_num() != frame.get_source_call_num())
			continue;
		if (d->get_remote_addr()->sin_addr.s_addr != sin->sin_addr.s_addr)
			continue;
		if (d->get_remote_addr()->sin_port != sin->sin_port)
			continue;
		break;
	}

	return d;
}
///////////////////////////////////////////////////////////////////////////////


iax2_outbound_registration::iax2_outbound_registration(const char *un, 
	const char *addr, const struct sockaddr_in *s)
{
	username = strdup(un);
	ip = strdup(addr);
	memcpy(&sin, s, sizeof(sin));
}

iax2_outbound_registration::~iax2_outbound_registration(void)
{
	if (username)
		free((void *) username);
	if (ip)
		free((void *) ip);
}

///////////////////////////////////////////////////////////////////////////////

iax2_timer_event::iax2_timer_event(void) :
	id(0), dialog(NULL)
{
	time_to_run.tv_sec = 0;
	time_to_run.tv_usec = 0;
}

iax2_timer_event::iax2_timer_event(iax2_dialog *dlg,
	struct timeval tv, unsigned int id_num) : 
	id(id_num), dialog(dlg), time_to_run(tv)
{
}

iax2_timer_event::~iax2_timer_event(void) 
{
}

/* This probably looks kind of backwards.  However, it is the way it is supposed
 * to be.  These entries go into a priority queue and this is how you make them
 * get sorted correctly. */
bool iax2_timer_event::operator<(const iax2_timer_event &e) const
{
	if (time_to_run.tv_sec < e.time_to_run.tv_sec)
		return false;
	else if (time_to_run.tv_sec != e.time_to_run.tv_sec)
		return true;
	else if (time_to_run.tv_usec < e.time_to_run.tv_usec)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////


