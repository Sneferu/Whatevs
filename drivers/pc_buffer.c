#include "pc_buffer.h"

//*****************************************************************************
// Initializes the producer-consumer buffer structure.  
//*****************************************************************************
void pc_buffer_init(PC_Buffer *buffer, uint16_t buffer_size)
{
	buffer->produce_count = 0;
	buffer->consume_count = 0;
	buffer->BUFFER_SIZE = buffer_size;
	buffer->array = (char*)malloc(buffer_size);
}

//*****************************************************************************
// Adds a character to the producer consumer buffer
//*****************************************************************************
void pc_buffer_add(PC_Buffer *buffer, char data)
{
	buffer->array[(buffer->produce_count)%(buffer->BUFFER_SIZE)] = data;
	(buffer->produce_count)++;
}

//*****************************************************************************
// Removes a character from the producer consumer buffer.
//*****************************************************************************
void pc_buffer_remove(PC_Buffer *buffer, char *data)
{
	*data = buffer->array[(buffer->consume_count)%(buffer->BUFFER_SIZE)];
	(buffer->consume_count)++;
}

//*****************************************************************************
// Returns if the producer consumer buffer is empty
//*****************************************************************************
bool pc_buffer_empty(PC_Buffer *buffer)
{
	return ((buffer->produce_count)==(buffer->consume_count));
}

//*****************************************************************************
// Returns if the producer consumer buffer is full
//*****************************************************************************
bool pc_buffer_full(PC_Buffer *buffer)
{
	return ((buffer->produce_count - buffer->consume_count) == buffer->BUFFER_SIZE);
}
