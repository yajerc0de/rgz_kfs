#include "keygen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void keygen_random(uint8_t key[KEY_SIZE], uint8_t iv[IV_SIZE])
{
    int i;
    srand((unsigned int)time(NULL));

    for (i = 0; i < KEY_SIZE; i++)
        key[i] = (uint8_t)(rand() % 256);

    for (i = 0; i < IV_SIZE; i++)
        iv[i] = (uint8_t)(rand() % 256);
}

int keygen_save(const char *path,
                const uint8_t key[KEY_SIZE],
                const uint8_t iv[IV_SIZE],
                const char *original_ext)
{
    FILE *f;
    char ext_buf[KEY_EXT_FIELD_SIZE];

    f = fopen(path, "wb");
    if (!f) {
        perror("keygen_save: fopen");
        return -1;
    }

    fwrite(key, 1, KEY_SIZE, f);
    fwrite(iv,  1, IV_SIZE,  f);

    /* Save extension field (padded with zeros) */
    memset(ext_buf, 0, sizeof(ext_buf));
    if (original_ext) {
        strncpy(ext_buf, original_ext, KEY_EXT_FIELD_SIZE - 1);
    }
    fwrite(ext_buf, 1, KEY_EXT_FIELD_SIZE, f);

    fclose(f);
    return 0;
}

int keygen_load(const char *path,
                uint8_t key[KEY_SIZE],
                uint8_t iv[IV_SIZE],
                char *ext_out)
{
    FILE *f;
    char ext_buf[KEY_EXT_FIELD_SIZE];

    f = fopen(path, "rb");
    if (!f) {
        perror("keygen_load: fopen");
        return -1;
    }

    if (fread(key, 1, KEY_SIZE, f) != KEY_SIZE) {
        fprintf(stderr, "keygen_load: failed to read key\n");
        fclose(f);
        return -1;
    }
    if (fread(iv, 1, IV_SIZE, f) != IV_SIZE) {
        fprintf(stderr, "keygen_load: failed to read IV\n");
        fclose(f);
        return -1;
    }
    if (fread(ext_buf, 1, KEY_EXT_FIELD_SIZE, f) != KEY_EXT_FIELD_SIZE) {
        fprintf(stderr, "keygen_load: failed to read extension\n");
        fclose(f);
        return -1;
    }

    fclose(f);

    ext_buf[KEY_EXT_FIELD_SIZE - 1] = '\0';
    if (ext_out)
        memcpy(ext_out, ext_buf, KEY_EXT_FIELD_SIZE);

    return 0;
}
