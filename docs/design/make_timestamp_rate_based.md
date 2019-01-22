# Make all TimelineElement timestamps "frame based" / rational numbers

Currently all timestamp in GES are GstClockTime but for video editing it makes
more sense to make timestamp based on frame timestamp/framerate based for frame
accuracy purposes. Note that all major video editing apps use frame based
"timestamps".

This is a proposal to allow GES to base timestamps on output framerate as well
as files framerates.

The idea would be to introduce a `GESTimeline:rate` property on which all
timestamps will be "snapped", with that done, we need each and every
`GESTimelineElement` timestamps inside a timeline snapped to it.

We still need backward compatibility with previous behaviour, and to do so, the
idea is to have a mode where no snapping happens, and to make that happen, the
possible solution is to make `GESTimeline:rate == GST_CLOCK_TIME_NONE` mean "no
snapping should happen.

Also one case we need to handle is setting the `GESTimeline:rate`, which
involves retimestamping all contained objects in a sensible way!

Also, setting `GESTimeline.rate` will override any
`GESTrack:restriction-caps['framerate']` value.

**Questions**:

* Should we restrict rates to SMPTE defined timestamps?
* What should be the `GESTimeline:rate` default value? `GST_CLOCK_TIME_NONE` so
  that default behaviour is unchanged
* When setting `gboolean GESTimelineElement:start` (and other timestamps) what
  should be the return value in case of snapping?
  * Could we abuse the "intness" nature of `gboolean` to let the user the
    offset between the real value and the one the user set?
  * In case we decide to make `GESTimeline:rate = GST_CLOCK_TIME_NONE` by
    default should we add an helper function to easily snap timestamp to the
    wanted rate and force the user to set properly snapped values?
    * I tend to think that doesn't work well as snapping should be different
      in case the value is a start (we should snap to previous frame in that
      case) or a duration (we should snap to the beginning of the next frame
      in that case)

* For `GESUriSource:inpoint` does it make sense to use the framerate of the
  input file? knowing that we anyway set have a videorate element that makes the
  output framerate the one of the `GESTrack:restriction-caps` value.

## API additions:

### `GESTimeline`

`GESTimeline:rate` property: The overall rate to be used for all timestamp
inside the timeline. `GST_CLOCK_TIME_NONE` means that the old behaviour is kept.
In case non rational variants of timestamps setters are used, rational times are
computed using that rate. This is also the default framerate for
`GESVideoTrack:restriction-caps`, not that rate is not a fraction but SMPTE
framerate values should (MUST?) be used.

### `GESTimelineElement`

Add `_rational_` timestamp setter variants, for example:

``` c
typedef struct
{
    gfloat rate;
    gint value;
} GESRationalTime;

/* if snap_edge is NONE, will simply round the value */
GESRationalTime *ges_rational_time_rescal_to(GESRationalTime *time, gint new_rate, GESEdge snap_edge);
GstClockTime ges_rational_time_to_clocktime(GESRationalTime *time);

/* Internally reuses what GstClockTime based things? Should we keep a reference on the
 * GESRationalTime? */
gboolean ges_timeline_element_set_rational_start(GESTimelineElement *element, gint value);
```
