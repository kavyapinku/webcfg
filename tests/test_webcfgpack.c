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
#include "../src/webcfgdoc.h"
#include "../src/webcfgparam.h"
#include "../src/webcfgpack.h"

void webcfgPackUnpack();
/*
	{
	  "parameters": [
	    {
	      "name": "Device.X_RDK_WebConfig.RootConfig.Data",
	      "value": "<blob>",
	      "datatype": 0
	    }
	  ]
	}
============
	{
	"name": "blob",
	"url": "http:/example.com/<mac>/blob",
	"version": 2274
	}
*/

int readFromFile(char *filename, char **data, int *len)
{
	FILE *fp;
	int ch_count = 0;
	fp = fopen(filename, "r+");
	if (fp == NULL)
	{
		printf("Failed to open file %s\n", filename);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	ch_count = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	*data = (char *) malloc(sizeof(char) * (ch_count + 1));
	fread(*data, 1, ch_count,fp);
        
	*len = ch_count;
	(*data)[ch_count] ='\0';
	fclose(fp);
	return 1;
}

int writeToFile(char *filename, char *data)
{
	FILE *fp;
	fp = fopen(filename , "w+");
	if (fp == NULL)
	{
		printf("Failed to open file %s\n", filename );
		return 0;
	}
	if(data !=NULL)
	{
		fwrite(data, strlen(data), 1, fp);
		fclose(fp);
		return 1;
	}
	else
	{
		printf("WriteToJson failed, Data is NULL\n");
		return 0;
	}
}


void test_webcfgpack_unpack()
{
	subdoc_t *packSubdocData = NULL;
        size_t subdocPackSize = -1;
        void *data = NULL;
        
        packSubdocData = ( subdoc_t * ) malloc( sizeof( subdoc_t ) );
        
        if( packSubdocData != NULL )
        {
            memset(packSubdocData, 0, sizeof(subdoc_t));

            packSubdocData->count = 4;
            packSubdocData->subdoc_items = (struct subdoc *) malloc( sizeof(struct subdoc) * packSubdocData->count );
            memset( packSubdocData->subdoc_items, 0, sizeof(struct subdoc) * packSubdocData->count);

            packSubdocData->subdoc_items[0].name = strdup("blob1");
            packSubdocData->subdoc_items[0].url = strdup("http://example.com/mac:112233445566/blob1");
            packSubdocData->subdoc_items[0].version = 1234; 

            packSubdocData->subdoc_items[1].name = strdup("blob2");
            packSubdocData->subdoc_items[1].url = strdup("http:/example.com/<mac>/blob2");
            packSubdocData->subdoc_items[1].version = 3366;

            packSubdocData->subdoc_items[2].name = strdup("blob3");
            packSubdocData->subdoc_items[2].url = strdup("$host/example.com/<mac>/blob3");
            packSubdocData->subdoc_items[2].version = 1357;

            packSubdocData->subdoc_items[3].name = strdup("blob4");
            packSubdocData->subdoc_items[3].url = strdup("$host/example.com<mac>/blob4");
            packSubdocData->subdoc_items[3].version = 65535;
        }

        printf("webcfg_pack_subdoc\n");
	subdocPackSize = webcfg_pack_subdoc( packSubdocData, &data );
	printf("subdocPackSize is %ld\n", subdocPackSize);
	printf("data packed is %s\n", (char*)data);
        
        webcfgPackUnpack(data);
}

void webcfgPackUnpack(char *blob)
{
   	data_t *packRootData = NULL;
	size_t rootPackSize=-1;
	void *data =NULL;
	int err;
	
	packRootData = ( data_t * ) malloc( sizeof( data_t ) );
	if(packRootData != NULL)
	{
		memset(packRootData, 0, sizeof(data_t));

		packRootData->count = 1;
                packRootData->version = strdup("154363892090392891829182011");
		packRootData->data_items = (struct rootdata *) malloc( sizeof(struct rootdata) * packRootData->count );
		memset( packRootData->data_items, 0, sizeof(struct rootdata) * packRootData->count );

		packRootData->data_items[0].name = strdup("Device.X_RDK_WebConfig.RootConfig.Data");
		packRootData->data_items[0].value = strdup(blob);
		packRootData->data_items[0].type = 2;
	}
	printf("webcfg_pack_rootdoc\n");
	rootPackSize = webcfg_pack_rootdoc( blob, packRootData, &data );
	printf("rootPackSize is %ld\n", rootPackSize);
	printf("data packed is %s\n", (char*)data);

	int status = writeToFile("buff.txt", (char*)data);
	if(status)
	{
		webcfgparam_t *pm;
		int len =0, i =0;
		void* rootbuff;
		char *binfileData = NULL;

		status = readFromFile("buff.txt", &binfileData , &len);
	
		if(status)
		{
			rootbuff = ( void*)binfileData;

			//decode root doc
			printf("--------------decode root doc-------------\n");
			pm = webcfgparam_convert( rootbuff, len+1 );
			err = errno;
			printf( "errno: %s\n", webcfgparam_strerror(err) );
			CU_ASSERT_FATAL( NULL != pm );
			CU_ASSERT_FATAL( NULL != pm->entries );
			CU_ASSERT_FATAL( 1 == pm->entries_count );
                        //CU_ASSERT_STRING_EQUAL("154363892090392891829182011",pm->version);
			CU_ASSERT_STRING_EQUAL( "Device.X_RDK_WebConfig.RootConfig.Data", pm->entries[0].name );
			CU_ASSERT_STRING_EQUAL( blob, pm->entries[0].value );
			CU_ASSERT_FATAL( 2 == pm->entries[0].type );
			for(i = 0; i < (int)pm->entries_count ; i++)
			{
				printf("pm->entries[%d].name %s\n", i, pm->entries[i].name);
				printf("pm->entries[%d].value %s\n" , i, pm->entries[i].value);
				printf("pm->entries[%d].type %d\n", i, pm->entries[i].type);
			}

			//decode inner blob
			webcfgdoc_t *rpm;
			printf("--------------decode blob-------------\n");
			rpm = webcfgdoc_convert( pm->entries[0].value, strlen(pm->entries[0].value) );
			printf("blob len %lu\n", strlen(pm->entries[0].value));
			err = errno;
			printf( "errno: %s\n", webcfgdoc_strerror(err) );
			CU_ASSERT_FATAL( NULL != rpm );
			CU_ASSERT_FATAL( NULL != rpm->entries );
			CU_ASSERT_FATAL( 4 == rpm->entries_count );
			
			//first blob
			CU_ASSERT_STRING_EQUAL( "blob1", rpm->entries[0].name );
			CU_ASSERT_STRING_EQUAL( "http://example.com/mac:112233445566/blob1", rpm->entries[0].url );
			CU_ASSERT_FATAL( 1234 == rpm->entries[0].version );

			//second blob
			CU_ASSERT_STRING_EQUAL( "blob2", rpm->entries[1].name );
			CU_ASSERT_STRING_EQUAL( "http:/example.com/<mac>/blob2", rpm->entries[1].url );
			CU_ASSERT_FATAL( 3366 == rpm->entries[1].version );

			//third blob
			CU_ASSERT_STRING_EQUAL( "blob3", rpm->entries[2].name );
			CU_ASSERT_STRING_EQUAL( "$host/example.com/<mac>/blob3", rpm->entries[2].url );
			CU_ASSERT_FATAL( 1357 == rpm->entries[2].version );

			//fourth blob
			CU_ASSERT_STRING_EQUAL( "blob4", rpm->entries[3].name );
			CU_ASSERT_STRING_EQUAL( "$host/example.com<mac>/blob4", rpm->entries[3].url );
			CU_ASSERT_FATAL( 65535 == rpm->entries[3].version );


			for(i = 0; i < (int)rpm->entries_count ; i++)
			{
				printf("rpm->entries[%d].name %s\n", i, rpm->entries[i].name);
				printf("rpm->entries[%d].url %s\n" , i, rpm->entries[i].url);
				printf("rpm->entries[%d].version %d\n", i, rpm->entries[i].version);
			}

			webcfgdoc_destroy( rpm );
			webcfgparam_destroy( pm );
		}

	}

		
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test_webcfgpack_unpack", test_webcfgpack_unpack);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
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
