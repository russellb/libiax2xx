/*
 * Copyright (C) 2007, Russell Bryant <russell@russellbryant.net>
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
 * \brief App to generate IAX2 frames
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>

using namespace std;

#include "iax2/iax2_peer.h"
#include "iax2/iax2_frame.h"

static const char USAGE[] = ""
"\n"
"Usage:\n"
"  ./iaxpacket [options]\n"
"For a full listing of options, use the --help option.\n"
"\n"
"";

static const char USAGE_FULL[] = ""
"\n"
"Usage:\n"
"  ./iaxpacket [options]\n"
"\n"
"  Required Arguments:\n"
"  ============================================================\n"
"    --ip <ipaddr[:port]> | -i <ipaddr[:port]>\n"
"         Specify the destination IP address and port number.\n"
"\n"
"    --shell <value> | -f <value>\n"
"         Specify the \"shell\" for the frame.\n"
"         This can either be FULL, MINI, or META\n"
"\n"
"  Optional Arguments:\n"
"  ============================================================\n"
"    Frame Parameters:\n"
"    ----------------------------------------------------------\n"
"    --type <type> | -t <type>\n"
"         Set the Frame type.  For FULL frames, the options are:\n"
"           DTMF_END, VOICE, VIDEO, CONTROL, NULL, IAX2, TEXT,\n"
"           IMAGE, HTML, CNG, MODEM, DTMF_BEGIN\n"
"\n"
"    --metatype <type> | -m <type>\n"
"         Set the type for a META frame.\n"
"\n"
"    --subclass <type> | -s <value>\n"
"         Set the frame subclass.  For a FULL frame of type IAX2,\n"
"         valid values are:\n"
"           NEW, PING, PONG, ACK, HANGUP, REJECT, ACCEPT, AUTHREQ,\n"
"           AUTHREP, INVAL, LAGRQ, LAGRP, REGREQ, REGAUTH, REGACK,\n"
"           REGREJ, REGREL, VNAK, DPREQ, DPREP, DIAL, TXREQ, TXCNT,\n"
"           TXACC, TXREADY, TXREL, TXREJ, QUELCH, UNQUELCH, POKE,\n"
"           MWI, UNSUPPORT, TRANSFER, PROVISION, FWDOWNL, FWDATA\n"
"\n"
"    --source_call_num <num> | -S <num>\n"
"         Set the source call number.\n"
"\n"
"    --dest_call_num <num> | -D <num>\n"
"         Set the destination call number.\n"
"\n"
"    --in_seq_num <num> | -I <num>\n"
"         Set the in sequence number.\n"
"\n"
"    --out_seq_num <num> | -O <num>\n"
"         Set the out sequence number.\n"
"\n"
"    --timestamp <num> | -T <num>\n"
"         Set the timestamp (taken in unsigned decimal)\n"
"\n"
"    --retransmission <val> | -r <val>\n"
"         Set the retransmission flag to either TRUE or FALSE\n"
"\n"
"    --ie_string <IE_NAME=string_value> | -R\n"
"\n"
"    --ie_ushort <IE_NAME=unsigned_short_value> | -o\n"
"\n"
"    --ie_ulong <IE_NAME=unsigned_long_value> | -l\n"
"\n"
"    Other Parameters:\n"
"    ------------------------------------------------------------\n"
"    --help | -h\n"
"         Print usage information\n"
"\n"
"    --wait_call_num | -W\n"
"         Wait until a frame is received and print the source\n"
"         call number that is in the received frame.\n"
"\n"
"";

static struct {
	unsigned int wait_call_num:1;
} g_flags;

static void handle_ip_opt(struct sockaddr_in &remote_addr, const char *val)
{
	char *ip, *port;
	char buf[64];

	strncpy(buf, val, sizeof(buf) - 1);
	port = buf;
	ip = strsep(&port, ":");
	if (!inet_aton(ip, &remote_addr.sin_addr)) {
		fprintf(stderr, "'%s' is not a valid IP address to send to\n", ip);
		exit(1);
	}
	if (port)
		remote_addr.sin_port = htons((uint16_t) atoi(port));
}

static int parse_args(int argc, char *argv[], iax2_frame &frame,
	struct sockaddr_in &local_addr, struct sockaddr_in &remote_addr)
{
	static const struct option long_opts[] = {
		{ "dest_call_num",   required_argument, NULL, 'D' },
		{ "help",            no_argument,       NULL, 'h' },
		{ "ie_string",       required_argument, NULL, 'R' },
		{ "ie_ushort",       required_argument, NULL, 'o' },
		{ "ie_ulong",        required_argument, NULL, 'l' },
		{ "ie_empty",        required_argument, NULL, 'e' },
		{ "in_seq_num",      required_argument, NULL, 'I' },
		{ "ip",              required_argument, NULL, 'i' },
		{ "out_seq_num",     required_argument, NULL, 'O' },
		{ "retransmission",  required_argument, NULL, 'r' },
		{ "shell",           required_argument, NULL, 'f' },
		{ "source_call_num", required_argument, NULL, 'S' },
		{ "subclass",        required_argument, NULL, 's' },
		{ "timestamp",       required_argument, NULL, 'T' },
		{ "type",            required_argument, NULL, 't' },
		{ "wait_call_num",   no_argument,       NULL, 'W' },
		{ NULL,              0,                 NULL,  0  },
	};
	static const char short_opts[] = "D:hf:i:I:O:s:S:t:W";
	int ch;

#define FRAME_ARG_STR(name) do { \
	if (frame.set_##name(optarg)) { \
		fprintf(stderr, "'%s' is not a valid arg to --" # name "\n", optarg); \
		exit(1); \
	} \
} while (0)

#define FRAME_ARG_UINT(name) do { \
	unsigned int num; \
	if (sscanf(optarg, "%u", &num) == 1) \
		frame.set_##name(num); \
	else { \
		fprintf(stderr, "'%s' is not a valid arg to --" # name "\n", optarg); \
		exit(1); \
	} \
} while (0)

#define FRAME_ARG_TF(name) do { \
	frame.set_##name(strcasecmp(optarg, "TRUE") ? false : true); \
} while (0)

	frame.set_direction(IAX2_DIRECTION_OUT);

	while ((ch = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (ch) {
		case 'D':
			FRAME_ARG_UINT(dest_call_num);
			break;
		case 'f':
			FRAME_ARG_STR(shell);
			break;
		case 'i':
			handle_ip_opt(remote_addr, optarg);
			break;
		case 'I':
			FRAME_ARG_UINT(in_seq_num);
			break;
		case 'O':
			FRAME_ARG_UINT(out_seq_num);
			break;
		case 'r':
			FRAME_ARG_TF(retransmission);
			break;
		case 'R':
		case 'l':
		case 'o':
		{
			char buf[512];
			char *ie, *val = buf;
			strncpy(buf, optarg, sizeof(buf) - 1);
			val = buf;
			ie = strsep(&val, "=");
			if (!ie || !val) {
				fprintf(stderr, "'%s' is not a valid arg to --ie_string\n", optarg);
				exit(1);
			}
			if (ch == 'R')
				frame.add_ie_string(ie, val);
			else if (ch == 'l') {
				uint32_t ie_val;
				if (sscanf(val, "%u", &ie_val) != 1) {
					fprintf(stderr, "'%s' is not a valid arg to --ie_ulong\n", val);
					exit(1);
				}
				frame.add_ie_unsigned_long(ie, ie_val);
			} else if (ch == 'o') {
				uint16_t ie_val;
				if (sscanf(val, "%hu", &ie_val) != 1) {
					fprintf(stderr, "'%s' is not a valid arg to --ie_ushort\n", val);
					exit(1);
				}
				frame.add_ie_unsigned_short(ie, ie_val);
			}
			break;
		}
		case 'e':
			frame.add_ie_empty(optarg);
			break;
		case 's':
			FRAME_ARG_STR(subclass);
			break;
		case 'S':
			FRAME_ARG_UINT(source_call_num);
			break;
		case 't':
			FRAME_ARG_STR(type);
			break;
		case 'T':
			FRAME_ARG_UINT(timestamp);
			break;
		case 'W':
			g_flags.wait_call_num = 1;
			break;
		case 'h':
		default:
			fprintf(stderr, "%s", USAGE_FULL);
			exit(0);
		}
	}

#undef FRAME_ARG_STR
#undef FRAME_ARG_UINT
#undef FRAME_ARG_TF

	return 0;
}

int main(int argc, char *argv[])
{
	iax2_frame frame;
	struct sockaddr_in remote_addr;
	struct sockaddr_in local_addr;
	int sockfd;

	if (argc == 1) {
		fprintf(stderr, "%s", USAGE);
		exit(1);
	}

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(DEFAULT_IAX2_PORT);

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(DEFAULT_IAX2_PORT + 1);

	if (parse_args(argc, argv, frame, local_addr, remote_addr)) {
		fprintf(stderr, "Invalid Arguments\n%s", USAGE);
		exit(1);
	}

	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Unable to create socket: %s\n", strerror(errno));
		exit(1);
	}

	if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr))) {
		fprintf(stderr, "Unable to bind socket to port '%d': %s\n", ntohs(local_addr.sin_port), strerror(errno));
		exit(1);
	}

	if (frame.send(&remote_addr, sockfd)) {
		fprintf(stderr, "Error sending packet!\n");
		exit(1);
	}

	if (!g_flags.wait_call_num)
		exit(0);

	unsigned char pkt_buf[4096];
	socklen_t addr_len = sizeof(remote_addr);
	int res;
	res = recvfrom(sockfd, pkt_buf, sizeof(pkt_buf), 0, (struct sockaddr *) &remote_addr, &addr_len);
	if (res == -1) {
		fprintf(stderr, "recvfrom failed\n");
		exit(1);
	}

	iax2_frame recv_frame(pkt_buf, res);
	cout << recv_frame.get_source_call_num() << "\n";

	exit(0);
}
