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

#ifndef UXR_CLIENT_PROFILE_MULTITHREAD_H_
#define UXR_CLIENT_PROFILE_MULTITHREAD_H_

#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif

#endif // UXR_CLIENT_PROFILE_MULTITHREAD_H_
