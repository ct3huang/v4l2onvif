/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** server.cpp
** 
** server
**
** -------------------------------------------------------------------------*/

#include <net/if.h>
#include <ifaddrs.h>

#include "DeviceBinding.nsmap"
#include "soapDeviceBindingService.h"
#include "soapMediaBindingService.h"
#include "soapRecordingBindingService.h"
#include "soapReceiverBindingService.h"
#include "soapReplayBindingService.h"
#include "soapEventBindingService.h"
#include "soapPullPointSubscriptionBindingService.h"

#include "wsseapi.h"

#include "serviceContext.h"

int http_get(struct soap *soap)
{
	int retCode = 404;
	ServiceContext* ctx = (ServiceContext*)soap->user;
	FILE *fd = fopen(ctx->m_wsdlurl.c_str(), "rb"); 
	if (fd != NULL)
	{
		soap->http_content = "text/xml"; 
		soap_response(soap, SOAP_FILE);
		for (;;)
		{
			size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
			if (!r)
				break;
			if (soap_send_raw(soap, soap->tmpbuf, r))
				break; // can't send, but little we can do about that
		}
		fclose(fd);
		soap_end_send(soap);
		retCode = SOAP_OK;
	}
	return retCode;
} 

std::string getServerIpFromClientIp(int clientip)
{
	std::string serverip;
	char host[NI_MAXHOST];
	struct ifaddrs *ifaddr = NULL;
	if (getifaddrs(&ifaddr) == 0) 
	{
		for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
		{
			if (ifa->ifa_addr != NULL)			
			{
				int family = ifa->ifa_addr->sa_family;
				struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
				struct sockaddr_in* mask = (struct sockaddr_in*)ifa->ifa_netmask;
				if (mask != NULL)
				{
					if ( (family == AF_INET) && ( (addr->sin_addr.s_addr & mask->sin_addr.s_addr) == (clientip & mask->sin_addr.s_addr)) )
					{
						if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0)
						{
							serverip = host;
							std::cout << "serverip:" << host << std::endl;
							break;
						}
					}
				}
			}
		}
	}
	freeifaddrs(ifaddr);
	return serverip;
}
	
int main(int argc, char* argv[])
{		
	std::string host("0.0.0.0");
	int port = 8080;
	
	struct soap *soap = soap_new();
	ServiceContext deviceCtx("devicemgmt.wsdl", port);
	soap->user = (void*)&deviceCtx;
	soap->fget = http_get; 
	{	
		MediaBindingService mediaService(soap);
		RecordingBindingService recordingService(soap);
		ReceiverBindingService receiverService(soap);
		ReplayBindingService replayService(soap);
		EventBindingService eventService(soap);
		PullPointSubscriptionBindingService pullPointService(soap);

		DeviceBindingService deviceService(soap);
		soap_register_plugin(deviceService.soap, soap_wsse);		
		
		if (!soap_valid_socket(soap_bind(soap, NULL, deviceCtx.m_port, 100))) 
		{
			soap_stream_fault(soap, std::cerr);
		}
		else
		{
			while (soap_accept(soap) != SOAP_OK) 
			{
				if (soap_begin_serve(soap))
				{
					soap_stream_fault(soap, std::cerr);
				}
				else if (deviceService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (recordingService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (receiverService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (replayService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (eventService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (pullPointService.dispatch() != SOAP_NO_METHOD)
				{
				}
				else if (mediaService.dispatch() != SOAP_NO_METHOD)
				{
					soap_stream_fault(soap, std::cerr);
				}
			}
		}
	}
	
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap); 			
	
	return 0;
}
