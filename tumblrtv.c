#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wordexp.h>
#include <webkit/webkit.h>


char* url_format = "https://www.tumblr.com/tv/%s";
//static gboolean closeWebViewCb(WebKitWebView* webView, GtkWidget* window);

static void destroyWindow(GtkWidget* widget, GtkWidget* data ) {
  gtk_main_quit ();
}

static void createDrawContext(GtkWidget** widget, GdkWindow** window){
  GtkWidget *main_window;
  GdkWindow *gdk_win;
  char* xwindow_s = getenv("XSCREENSAVER_WINDOW");

  if (xwindow_s != NULL) {
    // parse out the xid from xscreensaver's env var, which is given as
    // hex like 0x12023
    int xid = (int)strtol(xwindow_s, NULL, 0);

    // if we are running in xscreensaver mode, use the provided window XID
    main_window = gtk_window_new(GTK_WINDOW_POPUP);
    //main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gdk_win = gdk_window_foreign_new(xid);
    gtk_widget_show_all(main_window);
    // reparent window of main_window to gdk_win
    gdk_window_reparent(gtk_widget_get_window(main_window), gdk_win, 0, 0);
    gint width;
    gint height;
    gdk_window_get_geometry(gdk_win, NULL, NULL, &width, &height, NULL);
    printf("Looks like we are about %dx%d\n",width,height);
    // Make us cover our parent window
    gtk_window_move(main_window, 0, 0);
    gtk_window_set_default_size(main_window, width, height);
    gtk_widget_set_size_request(main_window, width, height);
  } else {
    // otherwise just get a normal window
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);
    gdk_win = gtk_widget_get_window(main_window);
  }
  *widget = main_window;
  *window = gdk_win;
}

int main(int argc, char* argv[])
{
    // Initialize GTK+
    gtk_init(&argc, &argv);

    GtkWidget *main_window;
    GdkWindow *gdk_window;
    createDrawContext(&main_window, &gdk_window);

    // Create a browser instance
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // Put the browser area into the main window
    gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(webView));

    // Set up callbacks so that if either the main window or the browser instance is
    // closed, the program will exit
    g_signal_connect(main_window, "destroy", G_CALLBACK(destroyWindow), NULL);
    g_signal_connect(main_window, "delete-event", G_CALLBACK(destroyWindow), NULL);

    // transparent background so we dont have any white flashes
    webkit_web_view_set_transparent(webView, true);

    int height;
    int width;
    gdk_window_get_geometry(gdk_window, NULL, NULL, &width, &height, NULL);
    if (height < 480 && width < 640) {
      // make the tiny preview windows look reasonable
      webkit_web_view_set_full_content_zoom(webView, true);
      webkit_web_view_set_zoom_level(webView, 0.4);
    }

    // set up some webkit settings
    WebKitWebSettings *settings = webkit_web_view_get_settings(webView);
    g_object_set (G_OBJECT(settings), "enable-webgl", true, NULL);

    // select tag and format url
    wordexp_t exp_result;
    wordexp("~/.tags", &exp_result, 0);
    printf("Loading tags file at %s\n", exp_result.we_wordv[0]);
    char* tags_path = exp_result.we_wordv[0];
    char url[1000];
    if (tags_path == NULL) {
        printf("No tag file found. defaulting to trippy\n");
        sprintf(url, url_format, "trippy");
    } else {
        char tags[100][100]; 
        FILE* tags_file = fopen(tags_path, "r");
        if (tags_file == NULL) {
            printf("File didn't open... defaulting to trippy\n");
            sprintf(url, url_format, "trippy");
            goto post_tags;
        }
        int i = 0;
        printf("Iterating through file\n");
        while (i < 100 && fgets(tags[i], sizeof(tags[0]), tags_file)) {
            tags[i][strlen(tags[i]) - 1] = '\0';
            printf("Got %s!\n", tags[i]);
            ++i;
        }
        printf("%d tags found. selecting one.\n", i);
        srand(time(0));
        int index = rand() % i;
        printf("%d is our index...\n", index);
        printf("%s selected!\n", tags[index]); 
        sprintf(url, url_format, tags[index]);
    }
post_tags:
    printf("URL is %s\n", url);

    webkit_web_view_load_uri(webView, url);

    // Make sure that when the browser area becomes visible, it will get mouse
    // and keyboard events
    gtk_widget_grab_focus(GTK_WIDGET(webView));

    // Make sure the main window and all its contents are visible
    gtk_widget_show_all(main_window);

    // Run the main GTK+ event loop
    gdk_threads_init();
    gtk_main();

    return 0;
}

