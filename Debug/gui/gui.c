#include <stdio.h>
#include <gtk/gtk.h>


// Use to keep state
struct TaskData {
	GtkWidget *counter_label;
	int count;
};


/* Async Background Processes */

static void print_hello(void) {
	g_print("Hello!\n");
}

static int print_counter(gpointer task_data) {
	struct TaskData *data = task_data;
	g_print("Counter: %d\n", data->count++);

	return 0;
}

/*static void update_counter_label(gpointer data) {
	struct TaskData *data = data;

	char *str = g_strdup_printf("Count: %d", data->count);

	gtk_label_set_text(GTK_LABEL(data->counter_label), str);
	g_free(str);
}*/


// The Background loop
static void count_to_five(GTask *task, gpointer source_obj, gpointer task_data, GCancellable *cancellable) {
	struct TaskData *data = task_data;

	for (int i = 0; i < 15; i++) {
		if (g_cancellable_is_cancelled(cancellable)) {
			g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Task cancelled");
			return;
		}

		// Invoke update label in the default main context
		g_main_context_invoke(NULL, print_counter, data);
		g_usleep(1000000);
	}

	g_task_return_boolean(task, TRUE);
}

static void count_to_five_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
	puts("Done");
}

static void count_to_five_async(GtkWidget *label, GCancellable *cancellable, GAsyncReadyCallback count_to_five_done) {
	struct TaskData *data = g_new(struct TaskData, 1);
	data->counter_label = label;
	data->count = 0;

	// Run count_to_five in a thread
	GTask *task = g_task_new(label, cancellable, count_to_five_done, data);
	g_task_set_task_data(task, data, g_free);
	g_task_run_in_thread(task, count_to_five);
	g_object_unref(task);
}



/* GUI */

static void activate(GtkApplication *app, gpointer data) {
	GtkWidget *window, *button, *box;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Test");
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

	gtk_window_set_child(GTK_WINDOW(window), box);

	button = gtk_button_new_with_label("Test");
	g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

	gtk_box_append(GTK_BOX(box), button);

	gtk_window_present(GTK_WINDOW(window));

	GCancellable *cancellable = g_cancellable_new();
	count_to_five_async(NULL, cancellable, count_to_five_done);
}


int main(int argc, char *argv[]) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
	return status;
}
