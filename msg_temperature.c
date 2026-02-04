#include "msg_temperature.h"
#include <string.h>
#include <stdio.h>

unsigned char *mt_serialize(const struct msg_temperature *m, size_t *len) {
    if (!m || !m->piece) return NULL;

    size_t piece_len = strlen(m->piece);
    *len = 5 + piece_len; // 4 octets (valeur) + 1 octet (type) + n octets (nom)
    
    unsigned char *buf = malloc(*len);
    if (!buf) return NULL;

    // Sérialisation de la valeur (int32_t) en Little-Endian
    uint32_t v = (uint32_t)m->valeur;
    buf[0] = (unsigned char)(v & 0xFF);
    buf[1] = (unsigned char)((v >> 8) & 0xFF);
    buf[2] = (unsigned char)((v >> 16) & 0xFF);
    buf[3] = (unsigned char)((v >> 24) & 0xFF);

    // Type du message
    buf[4] = m->type;

    // Nom de la pièce (pas de \0 final dans le paquet réseau selon le sujet)
    memcpy(buf + 5, m->piece, piece_len);

    return buf;
}

int mt_parse(const unsigned char *buf, size_t len, struct msg_temperature *out) {
    if (!buf || len < 5 || !out) return -1;

    // Reconstruction de la valeur (Little-Endian)
    uint32_t v = 0;
    v |= ((uint32_t)buf[0]);
    v |= ((uint32_t)buf[1]) << 8;
    v |= ((uint32_t)buf[2]) << 16;
    v |= ((uint32_t)buf[3]) << 24;
    out->valeur = (int32_t)v;

    // Type
    out->type = buf[4];

    // Extraction du nom de la pièce
    size_t piece_len = len - 5;
    out->piece = malloc(piece_len + 1);
    if (!out->piece) return -1;
    
    memcpy(out->piece, buf + 5, piece_len);
    out->piece[piece_len] = '\0'; // Ajout du terminateur pour le C

    return 0;
}

void mt_free(struct msg_temperature *m) {
    if (m && m->piece) {
        free(m->piece);
        m->piece = NULL;
    }
}