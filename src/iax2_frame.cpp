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
 * \brief IAX2 frame
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

using namespace std;

#include "iax2/iax2_frame.h"

iax2_frame::iax2_frame(void) :
	direction(IAX2_DIRECTION_UNKNOWN), shell(IAX2_FRAME_UNDEFINED), 
	type(IAX2_FRAME_TYPE_UNDEFINED), source_call_num(0), dest_call_num(0),
	timestamp(0), out_seq_num(0), in_seq_num(0), retransmission(false), subclass_coded(false),
	subclass(0), meta_type(IAX2_META_UNDEFINED), raw_data(NULL), raw_data_len(0)
{
	ies.clear();
}

iax2_frame::iax2_frame(const unsigned char *buf, size_t buflen) :
	direction(IAX2_DIRECTION_IN), shell(IAX2_FRAME_UNDEFINED), 
	type(IAX2_FRAME_TYPE_UNDEFINED), source_call_num(0), dest_call_num(0),
	timestamp(0), out_seq_num(0), in_seq_num(0), retransmission(false), subclass_coded(false),
	subclass(0), meta_type(IAX2_META_UNDEFINED), raw_data(NULL), raw_data_len(0)
{
	ies.clear();

	unsigned short begin = ntohs(*((unsigned short *) buf));

	if (begin & 0x8000)
		parse_full_frame(buf, buflen);
	else if (begin)
		parse_mini_frame(buf, buflen);
	else
		parse_meta_frame(buf, buflen);
}

iax2_frame::~iax2_frame(void)
{
	iax2_ie *ie;

	while (!ies.empty()) {
		ie = ies.front();
		ies.pop_front();
		free(ie);
	}

	if (raw_data)
		free(raw_data);
}

void iax2_frame::parse_full_frame(const unsigned char *buf, size_t buflen)
{
	iax2_full_header *header = (iax2_full_header *) buf;

	if (buflen < sizeof(*header)) {
		fprintf(stderr, "Invalid full frame!\n");
		return;
	}

	shell = IAX2_FRAME_FULL;
	source_call_num = ntohs(header->scallno) & 0x7FFF;
	retransmission = ntohs(header->dcallno) & 0x8000 ? true : false;
	dest_call_num = ntohs(header->dcallno) & 0x7FFF;
	timestamp = ntohl(header->ts);
	out_seq_num = header->oseqno;
	in_seq_num = header->iseqno;
	type = (enum iax2_frame_type) header->type;
	subclass_coded = header->csub & 0x80 ? true : false;
	subclass = header->csub & 0x7F;

	buf += sizeof(*header);
	buflen -= sizeof(*header);

	set_raw_data(buf, buflen);

	if (type != IAX2_FRAME_TYPE_IAX2)
		return;

	while (buflen) {
		iax2_ie *ie, *tmp = (iax2_ie *) buf;
		if (buflen - 2 < tmp->datalen) {
			fprintf(stderr, "IE datalen '%d' greater than '%d' bytes left in packet!\n",
				(int) tmp->datalen, (int) buflen);
			break;
		}
		if (!(ie = (iax2_ie *) calloc(1, sizeof(*ie) + tmp->datalen)))
			break;
		ie->type = tmp->type;
		ie->datalen = tmp->datalen;
		memcpy(ie->data, tmp->data, tmp->datalen);
		ies.push_back(ie);
		buf += sizeof(*ie) + tmp->datalen;
		buflen -= sizeof(*ie) + tmp->datalen;
		if (buflen && buflen < 2) {
			fprintf(stderr, "Space left in packet (%d) not big enough for an IE!\n", (int) buflen);
			break;
		}
	}
}

void iax2_frame::parse_mini_frame(const unsigned char *buf, size_t buflen)
{
	iax2_mini_header *header = (iax2_mini_header *) buf;

	shell = IAX2_FRAME_MINI;
	
	source_call_num = ntohs(header->callno);
	timestamp = ntohs(header->ts);
	
	set_raw_data(header->data, buflen - sizeof(iax2_mini_header));
}

void iax2_frame::parse_meta_frame(const unsigned char *buf, size_t buflen)
{
	iax2_meta_header *header = (iax2_meta_header *) buf;

	shell = IAX2_FRAME_META;
	if (header->metacmd == 0x80) {
		meta_type = IAX2_META_VIDEO;
		parse_meta_video_frame(buf, buflen);
	} else
		fprintf(stderr, "Unknown meta frame type!\n");
}

void iax2_frame::parse_meta_video_frame(const unsigned char *buf, size_t buflen)
{
	iax2_meta_video_header *header = (iax2_meta_video_header *) buf;
	
	if (buflen <= sizeof(*header)) {
		printf("Invalid meta video frame!\n");
		return;
	}

	source_call_num = ntohs(header->callno) & 0x7FFF;
	timestamp = ntohs(header->ts);
	
	set_raw_data(header->data, buflen - sizeof(iax2_meta_video_header));
}

const char *iax2_frame::type2str(void) const
{
	const char *str;
#define ST(a) case a: str = # a; break;
	switch (type) {
	ST(IAX2_FRAME_TYPE_UNDEFINED) 
	ST(IAX2_FRAME_TYPE_DTMF_END)
	ST(IAX2_FRAME_TYPE_VOICE)
	ST(IAX2_FRAME_TYPE_VIDEO)
	ST(IAX2_FRAME_TYPE_CONTROL)
	ST(IAX2_FRAME_TYPE_NULL)
	ST(IAX2_FRAME_TYPE_IAX2)
	ST(IAX2_FRAME_TYPE_TEXT)
	ST(IAX2_FRAME_TYPE_IMAGE)
	ST(IAX2_FRAME_TYPE_HTML)
	ST(IAX2_FRAME_TYPE_CNG)
	ST(IAX2_FRAME_TYPE_MODEM)
	ST(IAX2_FRAME_TYPE_DTMF_BEGIN)
	default:
		str = "Unknown";
	}
#undef ST
	return str;
}

const char *iax2_frame::meta_type2str(void) const
{
	const char *str;
#define ST(a) case a: str = # a; break;
	switch (meta_type) {
	ST(IAX2_META_VIDEO)
	default:
		str = "Unknown";
	}
#undef ST
	return str;
}

const char *iax2_frame::iax2subclass2str(void) const
{
	const char *str;
#define ST(a) case a: str = # a; break;
	switch (subclass) {
	ST(IAX2_SUBCLASS_NEW)
	ST(IAX2_SUBCLASS_PING)
	ST(IAX2_SUBCLASS_PONG)
	ST(IAX2_SUBCLASS_ACK)
	ST(IAX2_SUBCLASS_HANGUP)
	ST(IAX2_SUBCLASS_REJECT)
	ST(IAX2_SUBCLASS_ACCEPT)
	ST(IAX2_SUBCLASS_AUTHREQ)
	ST(IAX2_SUBCLASS_AUTHREP)
	ST(IAX2_SUBCLASS_INVAL)
	ST(IAX2_SUBCLASS_LAGRQ)
	ST(IAX2_SUBCLASS_LAGRP)
	ST(IAX2_SUBCLASS_REGREQ)
	ST(IAX2_SUBCLASS_REGAUTH)
	ST(IAX2_SUBCLASS_REGACK)
	ST(IAX2_SUBCLASS_REGREJ)
	ST(IAX2_SUBCLASS_REGREL)
	ST(IAX2_SUBCLASS_VNAK)
	ST(IAX2_SUBCLASS_DPREQ)
	ST(IAX2_SUBCLASS_DPREP)
	ST(IAX2_SUBCLASS_DIAL)
	ST(IAX2_SUBCLASS_TXREQ)
	ST(IAX2_SUBCLASS_TXCNT)
	ST(IAX2_SUBCLASS_TXACC)
	ST(IAX2_SUBCLASS_TXREADY)
	ST(IAX2_SUBCLASS_TXREL)
	ST(IAX2_SUBCLASS_TXREJ)
	ST(IAX2_SUBCLASS_QUELCH)
	ST(IAX2_SUBCLASS_UNQUELCH)
	ST(IAX2_SUBCLASS_POKE)
	ST(IAX2_SUBCLASS_MWI)
	ST(IAX2_SUBCLASS_UNSUPPORT)
	ST(IAX2_SUBCLASS_TRANSFER)
	ST(IAX2_SUBCLASS_PROVISION)
	ST(IAX2_SUBCLASS_FWDOWNL)
	ST(IAX2_SUBCLASS_FWDATA)
	default:
		str = NULL;
	}
#undef ST
	return str;
}

const char *iax2_frame::subclass2str(void) const
{
	const char *str;
#define ST(a) str = # a;
	switch (type) {
	case IAX2_FRAME_TYPE_IAX2:
		if ((str = iax2subclass2str()))
			break;
	default:
		str = "Unknown";
	}
#undef ST
	return str;
}

const char *iax2_ie::type2str(void) const
{
	const char *str;
#define ST(a) case a: str = # a; break;
	switch (type) {
	ST(IAX2_IE_CALLED_NUMBER)
	ST(IAX2_IE_CALLING_NUMBER)
	ST(IAX2_IE_CALLING_ANI)
	ST(IAX2_IE_CALLING_NAME)
	ST(IAX2_IE_CALLED_CONTEXT)
	ST(IAX2_IE_USERNAME)
	ST(IAX2_IE_PASSWORD)
	ST(IAX2_IE_CAPABILITY)
	ST(IAX2_IE_FORMAT)
	ST(IAX2_IE_VERSION)
	ST(IAX2_IE_ADSICPE)
	ST(IAX2_IE_DNID)
	ST(IAX2_IE_AUTHMETHODS)
	ST(IAX2_IE_CHALLENGE)
	ST(IAX2_IE_MD5_RESULT)
	ST(IAX2_IE_RSA_RESULT)
	ST(IAX2_IE_APPARENT_ADDR)
	ST(IAX2_IE_REFRESH)
	ST(IAX2_IE_DPSTATUS)
	ST(IAX2_IE_CALLNO)
	ST(IAX2_IE_CAUSE)
	ST(IAX2_IE_IAX2_UNKNOWN)
	ST(IAX2_IE_MSGCOUNT)
	ST(IAX2_IE_AUTOANSWER)
	ST(IAX2_IE_MUSICONHOLD)
	ST(IAX2_IE_TRANSFERID)
	ST(IAX2_IE_RDNIS)
	ST(IAX2_IE_PROVISIONING)
	ST(IAX2_IE_AESPROVISIONING)
	ST(IAX2_IE_DATETIME)
	ST(IAX2_IE_DEVICETYPE)
	ST(IAX2_IE_SERVICEIDENT)
	ST(IAX2_IE_FIRMWAREVER)
	ST(IAX2_IE_FWBLOCKDESC)
	ST(IAX2_IE_FWBLOCKDATA)
	ST(IAX2_IE_PROVVER)
	ST(IAX2_IE_CALLINGPRES)
	ST(IAX2_IE_CALLINGTON)
	ST(IAX2_IE_CALLINGTNS)
	ST(IAX2_IE_SAMPLINGRATE)
	ST(IAX2_IE_CAUSECODE)
	ST(IAX2_IE_ENCRYPTION)
	ST(IAX2_IE_ENCKEY)
	ST(IAX2_IE_CODEC_PREFS)
	ST(IAX2_IE_RR_JITTER)
	ST(IAX2_IE_RR_LOSS)
	ST(IAX2_IE_RR_PKTS)
	ST(IAX2_IE_RR_DELAY)
	ST(IAX2_IE_RR_DROPPED)
	ST(IAX2_IE_RR_OOO)
	ST(IAX2_IE_VARIABLE)
	ST(IAX2_IE_OSPTOKEN)
	default:
		fprintf(stderr, "Unknown IE type '%u'\n", type);
		str = "Unknown";
	}
#undef ST
	return str;
}

void iax2_frame::print_ies(void) const
{
	char *buf = NULL;

	for (iax2_ie_iterator i = ies.begin(); i != ies.end(); i++) {
		switch ((*i)->type) {
		// String Information Elements
		case IAX2_IE_USERNAME:
			if (!buf)
				buf = (char *) alloca(IAX2_IE_MAX_DATALEN + 1);
			memcpy((void *) buf, (void *) (*i)->data, (*i)->datalen);
			buf[(*i)->datalen] = '\0';
			fprintf(stderr, "      IE: Type: %s  Len: %u  Value: %s\n", 
				(*i)->type2str(), (*i)->datalen, buf);
			break;
		// Unsigned Short Information Elements
		case IAX2_IE_VERSION:
		case IAX2_IE_REFRESH:
			fprintf(stderr, "      IE: Type: %s  Len: %u  Value: %hu\n", 
				(*i)->type2str(), (*i)->datalen, 
				(unsigned short) ntohs(*((unsigned short *) (*i)->data)));
			break;
		// Unsigned long IEs
		case IAX2_IE_CAPABILITY:
		case IAX2_IE_FORMAT:
			fprintf(stderr, "      IE: Type: %s  Len: %u  Value: %u\n", 
				(*i)->type2str(), (*i)->datalen, 
				(u_int32_t) ntohl(*((u_int32_t *) (*i)->data)));
			break;
		default:
			fprintf(stderr, "      IE: Type: %s  Len: %u\n", 
				(*i)->type2str(), (*i)->datalen);
		};
	}
}

void iax2_frame::print_full_frame(const struct sockaddr_in *sin) const
{
	fprintf(stderr, "%s-[FULL%s] IP: %s:%hu  Type: %s  Subclass: %s\n"
		"      Source Callnum: %u  Dest Callnum: %u\n"
		"      Out Seqnum: %u  In Seqnum: %u  Timestamp: %u\n",
		direction == IAX2_DIRECTION_IN ? "Rx" : 
			(direction == IAX2_DIRECTION_OUT ? "Tx" : "Unknown"),
		retransmission ? "-Retransmission" : "", 
		inet_ntoa(sin->sin_addr), ntohs(sin->sin_port),
		type2str(), subclass2str(),
		source_call_num, dest_call_num,
		out_seq_num, in_seq_num, timestamp);

	print_ies();
	
	fprintf(stderr, "\n");
}

void iax2_frame::print_mini_frame(const struct sockaddr_in *sin) const
{
	fprintf(stderr, "%s-[MINI] IP: %s:%hu  Dest Callnum: %u  Timestamp: %u  DataLen: %u\n\n",
		direction == IAX2_DIRECTION_IN ? "Rx" : 
			(direction == IAX2_DIRECTION_OUT ? "Tx" : "Unknown"),
		inet_ntoa(sin->sin_addr), ntohs(sin->sin_port),
		dest_call_num, timestamp, get_raw_data_len());
}

void iax2_frame::print_meta_frame(const struct sockaddr_in *sin) const
{
	fprintf(stderr, "%s-[META] IP: %s:%hu  Type: %s  Dest Callnum: %u  Timestamp: %u  DataLen: %u\n\n",
		direction == IAX2_DIRECTION_IN ? "Rx" : 
			(direction == IAX2_DIRECTION_OUT ? "Tx" : "Unknown"),
		inet_ntoa(sin->sin_addr), ntohs(sin->sin_port),
		meta_type2str(), dest_call_num, 
		timestamp, get_raw_data_len());
}

void iax2_frame::print(const struct sockaddr_in *sin)
{
	switch (shell) {
	case IAX2_FRAME_FULL:
		print_full_frame(sin);
		break;
	case IAX2_FRAME_MINI:
		print_mini_frame(sin);
		break;
	case IAX2_FRAME_META:
		print_meta_frame(sin);
		break;
	default:
		fprintf(stderr, "Can not print unknown frame shell '%d'!\n", shell);	
	}
}

size_t iax2_frame::total_ie_len(void) const
{
	size_t len = 0;

	for (iax2_ie_iterator i = ies.begin(); i != ies.end(); i++)
		len += sizeof(**i) + (*i)->datalen;

	return len;
}

int iax2_frame::send_full_frame(const struct sockaddr_in *sin, const int sockfd)
{
	int res = 0;
	unsigned int len;
	struct iax2_full_header *header;

	if (direction != IAX2_DIRECTION_OUT) {
		fprintf(stderr, "Frames must be IAX2_DIRECTION_OUT to be sent!\n");
		return -1;
	}

	len = sizeof(*header) + total_ie_len() + raw_data_len;

	header = (struct iax2_full_header *) alloca(len);

	header->scallno = htons(source_call_num | 0x8000);
	header->dcallno = htons(dest_call_num | ((retransmission ? 1 : 0) << 15));
	header->ts = htonl(timestamp);
	header->oseqno = out_seq_num;
	header->iseqno = in_seq_num;
	header->type = type;
	header->csub = subclass | ((subclass_coded ? 1 : 0) << 7);

	size_t offset = 0;
	for (iax2_ie_iterator i = ies.begin(); i != ies.end(); i++) {
		memcpy(header->iedata + offset, (void *) *i, 
			(size_t) sizeof(**i) + (*i)->datalen);
		offset += sizeof(**i) + (*i)->datalen;
	}

	if (raw_data_len)
		memcpy(header->iedata + offset, raw_data, raw_data_len);

	if (sendto(sockfd, header, len, 0, (const struct sockaddr *) sin, sizeof(*sin)) == -1) {
		fprintf(stderr, "Error Sending IAX2 Full Frame: %s\n", strerror(errno));
		res = -1;
	}

	return res;
}

int iax2_frame::send_meta_frame(const struct sockaddr_in *sin, const int sockfd)
{
	if (direction != IAX2_DIRECTION_OUT) {
		fprintf(stderr, "Frames must be IAX2_DIRECTION_OUT to be sent!\n");
		return -1;
	}

	if (meta_type == IAX2_META_VIDEO)
		return send_meta_video_frame(sin, sockfd);

	fprintf(stderr, "Can't send Unknown meta frame type!\n");
	return -1;
}

int iax2_frame::send_meta_video_frame(const struct sockaddr_in *sin, const int sockfd)
{
	int res = 0;
	unsigned int len;
	struct iax2_meta_video_header *header;

	len = sizeof(*header) + get_raw_data_len();
	header = (iax2_meta_video_header *) alloca(len);

	header->zeros = 0;
	header->callno = htons(dest_call_num | 0x8000);
	unsigned short ts = timestamp;
	header->ts = htons(ts);

	if (get_raw_data_len())
		memcpy(header->data, get_raw_data(), get_raw_data_len());

	if (sendto(sockfd, header, len, 0, (const struct sockaddr *) sin, sizeof(*sin)) == -1) {
		fprintf(stderr, "Error Sending IAX2 Meta Video Frame: %s\n", strerror(errno));
		res = -1;
	}

	return res;
}

int iax2_frame::send_mini_frame(const struct sockaddr_in *sin, const int sockfd)
{
	int res = 0;
	unsigned int len;
	struct iax2_mini_header *header;

	len = sizeof(*header) + get_raw_data_len();
	header = (iax2_mini_header *) alloca(len);

	header->callno = htons(dest_call_num & ~0x8000);
	unsigned short ts = timestamp;
	header->ts = htons(ts);

	if (get_raw_data_len())
		memcpy(header->data, get_raw_data(), get_raw_data_len());
	
	if (sendto(sockfd, header, len, 0, (const struct sockaddr *) sin, sizeof(*sin)) == -1) {
		fprintf(stderr, "Error Sending IAX2 Mini Frame: %s\n", strerror(errno));
		res = -1;
	}

	return res;
}

iax2_frame &iax2_frame::add_ie(enum iax2_ie_type type, const void *data, unsigned char datalen)
{
	iax2_ie *ie;

	if (!(ie = (iax2_ie *) calloc(1, sizeof(*ie) + datalen)))
		return *this;

	ie->type = type;
	ie->datalen = datalen;
	if (data && datalen) {
		memcpy(ie->data, data, (size_t) datalen);
	}

	ies.push_back(ie);

	return *this;
}

iax2_frame &iax2_frame::add_ie_string(enum iax2_ie_type type, const char *str)
{
	return add_ie(type, (void *) str, (unsigned char) strlen(str));
}

iax2_frame &iax2_frame::add_ie_unsigned_short(enum iax2_ie_type type, u_int16_t num)
{
	num = htons(num);

	return add_ie(type, (void *) &num, (unsigned char) sizeof(num));
}

iax2_frame &iax2_frame::add_ie_unsigned_long(enum iax2_ie_type type, u_int32_t num)
{
	num = htonl(num);

	return add_ie(type, (void *) &num, (unsigned char) sizeof(num));
}

iax2_frame &iax2_frame::add_ie_empty(enum iax2_ie_type type)
{
	return add_ie(type, NULL, 0);
}

static int ie_val(const char *type)
{
	if (!strcasecmp(type, "CALLED_NUMBER")) return IAX2_IE_CALLED_NUMBER;
	else if (!strcasecmp(type, "CALLING_NUMBER")) return IAX2_IE_CALLING_NUMBER;
	else if (!strcasecmp(type, "CALLING_ANI")) return IAX2_IE_CALLING_ANI;
	else if (!strcasecmp(type, "CALLING_NAME")) return IAX2_IE_CALLING_NAME;
	else if (!strcasecmp(type, "CALLED_CONTEXT")) return IAX2_IE_CALLED_CONTEXT;
	else if (!strcasecmp(type, "USERNAME")) return IAX2_IE_USERNAME;
	else if (!strcasecmp(type, "PASSWORD")) return IAX2_IE_PASSWORD;
	else if (!strcasecmp(type, "CAPABILITY")) return IAX2_IE_CAPABILITY;
	else if (!strcasecmp(type, "FORMAT")) return IAX2_IE_FORMAT;
	else if (!strcasecmp(type, "LANGUAGE")) return IAX2_IE_LANGUAGE;
	else if (!strcasecmp(type, "VERSION")) return IAX2_IE_VERSION;
	else if (!strcasecmp(type, "ADSICPE")) return IAX2_IE_ADSICPE;
	else if (!strcasecmp(type, "DNID")) return IAX2_IE_DNID;
	else if (!strcasecmp(type, "AUTHMETHODS")) return IAX2_IE_AUTHMETHODS;
	else if (!strcasecmp(type, "CHALLENGE")) return IAX2_IE_CHALLENGE;
	else if (!strcasecmp(type, "MD5_RESULT")) return IAX2_IE_MD5_RESULT;
	else if (!strcasecmp(type, "RSA_RESULT")) return IAX2_IE_RSA_RESULT;
	else if (!strcasecmp(type, "APPARENT_ADDR")) return IAX2_IE_APPARENT_ADDR;
	else if (!strcasecmp(type, "REFRESH")) return IAX2_IE_REFRESH;
	else if (!strcasecmp(type, "DPSTATUS")) return IAX2_IE_DPSTATUS;
	else if (!strcasecmp(type, "CALLNO")) return IAX2_IE_CALLNO;
	else if (!strcasecmp(type, "CAUSE")) return IAX2_IE_CAUSE;
	else if (!strcasecmp(type, "IAX2_UNKNOWN")) return IAX2_IE_IAX2_UNKNOWN;
	else if (!strcasecmp(type, "MSGCOUNT")) return IAX2_IE_MSGCOUNT;
	else if (!strcasecmp(type, "AUTOANSWER")) return IAX2_IE_AUTOANSWER;
	else if (!strcasecmp(type, "MUSICONHOLD")) return IAX2_IE_MUSICONHOLD;
	else if (!strcasecmp(type, "TRANSFERID")) return IAX2_IE_TRANSFERID;
	else if (!strcasecmp(type, "RDNIS")) return IAX2_IE_RDNIS;
	else if (!strcasecmp(type, "PROVISIONING")) return IAX2_IE_PROVISIONING;
	else if (!strcasecmp(type, "AESPROVISIONING")) return IAX2_IE_AESPROVISIONING;
	else if (!strcasecmp(type, "DATETIME")) return IAX2_IE_DATETIME;
	else if (!strcasecmp(type, "DEVICETYPE")) return IAX2_IE_DEVICETYPE;
	else if (!strcasecmp(type, "SERVICEIDENT")) return IAX2_IE_SERVICEIDENT;
	else if (!strcasecmp(type, "FIRMWAREVER")) return IAX2_IE_FIRMWAREVER;
	else if (!strcasecmp(type, "FWBLOCKDESC")) return IAX2_IE_FWBLOCKDESC;
	else if (!strcasecmp(type, "FWBLOCKDATA")) return IAX2_IE_FWBLOCKDATA;
	else if (!strcasecmp(type, "PROVVER")) return IAX2_IE_PROVVER;
	else if (!strcasecmp(type, "CALLINGPRES")) return IAX2_IE_CALLINGPRES;
	else if (!strcasecmp(type, "CALLINGTON")) return IAX2_IE_CALLINGTON;
	else if (!strcasecmp(type, "CALLINGTNS")) return IAX2_IE_CALLINGTNS;
	else if (!strcasecmp(type, "SAMPLINGRATE")) return IAX2_IE_SAMPLINGRATE;
	else if (!strcasecmp(type, "CAUSECODE")) return IAX2_IE_CAUSECODE;
	else if (!strcasecmp(type, "ENCRYPTION")) return IAX2_IE_ENCRYPTION;
	else if (!strcasecmp(type, "ENCKEY")) return IAX2_IE_ENCKEY;
	else if (!strcasecmp(type, "CODEC_PREFS")) return IAX2_IE_CODEC_PREFS;
	else if (!strcasecmp(type, "RR_JITTER")) return IAX2_IE_RR_JITTER;
	else if (!strcasecmp(type, "RR_LOSS")) return IAX2_IE_RR_LOSS;
	else if (!strcasecmp(type, "RR_PKTS")) return IAX2_IE_RR_PKTS;
	else if (!strcasecmp(type, "RR_DELAY")) return IAX2_IE_RR_DELAY;
	else if (!strcasecmp(type, "RR_DROPPED")) return IAX2_IE_RR_DROPPED;
	else if (!strcasecmp(type, "RR_OOO")) return IAX2_IE_RR_OOO;
	else if (!strcasecmp(type, "VARIABLE")) return IAX2_IE_VARIABLE;
	else if (!strcasecmp(type, "OSPTOKEN")) return IAX2_IE_OSPTOKEN;
	else
		return -1;

	return 0;
}

int iax2_frame::add_ie_string(const char *type, const char *val)
{
	int res;

	if ((res = ie_val(type)) == -1)
		return -1;

	add_ie_string((enum iax2_ie_type) res, val);

	return 0;
}

int iax2_frame::add_ie_unsigned_short(const char *type, uint16_t num)
{
	int res;

	if ((res = ie_val(type)) == -1)
		return -1;

	add_ie_unsigned_short((enum iax2_ie_type) res, num);
}

int iax2_frame::add_ie_unsigned_long(const char *type, uint32_t num)
{
	int res;

	if ((res = ie_val(type)) == -1)
		return -1;

	add_ie_unsigned_long((enum iax2_ie_type) res, num);
}

int iax2_frame::add_ie_empty(const char *type)
{
	int res;

	if ((res = ie_val(type)) == -1) {
		fprintf(stderr, "Invalid IE type '%s'\n", type);
		return -1;
	}

	add_ie_empty((enum iax2_ie_type) res);
}

const char *iax2_frame::get_ie_string(enum iax2_ie_type type) const
{
	for (iax2_ie_iterator i = ies.begin(); i != ies.end(); i++) {
		if ((*i)->type == type)
			return (const char *) (*i)->data;
	}

	return NULL;
}

u_int32_t iax2_frame::get_ie_unsigned_long(enum iax2_ie_type type) const
{
	for (iax2_ie_iterator i = ies.begin(); i != ies.end(); i++) {
		if ((*i)->type == type)
			return ntohl(*((u_int32_t *) (*i)->data));
	}

	return 0;
}

int iax2_frame::send(const struct sockaddr_in *sin, const int sockfd)
{
	int res = -1;

	print(sin);

	switch (shell) {
	case IAX2_FRAME_FULL:
		res = send_full_frame(sin, sockfd);
		break;
	case IAX2_FRAME_META:
		res = send_meta_frame(sin, sockfd);
		break;
	case IAX2_FRAME_MINI:
		res = send_mini_frame(sin, sockfd);
		break;
	default:
		fprintf(stderr, "Don't know how to send frame with shell '%d'!\n", shell);
	}

	if (!res)
		retransmission = true;

	return res;
}

iax2_frame &iax2_frame::set_raw_data(const void *data, unsigned int data_len)
{
	if (raw_data) {
		if ((raw_data_len != data_len) 
			&& (!(raw_data = realloc(raw_data, data_len))))
			return *this;
	} else {
		if (!(raw_data = malloc(data_len)))
			return *this;
	}

	raw_data_len = data_len;
	memcpy(raw_data, data, data_len);

	return *this;
}

int iax2_frame::set_shell(const char *val)
{
	if (!strcasecmp(val, "FULL")) {
		set_shell(IAX2_FRAME_FULL);	
	} else if (!strcasecmp(val, "MINI")) {
		set_shell(IAX2_FRAME_MINI);
	} else if (!strcasecmp(val, "META")) {
		set_shell(IAX2_FRAME_META);
	} else {
		return -1;
	}

	return 0;
}

int iax2_frame::set_type(const char *val)
{
	if (!strcasecmp(val, "DTMF_END")) {
		set_type(IAX2_FRAME_TYPE_DTMF_END);
	} else if (!strcasecmp(val, "VOICE")) {
		set_type(IAX2_FRAME_TYPE_VOICE);
	} else if (!strcasecmp(val, "VIDEO")) {
		set_type(IAX2_FRAME_TYPE_VIDEO);
	} else if (!strcasecmp(val, "CONTROL")) {
		set_type(IAX2_FRAME_TYPE_CONTROL);
	} else if (!strcasecmp(val, "NULL")) {
		set_type(IAX2_FRAME_TYPE_NULL);
	} else if (!strcasecmp(val, "IAX2")) {
		set_type(IAX2_FRAME_TYPE_IAX2);
	} else if (!strcasecmp(val, "TEXT")) {
		set_type(IAX2_FRAME_TYPE_TEXT);
	} else if (!strcasecmp(val, "IMAGE")) {
		set_type(IAX2_FRAME_TYPE_IMAGE);
	} else if (!strcasecmp(val, "HTML")) {
		set_type(IAX2_FRAME_TYPE_HTML);
	} else if (!strcasecmp(val, "CNG")) {
		set_type(IAX2_FRAME_TYPE_CNG);
	} else if (!strcasecmp(val, "MODEM")) {
		set_type(IAX2_FRAME_TYPE_MODEM);
	} else if (!strcasecmp(val, "DTMF_BEGIN")) {
		set_type(IAX2_FRAME_TYPE_DTMF_BEGIN);
	} else {
		return -1;
	}

	return 0;
}

int iax2_frame::set_subclass(const char *val)
{
	if (!strcasecmp(val, "NEW"))
		set_subclass(IAX2_SUBCLASS_NEW);
	else if (!strcasecmp(val, "PING"))
		set_subclass(IAX2_SUBCLASS_PING);
	else if (!strcasecmp(val, "PONG"))
		set_subclass(IAX2_SUBCLASS_PONG);
	else if (!strcasecmp(val, "ACK"))
		set_subclass(IAX2_SUBCLASS_ACK);
	else if (!strcasecmp(val, "HANGUP"))
		set_subclass(IAX2_SUBCLASS_HANGUP);
	else if (!strcasecmp(val, "REJECT"))
		set_subclass(IAX2_SUBCLASS_REJECT);
	else if (!strcasecmp(val, "ACCEPT"))
		set_subclass(IAX2_SUBCLASS_ACCEPT);
	else if (!strcasecmp(val, "AUTHREQ"))
		set_subclass(IAX2_SUBCLASS_AUTHREQ);
	else if (!strcasecmp(val, "AUTHREP"))
		set_subclass(IAX2_SUBCLASS_AUTHREP);
	else if (!strcasecmp(val, "INVAL"))
		set_subclass(IAX2_SUBCLASS_INVAL);
	else if (!strcasecmp(val, "LAGRQ"))
		set_subclass(IAX2_SUBCLASS_LAGRQ);
	else if (!strcasecmp(val, "LAGRP"))
		set_subclass(IAX2_SUBCLASS_LAGRP);
	else if (!strcasecmp(val, "REGREQ"))
		set_subclass(IAX2_SUBCLASS_REGREQ);
	else if (!strcasecmp(val, "REGAUTH"))
		set_subclass(IAX2_SUBCLASS_REGAUTH);
	else if (!strcasecmp(val, "REGACK"))
		set_subclass(IAX2_SUBCLASS_REGACK);
	else if (!strcasecmp(val, "REGREL"))
		set_subclass(IAX2_SUBCLASS_REGREL);
	else if (!strcasecmp(val, "VNAK"))
		set_subclass(IAX2_SUBCLASS_VNAK);
	else if (!strcasecmp(val, "DPREQ"))
		set_subclass(IAX2_SUBCLASS_DPREQ);
	else if (!strcasecmp(val, "DPREP"))
		set_subclass(IAX2_SUBCLASS_DPREP);
	else if (!strcasecmp(val, "DIAL"))
		set_subclass(IAX2_SUBCLASS_DIAL);
	else if (!strcasecmp(val, "TXREQ"))
		set_subclass(IAX2_SUBCLASS_TXREQ);
	else if (!strcasecmp(val, "TXCNT"))
		set_subclass(IAX2_SUBCLASS_TXCNT);
	else if (!strcasecmp(val, "TXACC"))
		set_subclass(IAX2_SUBCLASS_TXACC);
	else if (!strcasecmp(val, "TXREADY"))
		set_subclass(IAX2_SUBCLASS_TXREADY);
	else if (!strcasecmp(val, "TXREL"))
		set_subclass(IAX2_SUBCLASS_TXREL);
	else if (!strcasecmp(val, "TXREJ"))
		set_subclass(IAX2_SUBCLASS_TXREJ);
	else if (!strcasecmp(val, "QUELCH"))
		set_subclass(IAX2_SUBCLASS_QUELCH);
	else if (!strcasecmp(val, "UNQUELCH"))
		set_subclass(IAX2_SUBCLASS_UNQUELCH);
	else if (!strcasecmp(val, "POKE"))
		set_subclass(IAX2_SUBCLASS_POKE);
	else if (!strcasecmp(val, "MWI"))
		set_subclass(IAX2_SUBCLASS_MWI);
	else if (!strcasecmp(val, "UNSUPPORT"))
		set_subclass(IAX2_SUBCLASS_UNSUPPORT);
	else if (!strcasecmp(val, "TRANSFER"))
		set_subclass(IAX2_SUBCLASS_TRANSFER);
	else if (!strcasecmp(val, "PROVISION"))
		set_subclass(IAX2_SUBCLASS_PROVISION);
	else if (!strcasecmp(val, "FWDOWNL")) 
		set_subclass(IAX2_SUBCLASS_FWDOWNL);
	else if (!strcasecmp(val, "FWDATA")) 
		set_subclass(IAX2_SUBCLASS_FWDATA);
	else 
		return -1;

	return 0;
}

int iax2_frame::set_meta_type(const char *val)
{
	if (!strcasecmp(val, "VIDEO"))
		set_meta_type(IAX2_META_VIDEO);
	else
		return -1;

	return 0;
}
