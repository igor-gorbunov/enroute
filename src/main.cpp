#include <windows.h>
#include <stdio.h>
#include <string.h>

#undef IN
#include <navigate++>

using namespace libnavigate;

int main()
{
	int result = 0;

	HANDLE hInputFile = INVALID_HANDLE_VALUE, hOutputFile = INVALID_HANDLE_VALUE;
	Navigate_t navigate;
	Message_t msg(MessageType_t::Unknown);
	
	hInputFile = CreateFile("iec.txt", FILE_READ_DATA, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInputFile == INVALID_HANDLE_VALUE)
	{
		printf("enroute: can not open input file.\n");
		result = 1;
		goto _Exit;
	}

	hOutputFile = CreateFile("google-route.html", FILE_WRITE_DATA, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOutputFile == INVALID_HANDLE_VALUE)
	{
		printf("enroute: can not open output file.\n");
		result = 2;
		goto _Exit;
	}

	char *outputPrefix =
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\" />\r\n"
"<style type=\"text/css\">\r\n"
"html { height: 100% } body { height: 100%; margin: 0px; padding: 0px } #map_canvas { height: 100% }\r\n"
"</style>\r\n"
"<script type=\"text/javascript\" src=\"https://maps.google.com/maps/api/js?sensor=false\"></script>\r\n"
"<script type=\"text/javascript\">\r\n"
"function initialize()\r\n"
"{\r\n"
"	var mapCenter = new google.maps.LatLng(59.950, 30.389);\r\n"
"	var myOptions = { zoom: 17, center: mapCenter, mapTypeId: google.maps.MapTypeId.ROADMAP };\r\n"
"	var map = new google.maps.Map(document.getElementById(\"map_canvas\"), myOptions);\r\n"
"	var walkPathCoordinates =\r\n"
"	[\r\n"
"";

	char *outputPostfix =
"	];\r\n"
"	var walkPath = new google.maps.Polyline({ path: walkPathCoordinates, strokeColor: \"#FF0000\", strokeOpacity: 1.0, strokeWeight: 2 });\r\n"
"	walkPath.setMap(map);\r\n"
"}\r\n"
"</script>\r\n"
"</head>\r\n"
"<body onload=\"initialize()\">\r\n"
"<div id=\"map_canvas\" style=\"width: 100%; height: 100%\">\r\n"
"</div>\r\n"
"</body>\r\n"
"</html>\r\n"
"";

	DWORD nmOfBytesToWrite = strlen(outputPrefix);
	DWORD nmOfBytesWritten;

	if (WriteFile(hOutputFile, outputPrefix, nmOfBytesToWrite, &nmOfBytesWritten, NULL) == 0)
	{
		printf("enroute: can not write prefix to output file.\n");
		result = 3;
		goto _Exit;
	}

	bool done = false;
	char buffer[2048];
	BOOL fResult;
	DWORD nmOfBytesToRead = sizeof(buffer);
	DWORD nmOfBytesRead;
	size_t i = 0, bytesCount = 0, maxsize, nmread;

	for ( ; ; )
	{
		nmOfBytesRead = 0;
		fResult = ReadFile(hInputFile, buffer + i, nmOfBytesToRead - i, &nmOfBytesRead, NULL);
		if ((fResult != 0) && (nmOfBytesRead == 0))
		{
			break;
		}
		else if (fResult == 0)
		{
			printf("Error reding input file.\n");
			result = 4;
			goto _Exit;
		}

		bytesCount += nmOfBytesRead;

		maxsize = nmOfBytesRead + i;
		done = false;

		while (!done)
		{
			try
			{
				nmread = 0;
				msg = navigate.ParseMessage(buffer, maxsize, &nmread);
				switch (msg.type())
				{
				case MessageType_t::RMC:
					{
						Rmc_t rmc(msg);
						if (rmc.isPositionValid())
						{
							PositionFix_t fix = rmc.positionFix();
							double latitude = fix.latitude();
							double longitude = fix.longitude();

							char str[1024];
							_snprintf(str, sizeof(str), "new google.maps.LatLng(%f, %f),\r\n", latitude, longitude);
							if (WriteFile(hOutputFile, str, strlen(str), &nmOfBytesWritten, NULL) == 0)
							{
								printf("enroute: can not write prefix to output file.\n");
								result = 5;
								goto _Exit;
							}
						}
					}
					break;
				default:
					break;
				}
				i = maxsize - nmread;
				memmove(buffer, buffer + nmread, i);
				maxsize -= nmread;
			}
			catch (NaviError_t e)
			{
				switch (e)
				{
				case NaviError_t::NoValidMessage:
					done = true;
					break;
				case NaviError_t::InvalidMessage:
					i = maxsize - nmread;
					memmove(buffer, buffer + nmread, i);
					maxsize -= nmread;
					break;
				default:
					i = maxsize - nmread;
					memmove(buffer, buffer + nmread, i);
					maxsize -= nmread;
					break;
				}
			}
		}
	}

	nmOfBytesToWrite = strlen(outputPostfix);
	if (WriteFile(hOutputFile, outputPostfix, nmOfBytesToWrite, &nmOfBytesWritten, NULL) == 0)
	{
		printf("enroute: can not write postfix to output file.\n");
		result = 6;
		goto _Exit;
	}

_Exit:
	if (hInputFile != INVALID_HANDLE_VALUE)
		CloseHandle(hInputFile);
	if (hOutputFile != INVALID_HANDLE_VALUE)
		CloseHandle(hOutputFile);

	return result;
}
