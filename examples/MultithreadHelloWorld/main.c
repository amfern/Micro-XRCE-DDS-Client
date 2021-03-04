// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "HelloWorld.h"

#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h> //printf
#include <string.h> //strcmp
#include <stdlib.h> //atoi
#include <pthread.h>
#include <unistd.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

uxrSession session;
uxrStreamId reliable_out;
uxrStreamId reliable_in;
uxrStreamId besteffort_out;
uxrObjectId participant_id;

pthread_mutex_t creation_mutex;

bool create_publisher(uint8_t id)
{   
    printf("Creating pub %d\n", id);
    uxrObjectId publisher_id = uxr_object_id(id, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    uxrObjectId datawriter_id = uxr_object_id(id, UXR_DATAWRITER_ID);
    const char* datawriter_xml = "<dds>"
                                     "<data_writer>"
                                         "<topic>"
                                             "<kind>NO_KEY</kind>"
                                             "<name>HelloWorldTopic</name>"
                                             "<dataType>HelloWorld</dataType>"
                                         "</topic>"
                                     "</data_writer>"
                                 "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);
    
    uint16_t requests[] = {publisher_req, datawriter_req};
    uint8_t status[sizeof(requests)/2];

    if(!uxr_run_session_until_all_status(&session, 1000, requests, status, sizeof(status)))
    {
        printf("Error at create pub: participant: %i topic: %i\n", status[0], status[1]);
        return NULL;
    }
}

void * publish(void * args)
{ 
    uint8_t id = *(uint8_t*) args;
    printf("Running pub %d\n", id);

    uxrObjectId publisher_id = uxr_object_id(id, UXR_PUBLISHER_ID);
    uxrObjectId datawriter_id = uxr_object_id(id, UXR_DATAWRITER_ID);

    // Write topics
    HelloWorld topic;
    topic.index = 0;
    sprintf(topic.message, "Thread %d - topic %d", id, topic.index);
    while(1)
    {
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);

        ucdrBuffer ub;
        uint8_t * buffer = (uint8_t*) malloc(topic_size);
        ucdr_init_buffer(&ub, buffer, topic_size);
        HelloWorld_serialize_topic(&ub, &topic); 
        
        uint16_t ret = uxr_buffer_topic(&session, reliable_out, datawriter_id, buffer, topic_size);

        if (0 == ret)
        {
            printf("ERRRORR\n");
        }
              
        printf("Thread %d - topic %d\n", id, topic.index);

        sprintf(topic.message, "Thread %d - topic %d", id, ++topic.index);

        uxr_run_session_time(&session, 10);

        // uxr_flash_output_streams(&session);

        sleep(1);
    }
}

int main(int args, char** argv)
{
    // CLI
    if(3 > args || 0 == atoi(argv[2]))
    {
        printf("usage: program [-h | --help] | ip port\n");
        return 0;
    }

    char* ip = argv[1];
    char* port = argv[2];

    // Transport
    uxrUDPTransport transport;
    if(!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Error at create transport.\n");
        return 1;
    }

    // Session
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);

    if(!uxr_create_session(&session))
    {
        printf("Error at create session.\n");
        return 1;
    }

    // Streams
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t output_besteffort_stream_buffer[BUFFER_SIZE];
    besteffort_out = uxr_create_output_best_effort_stream(&session, output_besteffort_stream_buffer, BUFFER_SIZE);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    reliable_in = uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Create entities
    participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds>"
                                      "<participant>"
                                          "<rtps>"
                                              "<name>default_xrce_participant</name>"
                                          "</rtps>"
                                      "</participant>"
                                  "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds>"
                                "<topic>"
                                    "<name>HelloWorldTopic</name>"
                                    "<dataType>HelloWorld</dataType>"
                                "</topic>"
                            "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);
    
    uint16_t requests[] = {participant_req, topic_req};
    uint8_t status[sizeof(requests)/2];
    if(!uxr_run_session_until_all_status(&session, 1000, requests, status, sizeof(status)))
    {
        printf("Error at create entities: participant: %i topic: %i\n", status[0], status[1]);
        return 1;
    }

    pthread_t tid[10];
    uint8_t * thread_args = (uint8_t*) malloc(sizeof(tid)/sizeof(pthread_t));

    for (size_t i = 0; i < sizeof(tid)/sizeof(pthread_t); i++)
    {
        create_publisher(i);
        thread_args[i] = i;
    }

    for (size_t i = 0; i < sizeof(tid)/sizeof(pthread_t); i++)
    {
        pthread_create(&tid[i], NULL, publish, (void *) &thread_args[i]);
    }

    for (size_t i = 0; i < sizeof(tid)/sizeof(pthread_t); i++)
    {
        pthread_join(tid[i], NULL);
    }

    // Delete resources
    // uxr_delete_session(&session);
    // uxr_close_udp_transport(&transport);

    return 0;
}
