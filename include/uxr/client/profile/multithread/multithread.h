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

#ifndef UXR_CLIENT_PROFILE_MULTITHREAD_INTERNAL_H_
#define UXR_CLIENT_PROFILE_MULTITHREAD_INTERNAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <uxr/client/config.h>

#include <uxr/client/visibility.h>
#include <uxr/client/core/session/session.h>

#include <uxr/client/config.h>

#ifdef WIN32
#elif defined(PLATFORM_NAME_FREERTOS)
#include <semphr. h>
#elif defined(UCLIENT_PLATFORM_ZEPHYR)
#elif defined(UCLIENT_PLATFORM_POSIX)
#include <pthread.h>
#endif    

typedef struct uxrMutex{
#ifdef WIN32
#elif defined(PLATFORM_NAME_FREERTOS)
    SemaphoreHandle_t impl;
    StaticSemaphore_t xMutexBuffer;
#elif defined(UCLIENT_PLATFORM_ZEPHYR)
    struct k_mutex impl;
#elif defined(UCLIENT_PLATFORM_POSIX)
    pthread_mutex_t impl;
#endif    
} uxrMutex;

// #ifdef UCLIENT_PROFILE_MULTITHREAD
// #define UXR_INIT_LOCK_SESSION uxr_init_session_lock(session)
// #define UXR_LOCK_SESSION uxr_lock_session(session)
// #define UXR_UNLOCK_SESSION uxr_unlock_session(session)
// #else
#define UXR_INIT_LOCK_SESSION uxr_init_lock(&session->mutex)
#define UXR_LOCK_SESSION(session) uxr_lock(&session->mutex)
#define UXR_UNLOCK_SESSION(session) uxr_unlock(&session->mutex)
// #endif

#define UXR_INIT_LOCK(X) uxr_init_lock(X)
// #define UXR_LOCK(X) uxr_lock(X)
// #define UXR_UNLOCK(X) uxr_unlock(X)

#define UXR_LOCK_STREAM_ID(session, stream_id) uxr_lock(uxr_get_stream_mutex_from_id(session, stream_id))
#define UXR_UNLOCK_STREAM_ID(session, stream_id) uxr_unlock(uxr_get_stream_mutex_from_id(session, stream_id))

#define UXR_LOCK_ALL_INPUT_STREAMS(session) \
    for(uint8_t i = 0; i < session->streams.input_best_effort_size; ++i){ uxr_lock(&session->streams.input_best_effort[i].mutex); } \
    for(uint8_t i = 0; i < session->streams.input_reliable_size; ++i){ uxr_lock(&session->streams.input_reliable[i].mutex); } 

#define UXR_UNLOCK_ALL_INPUT_STREAMS(session) \
    for(uint8_t i = 0; i < session->streams.input_best_effort_size; ++i){ uxr_unlock(&session->streams.input_best_effort[i].mutex); } \
    for(uint8_t i = 0; i < session->streams.input_reliable_size; ++i){ uxr_unlock(&session->streams.input_reliable[i].mutex); }   

/**
 * @brief
 * TODO
 */
UXRDLLAPI void uxr_init_lock(uxrMutex* mutex);

/**
 * @brief
 * TODO
 */
UXRDLLAPI void uxr_lock(uxrMutex* mutex);

/**
 * @brief
 * TODO
 */
UXRDLLAPI void uxr_unlock(uxrMutex* mutex);


/**
 * @brief
 * TODO
 */
UXRDLLAPI uxrMutex * uxr_get_stream_mutex_from_id(uxrSession* session, uxrStreamId stream_id);



#ifdef __cplusplus
}
#endif

#endif // UXR_CLIENT_PROFILE_MULTITHREAD_INTERNAL_H_
