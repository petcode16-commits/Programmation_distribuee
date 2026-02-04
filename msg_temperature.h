#ifndef MSG_TEMPERATURE_H
#define MSG_TEMPERATURE_H

#include <stdint.h>
#include <stdlib.h>

/* Définition des types de messages conformément au sujet */
#define MT_MESURE 0
#define MT_CHAUFFER 1

struct msg_temperature {
    int32_t valeur; /* 4 octets - Température ou Puissance */
    uint8_t type;   /* 1 octet  - MESURE ou CHAUFFER */
    char *piece;    /* Chaîne de caractères pour le nom de la pièce */
};

/**
 * Alloue un tampon et y sérialise la structure (format binaire).
 * L'appelant est responsable de libérer (free) le tampon retourné.
 */
unsigned char *mt_serialize(const struct msg_temperature *m, size_t *len);

/**
 * Analyse un tampon binaire pour remplir une structure msg_temperature.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int mt_parse(const unsigned char *buf, size_t len, struct msg_temperature *out);

/**
 * Libère la mémoire interne de la structure (le champ piece).
 */
void mt_free(struct msg_temperature *m);

#endif /* MSG_TEMPERATURE_H */