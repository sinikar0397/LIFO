#include "mst.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static void mst_copy_text(char *dst, size_t dst_size, const char *src) {
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

static void mst_ensure_dirs(void) {
#ifdef _WIN32
	_mkdir("database");
	_mkdir("database/couples");
#else
	mkdir("database", 0755);
	mkdir("database/couples", 0755);
#endif
}

static void mst_make_safe_id(char out[], size_t out_size, const char id[]) {
	size_t j = 0;
	if (out_size == 0) {
		return;
	}
	if (id == NULL || id[0] == '\0') {
		mst_copy_text(out, out_size, "partner");
		return;
	}
	for (size_t i = 0; id[i] != '\0' && j + 1 < out_size; i++) {
		char ch = id[i];
		if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
			out[j++] = ch;
		} else {
			out[j++] = '_';
		}
	}
	out[j] = '\0';
}

static void mst_build_paths(MstCoupleSpace *space) {
	char a[MAX_ID_LEN];
	char b[MAX_ID_LEN];
	const char *first = space->owner_id;
	const char *second = space->partner_id;

	if (strcmp(first, second) > 0) {
		first = space->partner_id;
		second = space->owner_id;
	}
	mst_make_safe_id(a, sizeof(a), first);
	mst_make_safe_id(b, sizeof(b), second);
	snprintf(space->couple_key, sizeof(space->couple_key), "%s_%s", a, b);
	snprintf(space->save_path, sizeof(space->save_path),
			 "database/couples/%s.json", space->couple_key);
}

void mst_init_space(MstCoupleSpace *space, const char owner_id[],
					const char partner_id[]) {
	memset(space, 0, sizeof(*space));
	mst_copy_text(space->owner_id, sizeof(space->owner_id), owner_id);
	mst_copy_text(space->partner_id, sizeof(space->partner_id), partner_id);
	if (space->partner_id[0] == '\0') {
		mst_copy_text(space->partner_id, sizeof(space->partner_id), "partner");
	}
	mst_build_paths(space);
}

int mst_load_space(MstCoupleSpace *space, const char owner_id[],
				   const char partner_id[]) {
	mst_init_space(space, owner_id, partner_id);
	mst_ensure_dirs();

	FILE *f = fopen(space->save_path, "r");
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

	cJSON *events = cJSON_GetObjectItem(root, "events");
	if (cJSON_IsArray(events)) {
		cJSON *item = NULL;
		cJSON_ArrayForEach(item, events) {
			if (space->event_count >= MST_MAX_EVENTS) {
				break;
			}
			cJSON *date = cJSON_GetObjectItem(item, "date");
			cJSON *title = cJSON_GetObjectItem(item, "title");
			if (cJSON_IsString(date) && cJSON_IsString(title)) {
				MstCalendarEvent *event = &space->events[space->event_count++];
				mst_copy_text(event->date, sizeof(event->date), date->valuestring);
				mst_copy_text(event->title, sizeof(event->title), title->valuestring);
			}
		}
	}

	cJSON *buckets = cJSON_GetObjectItem(root, "buckets");
	if (cJSON_IsArray(buckets)) {
		cJSON *item = NULL;
		cJSON_ArrayForEach(item, buckets) {
			if (space->bucket_count >= MST_MAX_BUCKETS) {
				break;
			}
			cJSON *title = cJSON_GetObjectItem(item, "title");
			cJSON *done = cJSON_GetObjectItem(item, "done");
			if (cJSON_IsString(title)) {
				MstBucketItem *bucket = &space->buckets[space->bucket_count++];
				mst_copy_text(bucket->title, sizeof(bucket->title), title->valuestring);
				bucket->done = cJSON_IsTrue(done) || (cJSON_IsNumber(done) && done->valueint != 0);
			}
		}
	}

	cJSON_Delete(root);
	return 1;
}

int mst_save_space(const MstCoupleSpace *space) {
	mst_ensure_dirs();

	cJSON *root = cJSON_CreateObject();
	if (root == NULL) {
		return 0;
	}
	cJSON_AddStringToObject(root, "owner_id", space->owner_id);
	cJSON_AddStringToObject(root, "partner_id", space->partner_id);
	cJSON_AddStringToObject(root, "couple_key", space->couple_key);

	cJSON *events = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "events", events);
	for (int i = 0; i < space->event_count; i++) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject(item, "date", space->events[i].date);
		cJSON_AddStringToObject(item, "title", space->events[i].title);
		cJSON_AddItemToArray(events, item);
	}

	cJSON *buckets = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "buckets", buckets);
	for (int i = 0; i < space->bucket_count; i++) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject(item, "title", space->buckets[i].title);
		cJSON_AddNumberToObject(item, "done", space->buckets[i].done);
		cJSON_AddItemToArray(buckets, item);
	}

	char *txt = cJSON_Print(root);
	cJSON_Delete(root);
	if (txt == NULL) {
		return 0;
	}

	FILE *f = fopen(space->save_path, "w");
	if (f == NULL) {
		free(txt);
		return 0;
	}
	fputs(txt, f);
	fclose(f);
	free(txt);
	return 1;
}

int mst_add_event(MstCoupleSpace *space, const char date[], const char title[]) {
	if (space->event_count >= MST_MAX_EVENTS || date == NULL || title == NULL ||
		date[0] == '\0' || title[0] == '\0') {
		return 0;
	}
	MstCalendarEvent *event = &space->events[space->event_count++];
	mst_copy_text(event->date, sizeof(event->date), date);
	mst_copy_text(event->title, sizeof(event->title), title);
	return 1;
}

int mst_add_bucket_item(MstCoupleSpace *space, const char title[]) {
	if (space->bucket_count >= MST_MAX_BUCKETS || title == NULL ||
		title[0] == '\0') {
		return 0;
	}
	MstBucketItem *bucket = &space->buckets[space->bucket_count++];
	mst_copy_text(bucket->title, sizeof(bucket->title), title);
	bucket->done = 0;
	return 1;
}

void mst_toggle_bucket_item(MstCoupleSpace *space, int idx) {
	if (idx < 0 || idx >= space->bucket_count) {
		return;
	}
	space->buckets[idx].done = !space->buckets[idx].done;
}
