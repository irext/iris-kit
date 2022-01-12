/* pb_encode.c -- encode a protobuf using minimal resources
 *
 * 2011 Petteri Aimonen <jpa@kapsi.fi>
 */

#include "pb.h"
#include "pb_encode.h"
#include "pb_common.h"

/* Use the GCC warn_unused_result attribute to check that all return values
 * are propagated correctly. On other compilers and gcc before 3.4.0 just
 * ignore the annotation.
 */
#if !defined(__GNUC__) || ( __GNUC__ < 3) || (__GNUC__ == 3 && __GNUC_MINOR__ < 4)
    #define checkreturn
#else
    #define checkreturn __attribute__((warn_unused_result))
#endif

/**************************************
 * Declarations internal to this file *
 **************************************/
static bool checkreturn buf_write(das_pb_ostream_t *stream, const das_pb_byte_t *buf, size_t count);
static bool checkreturn encode_array(das_pb_ostream_t *stream, das_pb_field_iter_t *field);
static bool checkreturn das_pb_check_proto3_default_value(const das_pb_field_iter_t *field);
static bool checkreturn encode_basic_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn encode_callback_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn encode_field(das_pb_ostream_t *stream, das_pb_field_iter_t *field);
static bool checkreturn encode_extension_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn default_extension_encoder(das_pb_ostream_t *stream, const das_pb_extension_t *extension);
static bool checkreturn das_pb_encode_varint_32(das_pb_ostream_t *stream, uint32_t low, uint32_t high);
static bool checkreturn das_pb_enc_bool(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_varint(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_fixed(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_bytes(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_string(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_submessage(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);
static bool checkreturn das_pb_enc_fixed_length_bytes(das_pb_ostream_t *stream, const das_pb_field_iter_t *field);

#ifdef PB_WITHOUT_64BIT
#define das_pb_int64_t int32_t
#define das_pb_uint64_t uint32_t
#else
#define das_pb_int64_t int64_t
#define das_pb_uint64_t uint64_t
#endif

/*******************************
 * das_pb_ostream_t implementation *
 *******************************/

static bool checkreturn buf_write(das_pb_ostream_t *stream, const das_pb_byte_t *buf, size_t count)
{
    size_t i;
    das_pb_byte_t *dest = (das_pb_byte_t*)stream->state;
    stream->state = dest + count;
    
    for (i = 0; i < count; i++)
        dest[i] = buf[i];
    
    return true;
}

das_pb_ostream_t das_pb_ostream_from_buffer(das_pb_byte_t *buf, size_t bufsize)
{
    das_pb_ostream_t stream;
#ifdef PB_BUFFER_ONLY
    stream.callback = (void*)1; /* Just a marker value */
#else
    stream.callback = &buf_write;
#endif
    stream.state = buf;
    stream.max_size = bufsize;
    stream.bytes_written = 0;
#ifndef PB_NO_ERRMSG
    stream.errmsg = NULL;
#endif
    return stream;
}

bool checkreturn das_pb_write(das_pb_ostream_t *stream, const das_pb_byte_t *buf, size_t count)
{
    if (count > 0 && stream->callback != NULL)
    {
        if (stream->bytes_written + count < stream->bytes_written ||
            stream->bytes_written + count > stream->max_size)
        {
            PB_RETURN_ERROR(stream, "stream full");
        }

#ifdef PB_BUFFER_ONLY
        if (!buf_write(stream, buf, count))
            PB_RETURN_ERROR(stream, "io error");
#else        
        if (!stream->callback(stream, buf, count))
            PB_RETURN_ERROR(stream, "io error");
#endif
    }
    
    stream->bytes_written += count;
    return true;
}

/*************************
 * Encode a single field *
 *************************/

/* Read a bool value without causing undefined behavior even if the value
 * is invalid. See issue #434 and
 * https://stackoverflow.com/questions/27661768/weird-results-for-conditional
 */
static bool safe_read_bool(const void *pSize)
{
    const char *p = (const char *)pSize;
    size_t i;
    for (i = 0; i < sizeof(bool); i++)
    {
        if (p[i] != 0)
            return true;
    }
    return false;
}

/* Encode a static array. Handles the size calculations and possible packing. */
static bool checkreturn encode_array(das_pb_ostream_t *stream, das_pb_field_iter_t *field)
{
    das_pb_size_t i;
    das_pb_size_t count;
#ifndef PB_ENCODE_ARRAYS_UNPACKED
    size_t size;
#endif

    count = *(das_pb_size_t*)field->pSize;

    if (count == 0)
        return true;

    if (PB_ATYPE(field->type) != PB_ATYPE_POINTER && count > field->array_size)
        PB_RETURN_ERROR(stream, "array max size exceeded");
    
#ifndef PB_ENCODE_ARRAYS_UNPACKED
    /* We always pack arrays if the datatype allows it. */
    if (PB_LTYPE(field->type) <= PB_LTYPE_LAST_PACKABLE)
    {
        if (!das_pb_encode_tag(stream, PB_WT_STRING, field->tag))
            return false;
        
        /* Determine the total size of packed array. */
        if (PB_LTYPE(field->type) == PB_LTYPE_FIXED32)
        {
            size = 4 * (size_t)count;
        }
        else if (PB_LTYPE(field->type) == PB_LTYPE_FIXED64)
        {
            size = 8 * (size_t)count;
        }
        else
        { 
            das_pb_ostream_t sizestream = PB_OSTREAM_SIZING;
            void *pData_orig = field->pData;
            for (i = 0; i < count; i++)
            {
                if (!das_pb_enc_varint(&sizestream, field))
                    PB_RETURN_ERROR(stream, PB_GET_ERROR(&sizestream));
                field->pData = (char*)field->pData + field->data_size;
            }
            field->pData = pData_orig;
            size = sizestream.bytes_written;
        }
        
        if (!das_pb_encode_varint(stream, (das_pb_uint64_t)size))
            return false;
        
        if (stream->callback == NULL)
            return das_pb_write(stream, NULL, size); /* Just sizing.. */
        
        /* Write the data */
        for (i = 0; i < count; i++)
        {
            if (PB_LTYPE(field->type) == PB_LTYPE_FIXED32 || PB_LTYPE(field->type) == PB_LTYPE_FIXED64)
            {
                if (!das_pb_enc_fixed(stream, field))
                    return false;
            }
            else
            {
                if (!das_pb_enc_varint(stream, field))
                    return false;
            }

            field->pData = (char*)field->pData + field->data_size;
        }
    }
    else /* Unpacked fields */
#endif
    {
        for (i = 0; i < count; i++)
        {
            /* Normally the data is stored directly in the array entries, but
             * for pointer-type string and bytes fields, the array entries are
             * actually pointers themselves also. So we have to dereference once
             * more to get to the actual data. */
            if (PB_ATYPE(field->type) == PB_ATYPE_POINTER &&
                (PB_LTYPE(field->type) == PB_LTYPE_STRING ||
                 PB_LTYPE(field->type) == PB_LTYPE_BYTES))
            {
                bool status;
                void *pData_orig = field->pData;
                field->pData = *(void* const*)field->pData;

                if (!field->pData)
                {
                    /* Null pointer in array is treated as empty string / bytes */
                    status = das_pb_encode_tag_for_field(stream, field) &&
                             das_pb_encode_varint(stream, 0);
                }
                else
                {
                    status = encode_basic_field(stream, field);
                }

                field->pData = pData_orig;

                if (!status)
                    return false;
            }
            else
            {
                if (!encode_basic_field(stream, field))
                    return false;
            }
            field->pData = (char*)field->pData + field->data_size;
        }
    }
    
    return true;
}

/* In proto3, all fields are optional and are only encoded if their value is "non-zero".
 * This function implements the check for the zero value. */
static bool checkreturn das_pb_check_proto3_default_value(const das_pb_field_iter_t *field)
{
    das_pb_type_t type = field->type;

    if (PB_ATYPE(type) == PB_ATYPE_STATIC)
    {
        if (PB_HTYPE(type) == PB_HTYPE_REQUIRED)
        {
            /* Required proto2 fields inside proto3 submessage, pretty rare case */
            return false;
        }
        else if (PB_HTYPE(type) == PB_HTYPE_REPEATED)
        {
            /* Repeated fields inside proto3 submessage: present if count != 0 */
            return *(const das_pb_size_t*)field->pSize == 0;
        }
        else if (PB_HTYPE(type) == PB_HTYPE_ONEOF)
        {
            /* Oneof fields */
            return *(const das_pb_size_t*)field->pSize == 0;
        }
        else if (PB_HTYPE(type) == PB_HTYPE_OPTIONAL && field->pSize != NULL)
        {
            /* Proto2 optional fields inside proto3 message, or proto3
             * submessage fields. */
            return safe_read_bool(field->pSize) == false;
        }
        else if (field->descriptor->default_value)
        {
            /* Proto3 messages do not have default values, but proto2 messages
             * can contain optional fields without has_fields (generator option 'proto3').
             * In this case they must always be encoded, to make sure that the
             * non-zero default value is overwritten.
             */
            return false;
        }

        /* Rest is proto3 singular fields */
        if (PB_LTYPE(type) <= PB_LTYPE_LAST_PACKABLE)
        {
            /* Simple integer / float fields */
            das_pb_size_t i;
            const char *p = (const char*)field->pData;
            for (i = 0; i < field->data_size; i++)
            {
                if (p[i] != 0)
                {
                    return false;
                }
            }

            return true;
        }
        else if (PB_LTYPE(type) == PB_LTYPE_BYTES)
        {
            const das_pb_bytes_array_t *bytes = (const das_pb_bytes_array_t*)field->pData;
            return bytes->size == 0;
        }
        else if (PB_LTYPE(type) == PB_LTYPE_STRING)
        {
            return *(const char*)field->pData == '\0';
        }
        else if (PB_LTYPE(type) == PB_LTYPE_FIXED_LENGTH_BYTES)
        {
            /* Fixed length bytes is only empty if its length is fixed
             * as 0. Which would be pretty strange, but we can check
             * it anyway. */
            return field->data_size == 0;
        }
        else if (PB_LTYPE_IS_SUBMSG(type))
        {
            /* Check all fields in the submessage to find if any of them
             * are non-zero. The comparison cannot be done byte-per-byte
             * because the C struct may contain padding bytes that must
             * be skipped. Note that usually proto3 submessages have
             * a separate has_field that is checked earlier in this if.
             */
            das_pb_field_iter_t iter;
            if (das_pb_field_iter_begin(&iter, field->submsg_desc, field->pData))
            {
                do
                {
                    if (!das_pb_check_proto3_default_value(&iter))
                    {
                        return false;
                    }
                } while (das_pb_field_iter_next(&iter));
            }
            return true;
        }
    }
    else if (PB_ATYPE(type) == PB_ATYPE_POINTER)
    {
        return field->pData == NULL;
    }
    else if (PB_ATYPE(type) == PB_ATYPE_CALLBACK)
    {
        if (PB_LTYPE(type) == PB_LTYPE_EXTENSION)
        {
            const das_pb_extension_t *extension = *(const das_pb_extension_t* const *)field->pData;
            return extension == NULL;
        }
        else if (field->descriptor->field_callback == das_pb_default_field_callback)
        {
            das_pb_callback_t *pCallback = (das_pb_callback_t*)field->pData;
            return pCallback->funcs.encode == NULL;
        }
        else
        {
            return field->descriptor->field_callback == NULL;
        }
    }

    return false; /* Not typically reached, safe default for weird special cases. */
}

/* Encode a field with static or pointer allocation, i.e. one whose data
 * is available to the encoder directly. */
static bool checkreturn encode_basic_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    if (!field->pData)
    {
        /* Missing pointer field */
        return true;
    }

    if (!das_pb_encode_tag_for_field(stream, field))
        return false;

    switch (PB_LTYPE(field->type))
    {
        case PB_LTYPE_BOOL:
            return das_pb_enc_bool(stream, field);

        case PB_LTYPE_VARINT:
        case PB_LTYPE_UVARINT:
        case PB_LTYPE_SVARINT:
            return das_pb_enc_varint(stream, field);

        case PB_LTYPE_FIXED32:
        case PB_LTYPE_FIXED64:
            return das_pb_enc_fixed(stream, field);

        case PB_LTYPE_BYTES:
            return das_pb_enc_bytes(stream, field);

        case PB_LTYPE_STRING:
            return das_pb_enc_string(stream, field);

        case PB_LTYPE_SUBMESSAGE:
        case PB_LTYPE_SUBMSG_W_CB:
            return das_pb_enc_submessage(stream, field);

        case PB_LTYPE_FIXED_LENGTH_BYTES:
            return das_pb_enc_fixed_length_bytes(stream, field);

        default:
            PB_RETURN_ERROR(stream, "invalid field type");
    }
}

/* Encode a field with callback semantics. This means that a user function is
 * called to provide and encode the actual data. */
static bool checkreturn encode_callback_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    if (field->descriptor->field_callback != NULL)
    {
        if (!field->descriptor->field_callback(NULL, stream, field))
            PB_RETURN_ERROR(stream, "callback error");
    }
    return true;
}

/* Encode a single field of any callback, pointer or static type. */
static bool checkreturn encode_field(das_pb_ostream_t *stream, das_pb_field_iter_t *field)
{
    /* Check field presence */
    if (PB_HTYPE(field->type) == PB_HTYPE_ONEOF)
    {
        if (*(const das_pb_size_t*)field->pSize != field->tag)
        {
            /* Different type oneof field */
            return true;
        }
    }
    else if (PB_HTYPE(field->type) == PB_HTYPE_OPTIONAL)
    {
        if (field->pSize)
        {
            if (safe_read_bool(field->pSize) == false)
            {
                /* Missing optional field */
                return true;
            }
        }
        else if (PB_ATYPE(field->type) == PB_ATYPE_STATIC)
        {
            /* Proto3 singular field */
            if (das_pb_check_proto3_default_value(field))
                return true;
        }
    }

    if (!field->pData)
    {
        if (PB_HTYPE(field->type) == PB_HTYPE_REQUIRED)
            PB_RETURN_ERROR(stream, "missing required field");

        /* Pointer field set to NULL */
        return true;
    }

    /* Then encode field contents */
    if (PB_ATYPE(field->type) == PB_ATYPE_CALLBACK)
    {
        return encode_callback_field(stream, field);
    }
    else if (PB_HTYPE(field->type) == PB_HTYPE_REPEATED)
    {
        return encode_array(stream, field);
    }
    else
    {
        return encode_basic_field(stream, field);
    }
}

/* Default handler for extension fields. Expects to have a das_pb_msgdesc_t
 * pointer in the extension->type->arg field, pointing to a message with
 * only one field in it.  */
static bool checkreturn default_extension_encoder(das_pb_ostream_t *stream, const das_pb_extension_t *extension)
{
    das_pb_field_iter_t iter;

    if (!das_pb_field_iter_begin_extension_const(&iter, extension))
        PB_RETURN_ERROR(stream, "invalid extension");

    return encode_field(stream, &iter);
}


/* Walk through all the registered extensions and give them a chance
 * to encode themselves. */
static bool checkreturn encode_extension_field(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    const das_pb_extension_t *extension = *(const das_pb_extension_t* const *)field->pData;

    while (extension)
    {
        bool status;
        if (extension->type->encode)
            status = extension->type->encode(stream, extension);
        else
            status = default_extension_encoder(stream, extension);

        if (!status)
            return false;
        
        extension = extension->next;
    }
    
    return true;
}

/*********************
 * Encode all fields *
 *********************/

bool checkreturn das_pb_encode(das_pb_ostream_t *stream, const das_pb_msgdesc_t *fields, const void *src_struct)
{
    das_pb_field_iter_t iter;
    if (!das_pb_field_iter_begin_const(&iter, fields, src_struct))
        return true; /* Empty message type */
    
    do {
        if (PB_LTYPE(iter.type) == PB_LTYPE_EXTENSION)
        {
            /* Special case for the extension field placeholder */
            if (!encode_extension_field(stream, &iter))
                return false;
        }
        else
        {
            /* Regular field */
            if (!encode_field(stream, &iter))
                return false;
        }
    } while (das_pb_field_iter_next(&iter));
    
    return true;
}

bool checkreturn das_pb_encode_ex(das_pb_ostream_t *stream, const das_pb_msgdesc_t *fields, const void *src_struct, unsigned int flags)
{
  if ((flags & PB_ENCODE_DELIMITED) != 0)
  {
    return das_pb_encode_submessage(stream, fields, src_struct);
  }
  else if ((flags & PB_ENCODE_NULLTERMINATED) != 0)
  {
    const das_pb_byte_t zero = 0;

    if (!das_pb_encode(stream, fields, src_struct))
        return false;

    return das_pb_write(stream, &zero, 1);
  }
  else
  {
    return das_pb_encode(stream, fields, src_struct);
  }
}

bool das_pb_get_encoded_size(size_t *size, const das_pb_msgdesc_t *fields, const void *src_struct)
{
    das_pb_ostream_t stream = PB_OSTREAM_SIZING;
    
    if (!das_pb_encode(&stream, fields, src_struct))
        return false;
    
    *size = stream.bytes_written;
    return true;
}

/********************
 * Helper functions *
 ********************/

/* This function avoids 64-bit shifts as they are quite slow on many platforms. */
static bool checkreturn das_pb_encode_varint_32(das_pb_ostream_t *stream, uint32_t low, uint32_t high)
{
    size_t i = 0;
    das_pb_byte_t buffer[10];
    das_pb_byte_t byte = (das_pb_byte_t)(low & 0x7F);
    low >>= 7;

    while (i < 4 && (low != 0 || high != 0))
    {
        byte |= 0x80;
        buffer[i++] = byte;
        byte = (das_pb_byte_t)(low & 0x7F);
        low >>= 7;
    }

    if (high)
    {
        byte = (das_pb_byte_t)(byte | ((high & 0x07) << 4));
        high >>= 3;

        while (high)
        {
            byte |= 0x80;
            buffer[i++] = byte;
            byte = (das_pb_byte_t)(high & 0x7F);
            high >>= 7;
        }
    }

    buffer[i++] = byte;

    return das_pb_write(stream, buffer, i);
}

bool checkreturn das_pb_encode_varint(das_pb_ostream_t *stream, das_pb_uint64_t value)
{
    if (value <= 0x7F)
    {
        /* Fast path: single byte */
        das_pb_byte_t byte = (das_pb_byte_t)value;
        return das_pb_write(stream, &byte, 1);
    }
    else
    {
#ifdef PB_WITHOUT_64BIT
        return das_pb_encode_varint_32(stream, value, 0);
#else
        return das_pb_encode_varint_32(stream, (uint32_t)value, (uint32_t)(value >> 32));
#endif
    }
}

bool checkreturn das_pb_encode_svarint(das_pb_ostream_t *stream, das_pb_int64_t value)
{
    das_pb_uint64_t zigzagged;
    if (value < 0)
        zigzagged = ~((das_pb_uint64_t)value << 1);
    else
        zigzagged = (das_pb_uint64_t)value << 1;
    
    return das_pb_encode_varint(stream, zigzagged);
}

bool checkreturn das_pb_encode_fixed32(das_pb_ostream_t *stream, const void *value)
{
    uint32_t val = *(const uint32_t*)value;
    das_pb_byte_t bytes[4];
    bytes[0] = (das_pb_byte_t)(val & 0xFF);
    bytes[1] = (das_pb_byte_t)((val >> 8) & 0xFF);
    bytes[2] = (das_pb_byte_t)((val >> 16) & 0xFF);
    bytes[3] = (das_pb_byte_t)((val >> 24) & 0xFF);
    return das_pb_write(stream, bytes, 4);
}

#ifndef PB_WITHOUT_64BIT
bool checkreturn das_pb_encode_fixed64(das_pb_ostream_t *stream, const void *value)
{
    uint64_t val = *(const uint64_t*)value;
    das_pb_byte_t bytes[8];
    bytes[0] = (das_pb_byte_t)(val & 0xFF);
    bytes[1] = (das_pb_byte_t)((val >> 8) & 0xFF);
    bytes[2] = (das_pb_byte_t)((val >> 16) & 0xFF);
    bytes[3] = (das_pb_byte_t)((val >> 24) & 0xFF);
    bytes[4] = (das_pb_byte_t)((val >> 32) & 0xFF);
    bytes[5] = (das_pb_byte_t)((val >> 40) & 0xFF);
    bytes[6] = (das_pb_byte_t)((val >> 48) & 0xFF);
    bytes[7] = (das_pb_byte_t)((val >> 56) & 0xFF);
    return das_pb_write(stream, bytes, 8);
}
#endif

bool checkreturn das_pb_encode_tag(das_pb_ostream_t *stream, das_pb_wire_type_t wiretype, uint32_t field_number)
{
    das_pb_uint64_t tag = ((das_pb_uint64_t)field_number << 3) | wiretype;
    return das_pb_encode_varint(stream, tag);
}

bool das_pb_encode_tag_for_field ( das_pb_ostream_t* stream, const das_pb_field_iter_t* field )
{
    das_pb_wire_type_t wiretype;
    switch (PB_LTYPE(field->type))
    {
        case PB_LTYPE_BOOL:
        case PB_LTYPE_VARINT:
        case PB_LTYPE_UVARINT:
        case PB_LTYPE_SVARINT:
            wiretype = PB_WT_VARINT;
            break;
        
        case PB_LTYPE_FIXED32:
            wiretype = PB_WT_32BIT;
            break;
        
        case PB_LTYPE_FIXED64:
            wiretype = PB_WT_64BIT;
            break;
        
        case PB_LTYPE_BYTES:
        case PB_LTYPE_STRING:
        case PB_LTYPE_SUBMESSAGE:
        case PB_LTYPE_SUBMSG_W_CB:
        case PB_LTYPE_FIXED_LENGTH_BYTES:
            wiretype = PB_WT_STRING;
            break;
        
        default:
            PB_RETURN_ERROR(stream, "invalid field type");
    }
    
    return das_pb_encode_tag(stream, wiretype, field->tag);
}

bool checkreturn das_pb_encode_string(das_pb_ostream_t *stream, const das_pb_byte_t *buffer, size_t size)
{
    if (!das_pb_encode_varint(stream, (das_pb_uint64_t)size))
        return false;
    
    return das_pb_write(stream, buffer, size);
}

bool checkreturn das_pb_encode_submessage(das_pb_ostream_t *stream, const das_pb_msgdesc_t *fields, const void *src_struct)
{
    /* First calculate the message size using a non-writing substream. */
    das_pb_ostream_t substream = PB_OSTREAM_SIZING;
    size_t size;
    bool status;
    
    if (!das_pb_encode(&substream, fields, src_struct))
    {
#ifndef PB_NO_ERRMSG
        stream->errmsg = substream.errmsg;
#endif
        return false;
    }
    
    size = substream.bytes_written;
    
    if (!das_pb_encode_varint(stream, (das_pb_uint64_t)size))
        return false;
    
    if (stream->callback == NULL)
        return das_pb_write(stream, NULL, size); /* Just sizing */
    
    if (stream->bytes_written + size > stream->max_size)
        PB_RETURN_ERROR(stream, "stream full");
        
    /* Use a substream to verify that a callback doesn't write more than
     * what it did the first time. */
    substream.callback = stream->callback;
    substream.state = stream->state;
    substream.max_size = size;
    substream.bytes_written = 0;
#ifndef PB_NO_ERRMSG
    substream.errmsg = NULL;
#endif
    
    status = das_pb_encode(&substream, fields, src_struct);
    
    stream->bytes_written += substream.bytes_written;
    stream->state = substream.state;
#ifndef PB_NO_ERRMSG
    stream->errmsg = substream.errmsg;
#endif
    
    if (substream.bytes_written != size)
        PB_RETURN_ERROR(stream, "submsg size changed");
    
    return status;
}

/* Field encoders */

static bool checkreturn das_pb_enc_bool(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    uint32_t value = safe_read_bool(field->pData) ? 1 : 0;
    PB_UNUSED(field);
    return das_pb_encode_varint(stream, value);
}

static bool checkreturn das_pb_enc_varint(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    if (PB_LTYPE(field->type) == PB_LTYPE_UVARINT)
    {
        /* Perform unsigned integer extension */
        das_pb_uint64_t value = 0;

        if (field->data_size == sizeof(uint_least8_t))
            value = *(const uint_least8_t*)field->pData;
        else if (field->data_size == sizeof(uint_least16_t))
            value = *(const uint_least16_t*)field->pData;
        else if (field->data_size == sizeof(uint32_t))
            value = *(const uint32_t*)field->pData;
        else if (field->data_size == sizeof(das_pb_uint64_t))
            value = *(const das_pb_uint64_t*)field->pData;
        else
            PB_RETURN_ERROR(stream, "invalid data_size");

        return das_pb_encode_varint(stream, value);
    }
    else
    {
        /* Perform signed integer extension */
        das_pb_int64_t value = 0;

        if (field->data_size == sizeof(int_least8_t))
            value = *(const int_least8_t*)field->pData;
        else if (field->data_size == sizeof(int_least16_t))
            value = *(const int_least16_t*)field->pData;
        else if (field->data_size == sizeof(int32_t))
            value = *(const int32_t*)field->pData;
        else if (field->data_size == sizeof(das_pb_int64_t))
            value = *(const das_pb_int64_t*)field->pData;
        else
            PB_RETURN_ERROR(stream, "invalid data_size");

        if (PB_LTYPE(field->type) == PB_LTYPE_SVARINT)
            return das_pb_encode_svarint(stream, value);
#ifdef PB_WITHOUT_64BIT
        else if (value < 0)
            return das_pb_encode_varint_32(stream, (uint32_t)value, (uint32_t)-1);
#endif
        else
            return das_pb_encode_varint(stream, (das_pb_uint64_t)value);

    }
}

static bool checkreturn das_pb_enc_fixed(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
#ifdef PB_CONVERT_DOUBLE_FLOAT
    if (field->data_size == sizeof(float) && PB_LTYPE(field->type) == PB_LTYPE_FIXED64)
    {
        return das_pb_encode_float_as_double(stream, *(float*)field->pData);
    }
#endif

    if (field->data_size == sizeof(uint32_t))
    {
        return das_pb_encode_fixed32(stream, field->pData);
    }
#ifndef PB_WITHOUT_64BIT
    else if (field->data_size == sizeof(uint64_t))
    {
        return das_pb_encode_fixed64(stream, field->pData);
    }
#endif
    else
    {
        PB_RETURN_ERROR(stream, "invalid data_size");
    }
}

static bool checkreturn das_pb_enc_bytes(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    const das_pb_bytes_array_t *bytes = NULL;

    bytes = (const das_pb_bytes_array_t*)field->pData;
    
    if (bytes == NULL)
    {
        /* Treat null pointer as an empty bytes field */
        return das_pb_encode_string(stream, NULL, 0);
    }
    
    if (PB_ATYPE(field->type) == PB_ATYPE_STATIC &&
        bytes->size > field->data_size - offsetof(das_pb_bytes_array_t, bytes))
    {
        PB_RETURN_ERROR(stream, "bytes size exceeded");
    }
    
    return das_pb_encode_string(stream, bytes->bytes, (size_t)bytes->size);
}

static bool checkreturn das_pb_enc_string(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    size_t size = 0;
    size_t max_size = (size_t)field->data_size;
    const char *str = (const char*)field->pData;
    
    if (PB_ATYPE(field->type) == PB_ATYPE_POINTER)
    {
        max_size = (size_t)-1;
    }
    else
    {
        /* das_pb_dec_string() assumes string fields end with a null
         * terminator when the type isn't PB_ATYPE_POINTER, so we
         * shouldn't allow more than max-1 bytes to be written to
         * allow space for the null terminator.
         */
        if (max_size == 0)
            PB_RETURN_ERROR(stream, "zero-length string");

        max_size -= 1;
    }


    if (str == NULL)
    {
        size = 0; /* Treat null pointer as an empty string */
    }
    else
    {
        const char *p = str;

        /* strnlen() is not always available, so just use a loop */
        while (size < max_size && *p != '\0')
        {
            size++;
            p++;
        }

        if (*p != '\0')
        {
            PB_RETURN_ERROR(stream, "unterminated string");
        }
    }

#ifdef PB_VALIDATE_UTF8
    if (!das_pb_validate_utf8(str))
        PB_RETURN_ERROR(stream, "invalid utf8");
#endif

    return das_pb_encode_string(stream, (const das_pb_byte_t*)str, size);
}

static bool checkreturn das_pb_enc_submessage(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    if (field->submsg_desc == NULL)
        PB_RETURN_ERROR(stream, "invalid field descriptor");

    if (PB_LTYPE(field->type) == PB_LTYPE_SUBMSG_W_CB && field->pSize != NULL)
    {
        /* Message callback is stored right before pSize. */
        das_pb_callback_t *callback = (das_pb_callback_t*)field->pSize - 1;
        if (callback->funcs.encode)
        {
            if (!callback->funcs.encode(stream, field, &callback->arg))
                return false;
        }
    }
    
    return das_pb_encode_submessage(stream, field->submsg_desc, field->pData);
}

static bool checkreturn das_pb_enc_fixed_length_bytes(das_pb_ostream_t *stream, const das_pb_field_iter_t *field)
{
    return das_pb_encode_string(stream, (const das_pb_byte_t*)field->pData, (size_t)field->data_size);
}

#ifdef PB_CONVERT_DOUBLE_FLOAT
bool das_pb_encode_float_as_double(das_pb_ostream_t *stream, float value)
{
    union { float f; uint32_t i; } in;
    uint_least8_t sign;
    int exponent;
    uint64_t mantissa;

    in.f = value;

    /* Decompose input value */
    sign = (uint_least8_t)((in.i >> 31) & 1);
    exponent = (int)((in.i >> 23) & 0xFF) - 127;
    mantissa = in.i & 0x7FFFFF;

    if (exponent == 128)
    {
        /* Special value (NaN etc.) */
        exponent = 1024;
    }
    else if (exponent == -127)
    {
        if (!mantissa)
        {
            /* Zero */
            exponent = -1023;
        }
        else
        {
            /* Denormalized */
            mantissa <<= 1;
            while (!(mantissa & 0x800000))
            {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x7FFFFF;
        }
    }

    /* Combine fields */
    mantissa <<= 29;
    mantissa |= (uint64_t)(exponent + 1023) << 52;
    mantissa |= (uint64_t)sign << 63;

    return das_pb_encode_fixed64(stream, &mantissa);
}
#endif
