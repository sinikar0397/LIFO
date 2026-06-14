#ifndef MST_H
#define MST_H

#include "../people/people.h"

#define MST_MAX_EVENTS 32
#define MST_MAX_BUCKETS 32
#define MST_TEXT_LEN 96
#define MST_DATE_LEN 16
#define MST_KEY_LEN (MAX_ID_LEN * 2 + 4)
#define MST_PATH_LEN 128

typedef struct MstCalendarEvent {
	char date[MST_DATE_LEN];
	char title[MST_TEXT_LEN];
} MstCalendarEvent;

typedef struct MstBucketItem {
	char title[MST_TEXT_LEN];
	int done;
} MstBucketItem;

typedef struct MstCoupleSpace {
	char owner_id[MAX_ID_LEN];
	char partner_id[MAX_ID_LEN];
	char couple_key[MST_KEY_LEN];
	char save_path[MST_PATH_LEN];
	MstCalendarEvent events[MST_MAX_EVENTS];
	int event_count;
	MstBucketItem buckets[MST_MAX_BUCKETS];
	int bucket_count;
} MstCoupleSpace;

void mst_init_space(MstCoupleSpace *space, const char owner_id[],
					const char partner_id[]);
int mst_load_space(MstCoupleSpace *space, const char owner_id[],
				   const char partner_id[]);
int mst_save_space(const MstCoupleSpace *space);
int mst_add_event(MstCoupleSpace *space, const char date[], const char title[]);
int mst_delete_event(MstCoupleSpace *space, int idx);
int mst_add_bucket_item(MstCoupleSpace *space, const char title[]);
int mst_delete_bucket_item(MstCoupleSpace *space, int idx);
void mst_toggle_bucket_item(MstCoupleSpace *space, int idx);

#endif // MST_H
