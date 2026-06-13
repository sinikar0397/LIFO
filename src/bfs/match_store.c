#include "match_store.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static const char *MATCH_REQUEST_PATH = "database/matches/requests.json";

static void match_copy_text(char *dst, size_t dst_size, const char *src) {
	if (dst_size == 0) {
		return;
	}
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}
	strncpy(dst, src, dst_size - 1);
	dst[dst_size - 1] = '\0';
}

static void match_ensure_dirs(void) {
#ifdef _WIN32
	_mkdir("database");
	_mkdir("database/matches");
#else
	mkdir("database", 0755);
	mkdir("database/matches", 0755);
#endif
}

static int match_load_all(MatchRequest requests[], int max_requests) {
	match_ensure_dirs();

	FILE *f = fopen(MATCH_REQUEST_PATH, "r");
	if (f == NULL) {
		return 0;
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size <= 0) {
		fclose(f);
		return 0;
	}

	char *buf = malloc((size_t)size + 1);
	if (buf == NULL) {
		fclose(f);
		return 0;
	}
	size_t rd = fread(buf, 1, (size_t)size, f);
	buf[rd] = '\0';
	fclose(f);

	cJSON *root = cJSON_Parse(buf);
	free(buf);
	if (root == NULL) {
		return 0;
	}

	int count = 0;
	if (cJSON_IsArray(root)) {
		cJSON *item = NULL;
		cJSON_ArrayForEach(item, root) {
			if (count >= max_requests) {
				break;
			}
			cJSON *from = cJSON_GetObjectItem(item, "from_id");
			cJSON *to = cJSON_GetObjectItem(item, "to_id");
			cJSON *status = cJSON_GetObjectItem(item, "status");
			if (cJSON_IsString(from) && cJSON_IsString(to)) {
				match_copy_text(requests[count].from_id,
								sizeof(requests[count].from_id),
								from->valuestring);
				match_copy_text(requests[count].to_id,
								sizeof(requests[count].to_id),
								to->valuestring);
				requests[count].status =
					cJSON_IsNumber(status) ? status->valueint
										   : MATCH_REQUEST_PENDING;
				count++;
			}
		}
	}

	cJSON_Delete(root);
	return count;
}

static int match_save_all(const MatchRequest requests[], int count) {
	match_ensure_dirs();

	cJSON *root = cJSON_CreateArray();
	if (root == NULL) {
		return 0;
	}
	for (int i = 0; i < count; i++) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject(item, "from_id", requests[i].from_id);
		cJSON_AddStringToObject(item, "to_id", requests[i].to_id);
		cJSON_AddNumberToObject(item, "status", requests[i].status);
		cJSON_AddItemToArray(root, item);
	}

	char *txt = cJSON_Print(root);
	cJSON_Delete(root);
	if (txt == NULL) {
		return 0;
	}

	FILE *f = fopen(MATCH_REQUEST_PATH, "w");
	if (f == NULL) {
		free(txt);
		return 0;
	}
	fputs(txt, f);
	fclose(f);
	free(txt);
	return 1;
}

int match_store_send_request(const char from_id[], const char to_id[]) {
	if (from_id == NULL || to_id == NULL || from_id[0] == '\0' ||
		to_id[0] == '\0' || strcmp(from_id, to_id) == 0) {
		return 0;
	}

	MatchRequest requests[MATCH_MAX_REQUESTS];
	int count = match_load_all(requests, MATCH_MAX_REQUESTS);
	for (int i = 0; i < count; i++) {
		if (strcmp(requests[i].from_id, from_id) == 0 &&
			strcmp(requests[i].to_id, to_id) == 0) {
			if (requests[i].status == MATCH_REQUEST_REJECTED) {
				requests[i].status = MATCH_REQUEST_PENDING;
				return match_save_all(requests, count) ? 1 : 0;
			}
			return requests[i].status == MATCH_REQUEST_ACCEPTED ? 3 : 2;
		}
		if (strcmp(requests[i].from_id, to_id) == 0 &&
			strcmp(requests[i].to_id, from_id) == 0 &&
			requests[i].status == MATCH_REQUEST_ACCEPTED) {
			return 3;
		}
	}

	if (count >= MATCH_MAX_REQUESTS) {
		return 0;
	}
	match_copy_text(requests[count].from_id, sizeof(requests[count].from_id),
					from_id);
	match_copy_text(requests[count].to_id, sizeof(requests[count].to_id),
					to_id);
	requests[count].status = MATCH_REQUEST_PENDING;
	count++;
	return match_save_all(requests, count) ? 1 : 0;
}

int match_store_load_incoming(const char user_id[], MatchRequest requests[],
							  int max_requests) {
	MatchRequest all[MATCH_MAX_REQUESTS];
	int count = match_load_all(all, MATCH_MAX_REQUESTS);
	int found = 0;

	for (int i = 0; i < count && found < max_requests; i++) {
		if (strcmp(all[i].to_id, user_id) == 0 &&
			all[i].status == MATCH_REQUEST_PENDING) {
			requests[found++] = all[i];
		}
	}
	return found;
}

int match_store_accept_request(const char from_id[], const char to_id[]) {
	MatchRequest requests[MATCH_MAX_REQUESTS];
	int count = match_load_all(requests, MATCH_MAX_REQUESTS);

	for (int i = 0; i < count; i++) {
		if (strcmp(requests[i].from_id, from_id) == 0 &&
			strcmp(requests[i].to_id, to_id) == 0 &&
			requests[i].status == MATCH_REQUEST_PENDING) {
			requests[i].status = MATCH_REQUEST_ACCEPTED;
			return match_save_all(requests, count);
		}
	}
	return 0;
}

int match_store_has_accepted_match(const char user_a[], const char user_b[]) {
	MatchRequest requests[MATCH_MAX_REQUESTS];
	int count = match_load_all(requests, MATCH_MAX_REQUESTS);

	for (int i = 0; i < count; i++) {
		int same_order = strcmp(requests[i].from_id, user_a) == 0 &&
						 strcmp(requests[i].to_id, user_b) == 0;
		int reverse_order = strcmp(requests[i].from_id, user_b) == 0 &&
							strcmp(requests[i].to_id, user_a) == 0;
		if ((same_order || reverse_order) &&
			requests[i].status == MATCH_REQUEST_ACCEPTED) {
			return 1;
		}
	}
	return 0;
}

int match_store_find_accepted_partner(const char user_id[], char partner_id[],
									  size_t partner_id_size) {
	if (user_id == NULL || user_id[0] == '\0' || partner_id == NULL ||
		partner_id_size == 0) {
		return 0;
	}

	MatchRequest requests[MATCH_MAX_REQUESTS];
	int count = match_load_all(requests, MATCH_MAX_REQUESTS);
	for (int i = 0; i < count; i++) {
		if (requests[i].status != MATCH_REQUEST_ACCEPTED) {
			continue;
		}
		if (strcmp(requests[i].from_id, user_id) == 0) {
			match_copy_text(partner_id, partner_id_size, requests[i].to_id);
			return 1;
		}
		if (strcmp(requests[i].to_id, user_id) == 0) {
			match_copy_text(partner_id, partner_id_size, requests[i].from_id);
			return 1;
		}
	}
	return 0;
}

int match_store_cancel_match(const char user_a[], const char user_b[]) {
	if (user_a == NULL || user_b == NULL || user_a[0] == '\0' ||
		user_b[0] == '\0') {
		return 0;
	}

	MatchRequest requests[MATCH_MAX_REQUESTS];
	int count = match_load_all(requests, MATCH_MAX_REQUESTS);
	int changed = 0;
	for (int i = 0; i < count; i++) {
		int same_order = strcmp(requests[i].from_id, user_a) == 0 &&
						 strcmp(requests[i].to_id, user_b) == 0;
		int reverse_order = strcmp(requests[i].from_id, user_b) == 0 &&
							strcmp(requests[i].to_id, user_a) == 0;
		if ((same_order || reverse_order) &&
			requests[i].status != MATCH_REQUEST_REJECTED) {
			requests[i].status = MATCH_REQUEST_REJECTED;
			changed = 1;
		}
	}

	return changed ? match_save_all(requests, count) : 0;
}
