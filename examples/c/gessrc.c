#include <ges/ges.h>

static void
bus_message_cb (GstBus * bus, GstMessage * message, GMainLoop * mainloop)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      g_printerr ("Got error message on the bus\n");
      g_main_loop_quit (mainloop);
      break;
    case GST_MESSAGE_EOS:
      g_print ("Done\n");
      g_main_loop_quit (mainloop);
      break;
    default:
      break;
  }
}

static void
source_setup_cb (GstElement * playbin, GstElement * source,
    GESTimeline * timeline)
{
  g_object_set (source, "timeline", timeline, NULL);
}

int
main (int argc, char **argv)
{
  GMainLoop *mainloop = NULL;
  GstElement *pipeline = NULL;
  GESTimeline *timeline;
  GESLayer *layer = NULL;
  GstBus *bus = NULL;
  guint i, ret = 0;
  gchar *uri = NULL;
  GstClockTime start = 0;

  if (argc < 2) {
    g_print ("Usage: %s <list of files>\n", argv[0]);
    return -1;
  }

  gst_init (&argc, &argv);
  ges_init ();

  timeline = ges_timeline_new_audio_video ();

  layer = (GESLayer *) ges_layer_new ();
  if (!ges_timeline_add_layer (timeline, layer))
    return -1;

  /* Build the timeline */
  for (i = 1; i < argc; i++) {
    GESClip *clip;

    uri = g_strdup (argv[i]);
    if (!gst_uri_is_valid (uri)) {
      g_free (uri);
      uri = gst_filename_to_uri (argv[i], NULL);
    }
    clip = GES_CLIP (ges_uri_clip_new (uri));

    if (!clip) {
      g_printerr ("Could not create clip for file: %s\n", argv[i]);
      g_free (uri);
      goto err;
    }
    g_object_set (clip, "start", start, NULL);
    ges_layer_add_clip (layer, clip);

    start += ges_timeline_element_get_duration (GES_TIMELINE_ELEMENT (clip));

    g_free (uri);
  }

  /* Use a usual playbin pipeline */
  pipeline = gst_element_factory_make ("playbin", NULL);
  g_object_set (pipeline, "uri", "ges://", NULL);
  g_signal_connect (pipeline, "source-setup", G_CALLBACK (source_setup_cb),
      timeline);

  mainloop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), mainloop);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (mainloop);
  gst_element_set_state (pipeline, GST_STATE_NULL);

done:
  gst_clear_object (&pipeline);
  if (mainloop)
    g_main_loop_unref (mainloop);

  return ret;
err:
  goto done;
}
