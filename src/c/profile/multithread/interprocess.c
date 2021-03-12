#include "multithread_internal.h"
#include "../../core/session/stream/stream_storage_internal.h"

#include <string.h>

#include <uxr/client/core/session/session.h>
//==================================================================
//                             PRIVATE
//==================================================================

#define MAX_DW 3
#define MAX_DR 3
#define HASH_LEN 10

// Limitations:
// Only one dw/dr topic can be save at the same time
// Dw should run session after serializing in one buffer

typedef struct uxr_interprocess_map_t{
    uxrObjectId datawriters[MAX_DW];
    uxrObjectId datareaders[MAX_DR];
    uxrSession* datawriters_sessions[MAX_DW];
    uxrSession* datareaders_sessions[MAX_DR];
    char datawriters_hash[MAX_DW][HASH_LEN];
    char datareaders_hash[MAX_DW][HASH_LEN];
    size_t datawriters_len;
    size_t datareaders_len;
    bool matching_matrix[MAX_DW][MAX_DR];
    ucdrBuffer data_matrix[MAX_DW][MAX_DR];
    uint32_t data_size_matrix[MAX_DW][MAX_DR];
} uxr_interprocess_map_t;

static uxr_interprocess_map_t uxr_interprocess_map = {0};


bool uxr_object_id_compare(uxrSession* session_1, uxrObjectId* object_id_1, uxrSession* session_2, uxrObjectId* object_id_2)
{
    bool ret = true;
    for (uint8_t i = 0; i < sizeof(session_1->info.key); i++)
    {
        ret &= session_1->info.key[i] == session_2->info.key[i];
    }

    ret &= object_id_1->id == object_id_2->id;
    ret &= object_id_2->id == object_id_2->id;
    
    return ret;
}

size_t uxr_get_datawriter_index(uxrSession* session, uxrObjectId* entity_id){
    for (size_t i = 0; i < uxr_interprocess_map.datawriters_len; i++)
    {
        if (uxr_object_id_compare(session, entity_id, uxr_interprocess_map.datawriters_sessions[i], &uxr_interprocess_map.datawriters[i]))
        {
            return i;
        }
    }
}

void UXR_PREPARE_INTERPROCESS(uxrSession* session, uxrObjectId entity_id, ucdrBuffer* ub, uint32_t data_size)
{
    size_t datawriter_index = uxr_get_datawriter_index(session, &entity_id);
    for (size_t i = 0; i < uxr_interprocess_map.datareaders_len; i++)
    {
        if (uxr_interprocess_map.matching_matrix[datawriter_index][i])
        {
            uxr_interprocess_map.data_matrix[datawriter_index][i] = *ub;
            uxr_interprocess_map.data_size_matrix[datawriter_index][i] = data_size;
        }
        
    }
}

void UXR_HANDLE_INTERPROCESS()
{
    for (size_t i = 0; i < uxr_interprocess_map.datawriters_len; i++)
    {
        for (size_t j = 0; j < uxr_interprocess_map.datareaders_len; j++)
        {
            if (uxr_interprocess_map.data_matrix[i][j].init != NULL)
            {
                uxrStreamId stream_id = {0};

                switch (uxr_interprocess_map.datareaders[j].type)
                {
                case UXR_DATAREADER_ID:
                    uxr_interprocess_map.datareaders_sessions[j]->on_topic(
                        uxr_interprocess_map.datareaders_sessions,
                        uxr_interprocess_map.datareaders[j],
                        0, // Req ID = 0 means interprocess?
                        stream_id,
                        &uxr_interprocess_map.data_matrix[i][j],
                        uxr_interprocess_map.data_size_matrix[i][j],
                        uxr_interprocess_map.datareaders_sessions[j]->on_topic_args
                    );                    
                    break;
                default:
                    break;
                }

                uxr_interprocess_map.data_matrix[i][j].init = NULL;
            }
            
        }
    }
}

void uxr_update_interprocess_matching(){
    for (size_t i = 0; i < uxr_interprocess_map.datawriters_len; i++)
    {
        for (size_t j = 0; j < uxr_interprocess_map.datareaders_len; j++)
        {
            uxr_interprocess_map.matching_matrix[i][j] = 0 == memcmp(uxr_interprocess_map.datawriters_hash[i], uxr_interprocess_map.datareaders_hash[j],HASH_LEN);
        }
    }
}

char * hash(const char* data){
    static char demo[HASH_LEN];
    memset(demo, 'A', HASH_LEN);
    return &demo;
}

void UXR_ADD_INTERPROCESS_DATAWRITER(uxrSession* session, uxrObjectId entity_id, const char* xml)
{
    if (uxr_interprocess_map.datawriters_len < MAX_DW - 1)
    {
        uxr_interprocess_map.datawriters[uxr_interprocess_map.datawriters_len] = entity_id;
        uxr_interprocess_map.datawriters_sessions[uxr_interprocess_map.datawriters_len] = session;
        memcpy(&uxr_interprocess_map.datawriters_hash[uxr_interprocess_map.datawriters_len], hash(xml), HASH_LEN);
        uxr_interprocess_map.datawriters_len++;
        uxr_update_interprocess_matching();
    }
}

void UXR_ADD_INTERPROCESS_DATAREADER(uxrSession* session, uxrObjectId entity_id, const char* xml)
{
    if (uxr_interprocess_map.datareaders_len < MAX_DW - 1)
    {
        uxr_interprocess_map.datareaders[uxr_interprocess_map.datareaders_len] = entity_id;
        uxr_interprocess_map.datareaders_sessions[uxr_interprocess_map.datareaders_len] = session;
        memcpy(&uxr_interprocess_map.datareaders_hash[uxr_interprocess_map.datareaders_len], hash(xml), HASH_LEN);
        uxr_interprocess_map.datareaders_len++;
        uxr_update_interprocess_matching();
    }
}
