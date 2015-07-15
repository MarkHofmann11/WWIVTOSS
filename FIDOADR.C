/*
 * Functions to convert to and from ASCII fidonet specs and their
 * seperate entities.
 *
 * These are the valid FidoNet address specifictions:
 *
 * zone:net/node.point@domain
 * zone:net/node.point
 * zone:net/node@domain
 * zone:net/node
 * net/node.point@domain
 * net/node.point
 * net/node@domain
 * net/node
 * node.point@domain
 * node.point
 * node@domain
 * node
 */

 #include <stdio.h>
 #include <string.h>

#include "fidoadr.h"

/*
 * Split a fido address into it's seperate parts.  The following are valid
 * fido address specifictions.
 */
void fidoadr_split(char *addr, FIDOADR *fadr)
{
    char *p, *q;
    char st[255];

    /*
     * Zone
     */
    p = strchr(addr, ':');
    if (p) fadr->zone = (word) atol(addr);
    else fadr->zone = 0;
    /*
     * Net
     */
    p = strchr(addr, '/');
    if (p) {
        p--;
        while (strchr("0123456789", *p) && (p >= addr)) p--;
        p++;
        fadr->net = (word) atol(p);
    }
    else fadr->net = 0;
    /*
     * Node
     */
    p = strchr(addr, '/');
    if (p) {
        p++;
        fadr->node = (word) atol(p);
    }
    else fadr->node = (word) atol(addr);
    /*
     * Point
     */
    p = strchr(addr, '.');
    if (p) {
        p++;
        fadr->point = (word) atol(p);
    }
    else fadr->point = 0;
    /*
     * Domain
     */
    p = strchr(addr, '@');
    if (p) {
        p++;
        strcpy(fadr->domain, p);
    }
    else *(fadr->domain) = '\0';
}

/*
 * Merge the parts specified in FIDOADR into a ASCII fidonet specification
 *
 * Beware of results if you don't pass good values in FIDOADR.
 *
 */
char *fidoadr_merge(char *addr, FIDOADR *fadr)
{
    static char tmp[64];

    *addr = '\0';
    if (fadr->zone) {
        ltoa((long)(fadr->zone), tmp, 10);
        strcat(addr, tmp);
        strcat(addr, ":");
    }
    if (fadr->zone || fadr->net) {
        ltoa((long) fadr->net, tmp, 10);
        strcat(addr, tmp);
        strcat(addr, "/");
    }
    ltoa((long) fadr->node, tmp, 10);
    strcat(addr, tmp);
    if (fadr->point && fadr->node) {
        strcat(addr, ".");
        ltoa((long) fadr->point, tmp, 10);
        strcat(addr, tmp);
    }
    if (*(fadr->domain)) {
        strcat(addr, "@");
        strcat(addr, fadr->domain);
    }
    return(addr);
}

/*
 * Simple 3-part address to string conversion
 */
char *fidostr(char *dest, word zone, word net, word node)
{
    FIDOADR fadr=DEF_FIDOADR;

    fadr.zone = zone;
    fadr.net = net;
    fadr.node = node;
    fidoadr_merge(dest, &fadr);
    return(dest);
}

/*
 * Split simple 3-part address
 */
void fidosplit(char *src, word *zone, word *net, word *node)
{
    FIDOADR fadr=DEF_FIDOADR;

    fidoadr_split(src, &fadr);
    *zone = fadr.zone;
    *net = fadr.net;
    *node = fadr.node;
}

/*
 * Extract an 8 character hex filename net/node spec to numeric net & node
 *
 * "01180133" -> net=280 node=307
 *
 */
void hexadr_split(char *hexadr, word *net, word *node)
{
    char tmp[5];

    tmp[4] = '\0';
    /*
     * Net
     */
    strncpy(tmp, hexadr, 4);
    strupr(tmp);
    sscanf(tmp, "%04x", net);
    /*
     * Node
     */
    strncpy(tmp, &(hexadr[4]), 4);
    strupr(tmp);
    sscanf(tmp, "%04x", node);
}

/*
 * Merge numeric net & node to an 8 character hex filename
 *
 * net=280 node=307 -> "01180133"
 *
 */
char *hexadr_merge(char *hexadr, word net, word node)
{
    sprintf(hexadr, "%04X%04X", net, node);
    return(hexadr);
}

