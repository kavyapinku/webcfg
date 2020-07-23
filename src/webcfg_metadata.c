/*
 * Copyright 2020 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "webcfg_log.h"
#include "webcfg_metadata.h"
#include "webcfg_multipart.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
char *bitMap[MAX_GROUP_SIZE][MAX_SUBDOCS_SIZE] = {{"00000001","portforwarding","wan", "lan","macbinding","hotspot","bridge"},{"00000010","privatessid","homessid","radio"},{"00000011","moca"},{"00000100","xdns"},{"00000101","advsecurity"},{"00000110","mesh"},{"00000111","aker"},{"00001000","telemetry"},{"00001001","trafficreport","statusreport"},{"00001010","radioreport","interfacereport"}};
static char * supported_bits = NULL;
static char * supported_version = NULL;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
int getSubdocGroupId(char *subDoc, char **groupId);
void getSubdDocBitForGroupId(char *groupId, char **subDocBit);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/


void initWebcfgProperties(char * filename)
{
     FILE *fp = NULL;
     size_t sz = 0;
     char *data = NULL;
     size_t len = 0;
     int ch_count=0;
     char * supported_bits_temp = NULL;
     char * supported_version_temp = NULL;

     WebcfgDebug("webcfg properties file path is %s\n", filename);
     fp = fopen(filename,"rb");

     if (fp == NULL)
     {
	WebcfgError("Failed to open file %s\n", filename);
     }
     
     fseek(fp, 0, SEEK_END);
     ch_count = ftell(fp);
     if (ch_count == (int)-1)
     {
         WebcfgError("fread failed.\n");
	 fclose(fp);
     }
     fseek(fp, 0, SEEK_SET);
     data = (char *) malloc(sizeof(char) * (ch_count + 1));
     if(NULL == data)
     {
         WebcfgError("Memory allocation for data failed.\n");
         fclose(fp);
     }
     memset(data,0,(ch_count + 1));
     sz = fread(data, 1, ch_count,fp);
     if (sz == (size_t)-1) 
     {	
	fclose(fp);
	WebcfgError("fread failed.\n");
	WEBCFG_FREE(data);
     }
     len = ch_count;
     fclose(fp);

     char * ptr_count = data;
     int flag = 1;

     while(flag == 1)
     {
        ptr_count = memchr(ptr_count, '#', len - (ptr_count - data));
        if(ptr_count == NULL)
        {
            break;
        }
        if(0 == memcmp(ptr_count, "#", 1))
        {
              ptr_count = memchr(ptr_count, '\n', len - (ptr_count - data));
        }
        ptr_count++;
        if(0 != memcmp(ptr_count, "#", 1))
        {
              flag = 0;
              ptr_count++;
        }
     }
     char* token = strtok(ptr_count, "=");
     while (token != NULL)
     { 
	printf("Initial %s\n", token);
      
        if(strcmp(token,"WEBCONFIG_SUPPORTED_DOCS_BIT") == 0)
        {
          token = strtok(NULL, "=");
          supported_bits_temp = strdup(token);
          WebcfgDebug("supported_bits = %s\n", supported_bits_temp);
          token = strtok(NULL, "=");
        }

        if(token!=NULL)
        {
	   supported_version_temp = strdup(token);
           WebcfgDebug("supported_version = %s\n", supported_version_temp);
        
        }
        
        if(token!=NULL)
        {
           token = strtok(NULL, "="); 
        }
     } 
     supported_bits_temp = strtok(supported_bits_temp,"\n");
	 supported_bits = strdup(supported_bits_temp);
	 
     supported_version = strtok(supported_version_temp,"\n");
     WebcfgDebug("supported_bits = %s\n", supported_bits);

     WEBCFG_FREE(supported_bits_temp);
     WEBCFG_FREE(supported_version_temp);
     WEBCFG_FREE(data);     
}

char * getsupportedDocs()
{
    char * token_temp = strdup(supported_bits);
    char * token = strtok(token_temp,"|");
    long long number = 0;
    char * endptr = NULL;
    char * finalvalue = NULL , *tempvalue = NULL;
    finalvalue = malloc(180);
    memset(finalvalue,0,180);
    tempvalue = malloc(20);
    while(token != NULL)
    {
        memset(tempvalue,0,20);
        number = strtoll(token,&endptr,2);
        sprintf(tempvalue,"%lld", number);
        token = strtok(NULL, "|");
        strcat(finalvalue, tempvalue);
        if(token!=NULL)
        {
            strcat(finalvalue, ",");
        }
    }
    WEBCFG_FREE(tempvalue);
    WEBCFG_FREE(token_temp);
    return finalvalue;
}

char * getsupportedVersion()
{
      return supported_version;
}

WEBCFG_STATUS isSubDocSupported(char *subDoc)
{
	int pos = 0;
	char *groupId = NULL, *subDocBit = NULL;
	long long docBit = 0;
	pos = getSubdocGroupId(subDoc, &groupId);
	WebcfgDebug("%s is at %d position in %s group\n",subDoc,pos, groupId);
	getSubdDocBitForGroupId(groupId, &subDocBit);
	if(subDocBit != NULL)
	{
		WebcfgDebug("subDocBit: %s\n",subDocBit);
		sscanf(subDocBit,"%lld",&docBit);
		WebcfgDebug("docBit: %lld\n",docBit);
	
		if(docBit & (1 << (pos -1)))
		{
			WebcfgInfo("%s is supported\n",subDoc);
			WEBCFG_FREE(groupId);
			WEBCFG_FREE(subDocBit);
			return WEBCFG_SUCCESS;
		}
	}
	else
	{
		WebcfgError("Supported doc bit not found for %s\n",subDoc);
	}
	WEBCFG_FREE(groupId);
	if(subDocBit != NULL)
	{
		WEBCFG_FREE(subDocBit);
	}
	WebcfgInfo("%s is not supported\n",subDoc);
	return WEBCFG_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
int getSubdocGroupId(char *subDoc, char **groupId)
{
	int position = 0, i = 0, j = 0;
	WebcfgDebug("subDoc: %s\n",subDoc);
	for(i=0; i<MAX_GROUP_SIZE; i++)
	{
		for(j=0;j<MAX_SUBDOCS_SIZE; j++)
		{
			if(bitMap[i][j] == NULL)
			{
				break;
			}
			if(strcmp(bitMap[i][j],subDoc) == 0)
			{
				WebcfgDebug("bitMap[%d][%d]: %s\n",i, j, bitMap[i][j]);
				*groupId = strdup(bitMap[i][0]);
				position = j;
				return position;
			}
		}
	}
	return -1;
}

void getSubdDocBitForGroupId(char *groupId, char **subDocBit)
{
	char *tmpStr=  NULL, *numStr = NULL;
	char subDoc[24] = {'\0'};
	WebcfgDebug("supported_bits: %s\n",supported_bits);
	if(supported_bits != NULL)
	{
		tmpStr = strdup(supported_bits);
		while(tmpStr != NULL)
		{
			numStr = strsep(&tmpStr, "|");
			WebcfgDebug("numStr: %s\n",numStr);
			WebcfgDebug("groupId: %s\n",groupId);
			if(strncmp(groupId, numStr, 8) == 0)
			{
				webcfgStrncpy(subDoc, numStr+8,sizeof(subDoc)+1);
				WebcfgDebug("subDoc: %s\n",subDoc);
				*subDocBit= strdup(subDoc);
				break;
			}
		}
	}
	else
	{
		WebcfgError("Failed to read supported bits from webconfig.properties\n");
	}
}
