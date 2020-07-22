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
          supported_bits = strdup(token);
          printf("supported_bits = %s\n", supported_bits);
          
        }

	supported_version = strdup(token);
        printf("supported_version = %s\n", supported_version);
        
        
        if(token!=NULL)
        {
           token = strtok(NULL, "="); 
        }
     } 
     supported_bits = strtok(supported_bits,"\n");
     supported_version = strtok(supported_version,"\n");
     printf("supported_bits = %s\n", supported_bits);

     WEBCFG_FREE(data);     
}

char * getsupportedDocs()
{
    char * token = strtok(supported_bits,"|");
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
	char bitmap[32] = {'\0'};
	char group[8] = {'\0'};
	char subDoc[24] = {'\0'};
	if(supported_bits != NULL)
	{
		tmpStr = strdup(supported_bits);
		while(tmpStr != NULL)
		{
			numStr = strsep(&tmpStr, "|");
			webcfgStrncpy(bitmap, numStr, sizeof(bitmap)+1);
			webcfgStrncpy(group, numStr, sizeof(group)+1);
			webcfgStrncpy(subDoc, numStr+sizeof(group),sizeof(subDoc)+1);
			if(strcmp(group,groupId) == 0)
			{
				WebcfgDebug("bitmap: %s group: %s subDoc: %s\n",bitmap, group,subDoc);
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
