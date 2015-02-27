//////////////////////////////////////////////////////////////////////////
//
//	created:	Dec 23 2009
//	file name:	billing.h
//
//////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2008 Elecard.
//	All rights are reserved.  Reproduction in whole or in part is prohibited
//	without the written consent of the copyright owner.
//
//	Elecard reserves the right to make changes without
//	notice at any time. Elecard makes no warranty, expressed,
//	implied or statutory, including but not limited to any implied
//	warranty of merchantability of fitness for any particular purpose,
//	or that the use will not infringe any third party patent, copyright
//	or trademark.
//
//	Elecard must not be liable for any loss or damage arising
//	from its use.
//
//////////////////////////////////////////////////////////////////////////
//
//  Author: Boris Vanin <Boris.Vanin@elecard.ru>
//	purpose:
//
//////////////////////////////////////////////////////////////////////////

#ifndef BILLING_H_INCLUDED
#define BILLING_H_INCLUDED

// Action name definition

typedef struct billing_action_s		billing_action;

typedef enum billing_version_e {
	BILLING_1	= 0x01,
	BILLING_2	= 0x02,
	BILLING_3	= 0x03,
	BILLING_4	= 0x04,
	BILLING_5	= 0x05,
	BILLING_6	= 0x06,
} billing_version;

typedef enum billing_proto_e {
	BILLING_PROTO_HTTP	= 0x01,
	BILLING_PROTO_RTSP	= 0x02,
} billing_proto;

typedef enum billing_event_e {
	BILLING_EVENT_CONNECT		= 0x01,
	BILLING_EVENT_DESCRIBE		= 0x02,
	BILLING_EVENT_DISCONNECT	= 0x03,
	BILLING_EVENT_GET_PARAM		= 0x04,
	BILLING_EVENT_OPTIONS		= 0x05,
	BILLING_EVENT_PAUSE			= 0x06,
	BILLING_EVENT_PLAY			= 0x07,
	BILLING_EVENT_PLAY_BW		= 0x08,
	BILLING_EVENT_PLAY_FW		= 0x09,
	BILLING_EVENT_SEEK			= 0x0A,
	BILLING_EVENT_SETUP			= 0x0B,
	BILLING_EVENT_STOP			= 0x0C,
	BILLING_EVENT_TEARDOWN		= 0x0D,
} billing_event;

struct billing_action_s {
	billing_version	ver;			//

//------------------------------------------------------------------- BILLING_1
	char*			xworks_module;	//Short name of module that calls billing
	billing_proto	proto;			//
	billing_event	event;			//
	char*			session_id;		//Client connection id
	char*			url;			//Requested file
	char*			ip;				//IPv4 - IP address of connected socket. It can be a NAT's address
	char*			agent;			//HTTP_USER_AGENT or Agent from the request's header
	char*			more;			//Some other client's data
	int				duration;		//session duration in milliseconds. valid for "easy" mode and disconnect event

//------------------------------------------------------------------- BILLING_2
	char*			url_ex;				//Full requested URL ("abs_path" see http://www.faqs.org/rfcs/rfc2616.html)
	char*			raw_query;			//Full HTTP/RTSP headers
	char*			Elecard_StbSerial;	//From HTTP/RTSP header
//------------------------------------------------------------------- BILLING_3
	char*			url_ex3;			//absoluteURI see http://www.faqs.org/rfcs/rfc2616.html
//------------------------------------------------------------------- BILLING_4
	//Only for BILLING_PROTO_RTSP
	// This values takes from RTSP:Transport: client_port:client_port1-client_port2
	// 0 - means client doesn't specify this parameters
	unsigned short	client_port1;
	unsigned short	client_port2;
//------------------------------------------------------------------- BILLING_5
	unsigned long long	clientid;		// Unique client id for all protocols (counter from 0)
//------------------------------------------------------------------- BILLING_6
	char*			rtsp_session_id;	// Unique rtsp session
};


#ifdef __cplusplus
extern "C" {
#endif

//Billing API functions
//these functions must be implemented by any valid billing modules

	void*			billing_open_instance		(const char* config_file_path, void* glib_main_loop);

	//
	//  Warning  Return value is ignored for events:
	// 	         BILLING_EVENT_CONNECT
	//           BILLING_EVENT_DISCONNECT
	//
	// return !0 to allow an action
	//         if ( billing_verify_action ) { action is allowed } else { action is denyed }
	int				billing_verify_action		(void* instance, billing_action* action);
	void			billing_close_instance		(void* instance);

#ifdef __cplusplus
}
#endif



#endif // BILLING_H_INCLUDED
