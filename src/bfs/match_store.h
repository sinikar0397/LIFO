#ifndef MATCH_STORE_H
#define MATCH_STORE_H

#include <stddef.h>

#include "../people/people.h"

#define MATCH_MAX_REQUESTS 128

typedef enum MatchRequestStatus {
	MATCH_REQUEST_PENDING,
	MATCH_REQUEST_ACCEPTED,
	MATCH_REQUEST_REJECTED
} MatchRequestStatus;

typedef struct MatchRequest {
	char from_id[MAX_ID_LEN];
	char to_id[MAX_ID_LEN];
	MatchRequestStatus status;
} MatchRequest;

int match_store_send_request(const char from_id[], const char to_id[]);
int match_store_load_incoming(const char user_id[], MatchRequest requests[],
							  int max_requests);
int match_store_accept_request(const char from_id[], const char to_id[]);
int match_store_has_accepted_match(const char user_a[], const char user_b[]);
int match_store_find_accepted_partner(const char user_id[], char partner_id[],
									  size_t partner_id_size);
int match_store_cancel_match(const char user_a[], const char user_b[]);

#endif // MATCH_STORE_H
