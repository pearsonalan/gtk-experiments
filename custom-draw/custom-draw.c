#include <assert.h>
#include <gtk/gtk.h>

#define APP_USER_DATA_MAGIC 0xEA4D57BB

typedef struct app_data {
  int magic;
  int count;
  /* Surface to store current scribbles */
  cairo_surface_t *surface;
} AppData;

static void clear_surface(cairo_surface_t *surface) {
  cairo_t *cr;

  cr = cairo_create(surface);

  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);

  cairo_destroy(cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean configure_cb(GtkWidget *widget, GdkEventConfigure *event,
    gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("configure\n");

  if (app_data->surface) {
    cairo_surface_destroy(app_data->surface);
		app_data->surface = NULL;
	}

  app_data->surface = gdk_window_create_similar_surface(
                  gtk_widget_get_window(widget),
                  CAIRO_CONTENT_COLOR,
                  gtk_widget_get_allocated_width(widget),
                  gtk_widget_get_allocated_height(widget));

  /* Initialize the surface to white */
  clear_surface(app_data->surface);

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("draw\n");

  cairo_set_source_surface(cr, app_data->surface, 0, 0);
  cairo_paint(cr);
  return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void draw_brush(GtkWidget *widget, cairo_surface_t *surface,
    gdouble x, gdouble y) {
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create(surface);

  cairo_rectangle(cr, x - 3, y - 3, 6, 6);
  cairo_fill(cr);

  cairo_destroy(cr);

  /* Now invalidate the affected region of the drawing area. */
  gtk_widget_queue_draw_area(widget, x - 3, y - 3, 6, 6);
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean button_press_cb(GtkWidget *widget,
    GdkEventButton *event, gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("button_press\n");

  /* paranoia check, in case we haven't gotten a configure event */
  if (app_data->surface == NULL) return FALSE;

  if (event->button == GDK_BUTTON_PRIMARY) {
    draw_brush(widget, app_data->surface, event->x, event->y);
  } else if (event->button == GDK_BUTTON_SECONDARY) {
    clear_surface(app_data->surface);
    gtk_widget_queue_draw(widget);
  }

  /* We've handled the event, stop processing */
  return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean motion_notify_cb(GtkWidget *widget,
    GdkEventMotion *event, gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("motion_notify\n");

  /* paranoia check, in case we haven't gotten a configure event */
  if (app_data->surface == NULL) return FALSE;

  if (event->state & GDK_BUTTON1_MASK) {
    draw_brush(widget, app_data->surface, event->x, event->y);
  }

  /* We've handled it, stop processing */
  return TRUE;
}

static gboolean key_press_cb(GtkWidget *widget, GdkEventKey *event,
    gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("key_press: keyval=%d, state=%08x\n", event->keyval, event->state);

	switch (event->keyval) {
	case GDK_KEY_F:
	case GDK_KEY_f:
	  gtk_window_fullscreen(GTK_WINDOW(widget));
		break;

	case GDK_KEY_Q:
	case GDK_KEY_q:
	  gtk_window_close(GTK_WINDOW(widget));
	  break;

	case GDK_KEY_Escape:
	  gtk_window_unfullscreen(GTK_WINDOW(widget));
		break;
	}

  return TRUE;
}

static void close_window_cb(GtkWidget *widget, gpointer data) {
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("close_window\n");

  if (app_data->surface) {
	  cairo_surface_destroy(app_data->surface);
		app_data->surface = NULL;
	}
}

static void activate(GtkApplication *app, gpointer data) {
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *drawing_area;
  AppData *app_data = (AppData *)data;

  assert(app_data != NULL);
  assert(app_data->magic == APP_USER_DATA_MAGIC);

	printf("activate\n");

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

  g_signal_connect(window, "destroy", G_CALLBACK(close_window_cb), data);

  gtk_container_set_border_width(GTK_CONTAINER(window), 8);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(window), frame);

  drawing_area = gtk_drawing_area_new();
  /* set a minimum size */
  gtk_widget_set_size_request(drawing_area, 400, 300);

  gtk_container_add(GTK_CONTAINER(frame), drawing_area);

  /* Signals used to handle the backing surface */
  g_signal_connect(drawing_area, "draw",
                   G_CALLBACK(draw_cb), data);
  g_signal_connect(drawing_area, "configure-event",
                   G_CALLBACK(configure_cb), data);

  /* Event signals */
  g_signal_connect(drawing_area, "motion-notify-event",
                   G_CALLBACK(motion_notify_cb), data);
  g_signal_connect(drawing_area, "button-press-event",
                   G_CALLBACK(button_press_cb), data);

  g_signal_connect(window, "key-press-event",
                   G_CALLBACK(key_press_cb), data);

  /* Ask to receive events the drawing area doesn't normally
   * subscribe to. In particular, we need to ask for the
   * button press and motion notify events that want to handle.
   */
  gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
                                     | GDK_BUTTON_PRESS_MASK
                                     | GDK_POINTER_MOTION_MASK);

  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  GtkApplication *app;
  int status;
  AppData app_data;

  app_data.magic = APP_USER_DATA_MAGIC;
	app_data.count = 0;
	app_data.surface = NULL;

  app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), &app_data);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
