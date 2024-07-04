#include <gtk/gtk.h>
#include <glib/gstdio.h>

static void activate(GtkApplication *app, gpointer data) {
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "builder.ui",NULL);

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(GTK_WINDOW(window), app);

	gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
	g_object_unref(builder);
}

int main(int argc, char *argv[]) {
	GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
