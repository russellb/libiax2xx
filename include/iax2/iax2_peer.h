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
 * \brief IAX2 peer definitions
 */

#ifndef IAX2_PEER_H
#define IAX2_PEER_H

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <map>
#include <list>
#include <queue>

using namespace std;

#include "iax2/iax2_dialog.h"
#include "iax2/iax2_event.h"
#include "iax2/iax2_command.h"
#include "iax2/iax2_frame.h"
#include "iax2/time.h"

/*! The default IAX2 port */
#define DEFAULT_IAX2_PORT    4569

/*!
 * \brief A scheduled callback event
 *
 * This class is used internally to a peer to keep track of a scheduled
 * callback event that was created by an iax2_dialog.
 */
class iax2_timer_event {
public:
	iax2_timer_event(void);
	iax2_timer_event(iax2_dialog *dialog, struct timeval tv, unsigned int id_num);
	~iax2_timer_event(void);

	inline unsigned int get_id(void) const { return id; }
	inline iax2_dialog *get_dialog(void) const { return dialog; }
	inline struct timeval get_time_to_run(void) const { return time_to_run; }

	bool operator<(const iax2_timer_event& event) const;

private:
	unsigned int id;

	iax2_dialog *dialog;
	struct timeval time_to_run;
};

/*!
 * \brief An outbound registration
 *
 * This class is used internally to a peer to keep track of outbound
 * registrations that the application using this peer has requested be sent
 * once the peer is executed using run().
 */
struct iax2_outbound_registration {
	iax2_outbound_registration(const char *username, const char *ip, 
		const struct sockaddr_in *sin);
	~iax2_outbound_registration(void);

	inline const char *get_username(void) const
		{ return username; }

	inline const struct sockaddr_in *get_sin(void) const
		{ return &sin; }

private:
	const char *username;
	const char *ip;
	struct sockaddr_in sin;
};

/*!
 * \brief Implementation of an IAX2 peer
 *
 * This is a base class that implements all of the common functionality for any
 * type of IAX2 peer.  It should never be used directly by an application.  An
 * application should use one of the specific implementations of a peer which
 * inherit iax2_peer, such as iax2_client or iax2_server.
 */
class iax2_peer {
public:
	/*!
	 * \brief Default constructor for an iax2_peer
	 *
	 * This will bind the peer to the default IAX2 port, 4569.
	 */
	iax2_peer(void);

	/*!
	 * \brief Constructor for an iax2_peer
	 *
	 * \param local_port Local port to bind to
	 *
	 * The default IAX2 port is 4569, but a different one may be specified
	 * by using this constructor.
	 */
	iax2_peer(unsigned short local_port);
	
	/*!
	 * \brief Destructor for an iax2_peer
	 */
	virtual ~iax2_peer(void);

	/*!
	 * \brief Run the iax2 peer
	 *
	 * \param cond thread condition to signal once the server is up and running (optional)
	 * \param cond_lock lock associated with the condition (required IFF cond is provided)
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * Currently, this function is a blocking operation that must be called
	 * after calling all of the other necessary functions to set up the peer.
	 * To request that this function returns, you send the peer a command of
	 * type IAX2_COMMAND_TYPE_SHUTDOWN.
	 */
	int run(pthread_cond_t *cond, pthread_mutex_t *cond_lock);

	/*!
	 * \brief This is the type for an event handler
	 */
	typedef void (*iax2_event_handler)(iax2_event &);

	/*!
	 * \brief Register a handler for events from this peer.
	 *
	 * \param handler the event handler
	 *
	 * An application that uses the library to create a type of an iax2_peer
	 * should register at least one function to handle events as this is the
	 * method that the library uses to communicate back with the application,
	 * to pass information about things such as registrations, as well as the
	 * actutal media during a call that is passing media.
	 */
	int register_event_handler(iax2_event_handler handler);

	/*!
	 * \brief Add an outbound registration for this peer
	 *
	 * \param username the username to use when registering
	 * \param ip the IP address to register to
	 * \param port the remote port to use for the registration
	 *
	 * This function should be called by an application that is creating the
	 * peer.  It MUST be called BEFORE run().  When the peer starts running,
	 * it will attempt to send out all of the registrations that were added
	 * using this function, and refresh them as appropriate.
	 */
	void add_outbound_registration(const char *username, const char *ip, 
		unsigned short port);

	/*!
	 * \brief Start a new call
	 *
	 * \param uri the URI that identifies the IAX2 peer to call
	 *
	 * \return the call number for the new call. 0 is returned
	 *         for an error.
 	 */
	unsigned short new_call(const char *uri);

	/*!
	 * \brief Start a new lag
	 *
	 * \param uri the URI that identifies the IAX2 peer to send lag request to
	 *
	 * \return the call number for the new call. 0 is returned
	 *         for an error.
 	 */
	unsigned short new_lag(const char *uri);

	/*!
	 * \brief Send a command for an active call
	 *
	 * \param command The command to send in the call.  This must be dynamically
	 *        allocated using the new operator.
	 *
	 * \return whether or not the command was successful
	 */
	enum iax2_command_result send_command(iax2_command *command);

	/*!
	 * \brief Set the codec capabilities for this peer.
	 *
	 * This sets the codec capabilities that this peer will offer and accept.
	 * It is a bitmask, and the values are defined in iax2_frame.h. The default
	 * is just IAX2_FORMAT_SLINEAR if this function is not called.
	 *
	 * This value is global.  There is currently no way to set capabilities
	 * based on which peer the call is to or from.
 	 */
	void set_capabilities(unsigned int cap);

	/*!
	 * \brief Get the currently set codec capabilities for this peer.
	 *	 
	 * This value is global.  There is currently no way to set capabilities
	 * based on which peer the call is to or from.
	 */
	inline unsigned int get_capabilities(void) const
		{ return capabilities; }

	u_int32_t choose_formats(u_int32_t peer_capabilities);

	/*!
	 * \brief Get the preferred format, based on capabilities
	 *
 	 * This function returns our opinion of the "best" format, based on the
	 * current set of capabilities.
	 *
 	 * \note This can actually be *two* formats - one for audio, and one for
	 *       video.
	 */
	inline unsigned int get_preferred_format(void) const
		{ return preferred_format; }

	u_int32_t choose_formats(u_int32_t peer_capabilities) const;

	/*!
	 * \brief Schedule a callback
	 *
	 * \param dialog the dialog that started this timer
	 * \param tv the time to call the function
	 *
	 * \return The identifier of the timer which can be used to
	 *         stop it from running if necessary.  Once stopped, it is
	 *         removed from the queue.
	 *
	 * \note This function should not be used by the application using the
	 *       library.  It is called by dialogs, which are internal to the
	 *       library.
	 *
	 * \todo It would be nice if this were moved so it was not public.
	 */
	unsigned int start_timer(iax2_dialog *dialog, struct timeval tv);

	/*!
	 * \brief Remove a callback event
	 *
	 * \param id the id returned from schedule_callback()
	 *
	 * \retval 0 success
	 * \retval non-zero failure
	 *
	 * \note This function should not be used by the application using the
	 *       library.  It is called by dialogs, which are internal to the
	 *       library.
	 *
	 * \todo It would be nice if this were moved so it was not public.
	 */
	int stop_timer(unsigned int id);

	/*!
	 * \brief Queue an event to be dispatched to the event event_handlers
	 *
	 * \note This function should not be used by the application using the
	 *       library.  It is used by objects internal to the library to
	 *       communicate information back to the application.
	 *
	 * \todo It would be nice if this were moved so it was not public.
	 */ 
	void queue_event(iax2_event *event);

	/*!
	 * \brief Get the reference time
	 *
	 * \param none
	 *
	 * \return the reference time struct timeval
	 *
	 * \todo It would be nice if this were moved so it was not public.
	 */
	inline struct timeval get_reference_time(void) const 
		{ return reference_time; }

protected:
	/*!
	 * \brief Determine when the next callback is scheduled for
	 * 
	 * \return The number of milliseconds until the next callback
	 *
	 * This analyzes all of the timers that have been started with
	 * start_timer() and returns the number of milliseconds until the
	 * next timer will be up.
	 */
	int next_callback_time(void);

	/*!
	 * \brief Run callbacks that have reached their time
	 *
	 * This function is used after a call to next_callback_time() has
	 * indicated that there is at least one timer that is up and thus, its
	 * associated callback needs to be called.  If there is more than one
	 * ready, it will call all of them.  If a timer finishes during the time
	 * it takes to service the other callbacks, it will also be called.
	 */
	void run_callbacks(void);
  
	unsigned short get_next_call_num(void);

	/*
	 * \brief Find the dialog for a media frame
	 *
	 * This function returns a pointer for the active dialog that a media frame
	 * is destined for.  It must match the dialog based on source call number,
	 * IP address, and port number.
	 */
	iax2_dialog *find_dialog_media(iax2_frame &frame, const struct sockaddr_in *sin);

	virtual void handle_newcall_command(iax2_command &command) = 0;
	virtual void handle_lagrq_command(iax2_command &command) = 0;

	/*! 
	 * \brief the socket used for sending and receiving messages 
	 *
	 * This socket is created by the network_init() function and will be closed
	 * when this peer's destructor gets called.
	 */
	int sockfd;
	
	/*! 
	 * \brief the currently active dialogs 
 	 *
	 * This is the container for all of the active dialogs for this peer.
	 * It is implemented as a map for convenience.  The unsigned short used
	 * for the mapping should be the call number associated with the dialog.
	 * Every incoming frame has an associated call number, so this makes it
	 * very easy to look up which dialog the incoming frame is associated
	 * with.
	 */
	map<unsigned short, iax2_dialog *> dialogs;
	typedef map<unsigned short, iax2_dialog *>::const_iterator dialogs_iterator;

private:
	/*!
	 * \brief Common initialization
	 *
	 * This function performs initialization steps common to all constructors.
	 */
	void common_init(void);

	/*!
	 * \brief Create and bind socket
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * This function is called when the peer's run() function gets called to
	 * create and bind the socket used for this peer.
	 */
	int network_init(void);

	/*!
	 * \brief Read a packet from the socket
	 *
	 * This function gets called after polling the socket has indicated that
	 * there is input available on the socket so that this function will
	 * never block when it tries to read from the socket.
	 */
	void recv_packet(void);

	int handle_command(void);

	/*!
	 * \brief the list of outbound registrations
	 *
	 * This list is built by the application by calling add_outbound_registration().
	 * Once the run() function is called, all of these registrations are started
	 * and the entires in this list will all be deleted.
	 */
	list<iax2_outbound_registration *> outbound_registrations;
	/*! A const_iterator type for iterating the outbound_registrations list */
	typedef list<iax2_outbound_registration *>::const_iterator iax2_out_reg_iterator;

	/*!
	 * \brief start outbound registrations
	 *
	 * This function gets called when the peer's run() function gets called
	 * and iterates the outbound_registrations list to start all of the
	 * outbound registrations that the application added when setting up the
	 * peer.
	 */
	void start_registrations(void);

	/*!
	 * \brief Process an incoming frame
	 *
	 * The actions taken based on an incoming frame are specific to the type
	 * peer.  For example, a server would want to process incoming registrations,
	 * but a simple client would want to ignore them.  Because of this, this
	 * function is declared as pure virtual, and must be implemented by the
	 * specific peer type.
	 */
	virtual void process_incoming_frame(iax2_frame &frame, const struct sockaddr_in *sin) = 0;

	/*! Local port and address to bind to */
	struct sockaddr_in local_addr;

	/*! 
	 * \brief next call number to use
	 *
	 * This number is used to figure out what number should be used for the
	 * next call number for a dialog.  After this number is used for creating
	 * a new dialog, it should be incremented.
	 */
	unsigned short next_call_num;
	pthread_mutex_t next_call_num_lock;

	priority_queue<iax2_timer_event> callback_queue;
	unsigned int next_timer_id;

	static void *event_dispatcher(void *data);

	/*! Set to false to notify the event_dispatcher to return */
	bool event_dispatch;
	pthread_t event_dispatch_thread;
	pthread_cond_t event_cond;
	pthread_mutex_t event_cond_lock;

	pthread_mutex_t event_handlers_lock;
	list<iax2_event_handler> event_handlers;
	typedef list<iax2_event_handler>::const_iterator iax2_event_handler_iterator;

	pthread_mutex_t event_queue_lock;
	queue<iax2_event *> event_queue;

	pthread_mutex_t command_queue_lock;
	queue<iax2_command *> command_queue;

	int command_alert_pipe[2];

	struct timeval reference_time;

	unsigned int capabilities;
	unsigned int preferred_format;
};

#endif /* IAX2_PEER_H */
