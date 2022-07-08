#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size) {
    channel_t* chan = (channel_t*)malloc(sizeof(channel_t));
    chan->buffer = buffer_create(size);
    pthread_mutex_init(&chan->mutex, NULL);
    pthread_cond_init(&chan->recv, NULL);
    pthread_cond_init(&chan->send, NULL);
    chan->is_closed = false;
    chan->select = list_create();
    return chan;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data) {
    pthread_mutex_lock(&channel->mutex);
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    else if (!channel->is_closed) {
        while (buffer_add(channel->buffer, data) == BUFFER_ERROR) {
            pthread_cond_wait(&channel->recv, &channel->mutex);
            if (channel->is_closed) {
                pthread_mutex_unlock(&channel->mutex);
                return CLOSED_ERROR;
            }
        }
        pthread_cond_signal(&channel->send);
        if (channel->select)
            list_foreach(channel->select, (void*)sem_post);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    else {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data) {
    pthread_mutex_lock(&channel->mutex);
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    else if (!channel->is_closed) {
        while (buffer_remove(channel->buffer, data) == BUFFER_ERROR) {
            pthread_cond_wait(&channel->send, &channel->mutex);
            if (channel->is_closed) {
                pthread_mutex_unlock(&channel->mutex);
                return CLOSED_ERROR;
            }
        }
        pthread_cond_signal(&channel->recv);
        if (channel->select)
            list_foreach(channel->select, (void*)sem_post);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    else {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data) {
    pthread_mutex_lock(&channel->mutex);
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    else if (!channel->is_closed) {
        enum buffer_status status = buffer_add(channel->buffer, data);
        if (status == BUFFER_ERROR) {
                pthread_mutex_unlock(&channel->mutex);
                return CHANNEL_FULL;
        }
        pthread_cond_signal(&channel->send);
        if (channel->select)
            list_foreach(channel->select, (void*)sem_post);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    else {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data) {
    pthread_mutex_lock(&channel->mutex);
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    else if (!channel->is_closed) {
        enum buffer_status status = buffer_remove(channel->buffer, data);
        if (status == BUFFER_ERROR) {
                pthread_mutex_unlock(&channel->mutex);
                return CHANNEL_EMPTY;
        }
        pthread_cond_signal(&channel->recv);
        if (channel->select)
            list_foreach(channel->select, (void*)sem_post);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    else {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel) {
    pthread_mutex_lock(&channel->mutex);
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    else if (!channel->is_closed) {
        channel->is_closed = true;
        pthread_cond_broadcast(&channel->send);
        pthread_cond_broadcast(&channel->recv);
        if (channel->select)
            list_foreach(channel->select, (void*)sem_post);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    else {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel) {
    //pthread_mutex_lock(&channel->mutex);
    if (!channel->is_closed) {
        //pthread_mutex_unlock(&channel->mutex);
        return DESTROY_ERROR;
    }
    else if (channel->is_closed) {
        buffer_free(channel->buffer);
        pthread_mutex_destroy(&channel->mutex);
        pthread_cond_destroy(&channel->recv);
        pthread_cond_destroy(&channel->send);
        list_destroy(channel->select);
        free(channel);
        return SUCCESS;
    }
    else {
        //pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index) {
    sem_t select;
    sem_init(&select, 0, 0);

    for (int i=0; i<channel_count; i++) {
        pthread_mutex_lock(&channel_list[i].channel->mutex);
        if (channel_list[i].channel->is_closed) {
            pthread_mutex_unlock(&channel_list[i].channel->mutex);
            sem_destroy(&select);
            return CLOSED_ERROR;
        }
        else
            list_insert(channel_list[i].channel->select, &select);
        pthread_mutex_unlock(&channel_list[i].channel->mutex);
    }
    
    while (true) {
        for (int i = 0; i<channel_count; i++) {
            //list_foreach(channel_list[i].channel->select, sem_wait(&select));
            enum channel_status status = GEN_ERROR;
            if (channel_list[i].dir == SEND)
                status = channel_non_blocking_send(channel_list[i].channel, channel_list[i].data);
            else if (channel_list[i].dir == RECV)
                status = channel_non_blocking_receive(channel_list[i].channel, channel_list[i].data);
            if (status==SUCCESS || status==GEN_ERROR || status==DESTROY_ERROR || status==CLOSED_ERROR) {
                for (int j=0; j<channel_count; j++) {
                    pthread_mutex_lock(&channel_list[j].channel->mutex);
                    list_remove(channel_list[j].channel->select, list_find(channel_list[j].channel->select, &select));
                    pthread_mutex_unlock(&channel_list[j].channel->mutex);
                }
                *selected_index = (size_t) i;
                sem_destroy(&select);
                return status;
            }
        }
        sem_wait(&select);
    }
}