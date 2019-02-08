#ifndef __GES_TIMELINE_TREE__
#define __GES_TIMELINE_TREE__

#include <ges/ges.h>

void timeline_tree_track_element (GNode *self, GESTimelineElement *element);
void timeline_tree_stop_tracking_element  (GNode *self, GESTimelineElement *element);
gboolean timeline_tree_can_move_element(GNode *self, GESTimelineElement *element,
    guint32 priority, GstClockTime start, GstClockTime duration, GList *moving_track_elements);
void timeline_tree_init_debug(void);

#endif // __GES_TIMELINE_TREE__
