/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020.
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    
    // Case 1: Nothing in the circular buffer currently
    if ((buffer->out_offs == buffer->in_offs) && !buffer->full)
        return NULL;

    // Case 2: out_offs < in_offs
    if(buffer->out_offs < buffer->in_offs){
        int temp_out_offs = buffer->out_offs;
        while(temp_out_offs < buffer->in_offs){
            if(char_offset < buffer->entry[temp_out_offs].size){
                *entry_offset_byte_rtn = char_offset;
                return &buffer->entry[temp_out_offs];
            }
            else{
                char_offset -= buffer->entry[temp_out_offs].size;
                // Increment read position
                temp_out_offs ++;
            }
        }
    }
    // Case 3: out_offs > in_offs
    else{
        int temp_out_offs = buffer->out_offs;
        int temp_in_offs = buffer->in_offs;
        int i;
        while(temp_out_offs < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
            if(char_offset < buffer->entry[temp_out_offs].size){
                *entry_offset_byte_rtn = char_offset;
                return &buffer->entry[temp_out_offs];
            }
            else{
                char_offset -= buffer->entry[temp_out_offs].size;
                // Increment read position
                temp_out_offs ++;
            }
        }
        for(i = 0; i < temp_in_offs; i++){
            if(char_offset < buffer->entry[i].size){
                *entry_offset_byte_rtn = char_offset;
                return &buffer->entry[i];
            }
            else{
                // Decrement the char_offset
                char_offset -= buffer->entry[i].size;
            }
        }
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    const char *buffptr_rtn = NULL;

    // Check if the buffer is full
    if (buffer->full)
    {
        buffptr_rtn = buffer->entry[buffer->out_offs].buffptr;
        // Advance out_offs to overwrite the oldest entry. Use modulo to ensure the operation does not increment 
        // beyond the buffer limit
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Copy the new entry to the buffer at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // Update the full flag
    if(buffer->in_offs == buffer->out_offs)
    {
        buffer->full = true;
    }

    return buffptr_rtn;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
