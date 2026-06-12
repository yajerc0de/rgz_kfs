#ifndef KEYGEN_H
#define KEYGEN_H

#include <stdint.h>
#include <stddef.h>

#define KEY_SIZE  16   
#define IV_SIZE    8   


#define KEY_EXT_FIELD_SIZE  16
#define KEY_FILE_SIZE       (KEY_SIZE + IV_SIZE + KEY_EXT_FIELD_SIZE)


void keygen_random(uint8_t key[KEY_SIZE], uint8_t iv[IV_SIZE]);


int keygen_save(const char *path,
                const uint8_t key[KEY_SIZE],
                const uint8_t iv[IV_SIZE],
                const char *original_ext);


int keygen_load(const char *path,
                uint8_t key[KEY_SIZE],
                uint8_t iv[IV_SIZE],
                char *ext_out);

#endif 
