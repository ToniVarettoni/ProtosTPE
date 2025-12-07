#include "request.h"

static void act_ver(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_VER;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_cmd(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_CMD;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_atyp(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_ATYP;
  ret->n = 1;
  ret->data[0] = c;
}

static struct parser_state_transition ST_VER[] = {
    {ANY, REQUEST_STATE_CMD, act_ver, NULL}
};

static struct parser_state_transition ST_CMD[] = {
    {ANY, REQUEST_STATE_RSV, act_cmd, NULL}
};

static struct parser_state_transition ST_RSV[] = {
    {ANY, REQUEST_STATE_ATYP, NULL, NULL}
};
    
static struct parser_state_transition ST_ATYP[] = {
    {ANY, REQUEST_STATE_DSTADDR, act_atyp, NULL}
};    


    

unsigned read_request(struct selector_key *key) { return 0; }

void dns_lookup(const unsigned state, struct selector_key *key) {}

void try_connect(const unsigned state, struct selector_key *key) {}

unsigned write_request(struct selector_key *key) { return 0; }