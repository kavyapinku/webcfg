 /**
  * Copyright 2019 Comcast Cable Communications Management, LLC
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
 */
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <pthread.h>
#include <time.h>
#include "../src/webcfg_log.h"
#include "../src/webcfg_metadata.h"

/*----------------------------------------------------------------------------*/
/*                             Mock Functions                             */
/*----------------------------------------------------------------------------*/
void webcfgStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}

int writeToFile(char *file_path, char *data, size_t size)
{
	FILE *fp;
	fp = fopen(file_path , "w+");
	if (fp == NULL)
	{
		WebcfgError("Failed to open file in db %s\n", file_path );
		return 0;
	}
	if(data !=NULL)
	{
		fwrite(data, size, 1, fp);
		fclose(fp);
		return 1;
	}
	else
	{
		WebcfgError("Write failed, Data is NULL\n");
		fclose(fp);
		return 0;
	}
}
/*----------------------------------------------------------------------------*/
/*                             Test Functions                             */
/*----------------------------------------------------------------------------*/

void test_initWebcfgProperties()
{
	char buf[512] = {'\0'};
	snprintf(buf,sizeof(buf),"WEBCONFIG_SUPPORTED_DOCS_BIT=16777231,33554435,50331649,67108865,83886081,100663297,117440513,134217729");
	writeToFile(WEBCFG_PROPERTIES_FILE, buf, strlen(buf));
	initWebcfgProperties(WEBCFG_PROPERTIES_FILE);
	snprintf(buf,sizeof(buf),"WEBCONFIG_DOC_SCHEMA_VERSION=16777227-1.1,16777229-1.2");
	writeToFile(WEBCFG_PROPERTIES_FILE, buf, strlen(buf));
	initWebcfgProperties(WEBCFG_PROPERTIES_FILE);
}

void err_initWebcfgProperties()
{
	char command[128] = {'\0'};
	sprintf(command,"rm -rf %s",WEBCFG_PROPERTIES_FILE);
	system(command);
	initWebcfgProperties(WEBCFG_PROPERTIES_FILE);
}

void test_supportedDocs()
{
	char *docs = NULL;
	setsupportedDocs("16777231");
	docs = getsupportedDocs();
	printf("docs: %s\n",docs);
	free(docs);
	setsupportedDocs("16777231,33554435,50331649");
	docs = getsupportedDocs();
	printf("docs: %s\n",docs);
	free(docs);
	setsupportedDocs("");
	docs = getsupportedDocs();
	printf("docs: %s\n",docs);
	free(docs);
	setsupportedDocs(NULL);
	docs = getsupportedDocs();
	printf("docs: %s\n",docs);
}

void test_supportedVersions()
{
	char *versions = NULL;
	setsupportedVersion("50331649-1.1");
	versions = getsupportedVersion();
	printf("versions: %s\n",versions);
	free(versions);
	setsupportedVersion("50331649-1.2,16777231-1.4");
	versions = getsupportedVersion();
	printf("versions: %s\n",versions);
	free(versions);
	setsupportedVersion("");
	versions = getsupportedVersion();
	printf("versions: %s\n",versions);
	free(versions);
	setsupportedVersion(NULL);
	versions = getsupportedVersion();
	printf("versions: %s\n",versions);
	free(versions);
}

void test_isSubDocSupported()
{
	char buf[512] = {'\0'};
	snprintf(buf,sizeof(buf),"WEBCONFIG_SUBDOC_MAP_1=portforwarding:1:true,wan:2:true,lan:3:true,macbinding:4:true,hotspot:5:false,bridge:6:false");
	writeToFile(WEBCFG_PROPERTIES_FILE, buf, strlen(buf));
	initWebcfgProperties(WEBCFG_PROPERTIES_FILE);
	isSubDocSupported("portforwarding");
	isSubDocSupported("wan");
	isSubDocSupported("lan");
	isSubDocSupported("bridge");
	isSubDocSupported("interfacereport");
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test initWebcfgProperties\n", test_initWebcfgProperties);
	CU_add_test( *suite, "Error initWebcfgProperties\n", err_initWebcfgProperties);
	CU_add_test( *suite, "Test Supported docs\n", test_supportedDocs);
	CU_add_test( *suite, "Test Supported versions\n", test_supportedVersions);
	CU_add_test( *suite, "Test isSubDocSupported\n", test_isSubDocSupported);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main()
{
	unsigned rv = 1;
	CU_pSuite suite = NULL;
	if( CUE_SUCCESS == CU_initialize_registry() ) {
	add_suites( &suite );

	if( NULL != suite ) {
	    CU_basic_set_mode( CU_BRM_VERBOSE );
	    CU_basic_run_tests();
	    printf( "\n" );
	    CU_basic_show_failures( CU_get_failure_list() );
	    printf( "\n\n" );
	    rv = CU_get_number_of_tests_failed();
	}

	CU_cleanup_registry();

	}
	return rv;
}
