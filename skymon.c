//    HDS OPE file Editor
//      skymon.c  --- Sky Monitor Using Cairo
//   
//                                           2008.5.8  A.Tajitsu


#include"main.h"    // 設定ヘッダ
#include"version.h"

#ifdef USE_SKYMON
#include <cairo.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <signal.h>

#ifndef USE_WIN32
#include<sys/time.h>
#endif

extern void my_signal_connect();
#ifdef __GTK_STOCK_H__
extern GtkWidget* gtkut_button_new_from_stock();
extern GtkWidget* gtkut_toggle_button_new_from_stock();
#endif
extern GtkWidget* gtkut_toggle_button_new_from_pixbuf();
extern GtkWidget *make_menu();
extern void do_quit();

extern void calcpa2_skymon();

extern void get_current_obs_time();
extern void add_day();

extern int get_allsky();
extern gint update_allsky();
extern void allsky_read_data();

#ifdef USE_XMLRPC
extern gint update_telstat();
extern int close_telstat();
extern void cc_get_toggle();
#endif

extern void allsky_debug_print (const gchar *format, ...) G_GNUC_PRINTF(1, 2);

extern time_t ghttp_parse_date();

extern void do_update_azel();

extern void make_tree();

void create_skymon_dialog();
void close_skymon();

void screen_changed();
gboolean expose_skymon();
gboolean draw_skymon_cairo();
#ifdef USE_XMLRPC
gboolean draw_skymon_with_telstat_cairo();
#endif

void my_cairo_arc_center();
void my_cairo_arc_center2();
void my_cairo_arc_center_path();
void my_cairo_object();
void my_cairo_object2();
void my_cairo_object3();
void my_cairo_std();
void my_cairo_std2();
void my_cairo_moon();
void my_cairo_sun();
void my_cairo_planet();
#ifdef USE_XMLRPC
void my_cairo_telescope();
void my_cairo_telescope_cmd();
void my_cairo_telescope_path();
#endif

static void cc_skymon_mode ();
static void cc_get_adj ();
static void skymon_set_and_draw();
static void skymon_set_noobj();
static void skymon_set_hide();
static void skymon_set_allsky();
#ifdef USE_XMLRPC
static void skymon_set_telstat();
#endif

static void skymon_morning();
static void skymon_evening();

static void skymon_fwd();
gint skymon_go();
gint skymon_last_go();
static void skymon_rev();
gint skymon_back();
void skymon_set_time_current();

gint button_signal();
void draw_stderr_graph();


gboolean flagDrawing=FALSE;
gboolean supports_alpha = FALSE;
extern gboolean flagSkymon;
extern gboolean flagTree;
extern gboolean flag_getting_allsky;
gint old_width=0, old_height=0, old_w=0, old_h=0;
gdouble old_r=0;

//GdkPixmap *pixmap_skymon=NULL;

static gint work_page=0;

#ifndef USE_WIN32
extern pid_t allsky_pid;
#endif

extern void printf_log();

// Create Sky Monitor Window
void create_skymon_dialog(typHOE *hg)
{
  GtkWidget *vbox;
  GtkWidget *hbox, *hbox1, *ebox;
  GtkWidget *frame, *check, *label,*button,*spinner;
  GSList *group=NULL;
  GtkAdjustment *adj;
  GtkWidget *menubar;
#ifndef __GTK_TOOLTIP_H__
  GtkTooltips *tooltips;
#endif
  GdkPixbuf *icon;

  skymon_debug_print("Starting create_skymon_dialog\n");

  // Win構築は重いので先にExposeイベント等をすべて処理してから
  while (my_main_iteration(FALSE));
  gdk_flush();

  hg->skymon_mode=SKYMON_CUR;

  skymon_set_time_current(hg);
  
  hg->skymon_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(hg->skymon_main), "Sky Monitor : Main");
  //gtk_widget_set_usize(hg->skymon_main, SKYMON_SIZE, SKYMON_SIZE);
  
  my_signal_connect(hg->skymon_main, "destroy",
		    do_quit,(gpointer)hg);
  /*
  my_signal_connect(hg->skymon_main,
		    "destroy",
		    close_skymon, 
		    (gpointer)hg);
  */

  gtk_widget_set_app_paintable(hg->skymon_main, TRUE);
  

  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->skymon_main), vbox);

  menubar=make_menu(hg);
  gtk_box_pack_start(GTK_BOX(vbox), menubar,FALSE, FALSE, 0);


  // Menu
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  hg->skymon_frame_mode = gtk_frame_new ("Mode");
  gtk_box_pack_start(GTK_BOX(hbox), hg->skymon_frame_mode, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_frame_mode), 3);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Current",
		       1, SKYMON_CUR, -1);
    if(hg->skymon_mode==SKYMON_CUR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Set",
		       1, SKYMON_SET, -1);
    if(hg->skymon_mode==SKYMON_SET) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyCheck",
		       1, SKYMON_LAST, -1);
    if(hg->skymon_mode==SKYMON_LAST) iter_set=iter;
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add (GTK_CONTAINER (hg->skymon_frame_mode), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_skymon_mode,
		       (gpointer)hg);
  }

  
  hg->skymon_frame_date = gtk_frame_new ("Date");
  gtk_box_pack_start(GTK_BOX(hbox), hg->skymon_frame_date, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_frame_date), 3);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->skymon_frame_date), hbox1);

  skymon_set_time_current(hg);

  hg->skymon_adj_year = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_year,
							    hg->skymon_year-10, 
							    hg->skymon_year+99,
							    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_year, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (hg->skymon_adj_year, "value_changed",
		     cc_get_adj,
		     &hg->skymon_year);

  hg->skymon_adj_month = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_month,
							     1, 12, 1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_month, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (hg->skymon_adj_month, "value_changed",
		     cc_get_adj,
		     &hg->skymon_month);

  hg->skymon_adj_day = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_day,
							   1, 31, 1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_day, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (hg->skymon_adj_day, "value_changed",
		     cc_get_adj,
		     &hg->skymon_day);
  

  hg->skymon_frame_time = gtk_frame_new (hg->obs_tzname);
  gtk_box_pack_start(GTK_BOX(hbox), hg->skymon_frame_time, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_frame_time), 3);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->skymon_frame_time), hbox1);

  hg->skymon_adj_hour = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_hour,
							    0, 23,
							    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_hour, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (hg->skymon_adj_hour, "value_changed",
		     cc_get_adj,
		     &hg->skymon_hour);

  label=gtk_label_new(":");
  gtk_box_pack_start(GTK_BOX(hbox1),label,FALSE,FALSE,1);

  hg->skymon_adj_min = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_min,
							   0, 59,
							   1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_min, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (hg->skymon_adj_min, "value_changed",
		     cc_get_adj,
		     &hg->skymon_min);

#ifdef USE_XMLRPC
  frame = gtk_frame_new ("ASC/Telstat");
#else
  frame = gtk_frame_new ("ASC");
#endif
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
#ifdef __GTK_STOCK_H__
  //button=gtkut_toggle_button_new_from_stock(NULL, GTK_STOCK_ABOUT);
  icon = gdk_pixbuf_new_from_inline(sizeof(icon_feed), icon_feed, 
				    FALSE, NULL);

  button=gtkut_toggle_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
#else
  button=gtk_toggle_button_new_with_label("Show");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->allsky_flag);
  my_signal_connect(button,"toggled",
		    skymon_set_allsky, 
		    (gpointer)hg);

#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "All Sky Camera");
#else
  //gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       button,
  //		       "All Sky Camera",
  //		       NULL);
#endif


#ifdef __GTK_STOCK_H__
  button=gtkut_toggle_button_new_from_stock(NULL,GTK_STOCK_REMOVE);
#else
  button=gtk_toggle_button_new_with_label("Hide unused Obj.");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->hide_flag);
  my_signal_connect(button,"toggled",
		    skymon_set_hide, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Hide Objects unused in OPE file");
#else
  //gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       button,
  //		       "Hide Objects and Characters",
  //		       NULL);
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_toggle_button_new_from_stock(NULL,GTK_STOCK_STRIKETHROUGH);
#else
  button=gtk_toggle_button_new_with_label("Del Obj.");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->noobj_flag);
  my_signal_connect(button,"toggled",
		    skymon_set_noobj, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Hide Objects and Characters");
#else
  //gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       button,
  //		       "Hide Objects and Characters",
  //		       NULL);
#endif

#ifdef USE_XMLRPC
#ifdef __GTK_STOCK_H__
  //hg->skymon_button_telstat=gtkut_toggle_button_new_from_stock(NULL, 
  //						       GTK_STOCK_NETWORK);
  icon = gdk_pixbuf_new_from_inline(sizeof(icon_subaru), icon_subaru, 
				    FALSE, NULL);

  hg->skymon_button_telstat=gtkut_toggle_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
#else
  hg->skymon_button_telstat=gtk_toggle_button_new_with_label("Telstat");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_telstat), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_telstat,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->skymon_button_telstat),
			       hg->telstat_flag);
  my_signal_connect(hg->skymon_button_telstat,"toggled",
		    skymon_set_telstat, 
		    (gpointer)hg);

#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_telstat,
			      "Telescope Status");
#else
  //gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_telstat,
  //		       "Telescope Status",
  //		       NULL);
#endif

  /*
#ifdef __GTK_STOCK_H__
  button=gtkut_toggle_button_new_from_stock(NULL, GTK_STOCK_ZOOM_FIT);
#else
  button=gtk_toggle_button_new_with_label("Auto Mark");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->auto_check_lock);
  my_signal_connect(button,"toggled",
		    cc_get_toggle, 
		    &hg->auto_check_lock);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Auto Mark Current Target");
#else
  //gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       button,
  //		       "Auto Mark Current Target",
  //		       NULL);
#endif
  */

#endif  //USE_XMLRPC


  frame = gtk_frame_new ("Action");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);


#ifdef __GTK_STOCK_H__
  hg->skymon_button_set=gtkut_button_new_from_stock(NULL, GTK_STOCK_APPLY);
#else
  hg->skymon_button_set=gtk_button_new_with_label("Set");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_set), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_set,FALSE,FALSE,0);
  my_signal_connect(hg->skymon_button_set,"pressed",
		    skymon_set_and_draw, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_set,
			      "Set Date & Time");
#else
  //  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_set,
  //		       "Set Date & Time",
  //		       NULL);
#endif


#ifdef __GTK_STOCK_H__
  hg->skymon_button_even=gtkut_button_new_from_stock(NULL, GTK_STOCK_MEDIA_PREVIOUS);
#else
  hg->skymon_button_even=gtk_button_new_with_label("Even");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_even), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_even,FALSE,FALSE,0);
  my_signal_connect(hg->skymon_button_even,"pressed",
		    skymon_evening, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_even,
			      "Evening");
#else
  //  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_even,
  //		       "Evening",
  //		       NULL);
#endif


#ifdef __GTK_STOCK_H__
  hg->skymon_button_rev=gtkut_toggle_button_new_from_stock(NULL, GTK_STOCK_MEDIA_REWIND);
#else
  hg->skymon_button_rev=gtk_toggle_button_new_with_label("Rew");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_rev), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_rev,FALSE,FALSE,0);
  my_signal_connect(hg->skymon_button_rev,"toggled",
		    skymon_rev, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_rev,
			      "Rew");
#else
  //  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_rev,
  //	       "Rew",
  //		       NULL);
#endif


#ifdef __GTK_STOCK_H__
  hg->skymon_button_fwd=gtkut_toggle_button_new_from_stock(NULL, GTK_STOCK_MEDIA_FORWARD);
#else
  hg->skymon_button_fwd=gtk_toggle_button_new_with_label("Fwd");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_fwd), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_fwd,FALSE,FALSE,0);
  my_signal_connect(hg->skymon_button_fwd,"toggled",
		    skymon_fwd, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_fwd,
			      "FF");
#else
  // gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_fwd,
  //		       "FF",
  //		       NULL);
#endif


#ifdef __GTK_STOCK_H__
  hg->skymon_button_morn=gtkut_button_new_from_stock(NULL, GTK_STOCK_MEDIA_NEXT);
#else
  hg->skymon_button_morn=gtk_button_new_with_label("Morn");
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_button_morn), 0);
  gtk_box_pack_start(GTK_BOX(hbox1),hg->skymon_button_morn,FALSE,FALSE,0);
  my_signal_connect(hg->skymon_button_morn,"pressed",
		    skymon_morning, 
		    (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->skymon_button_morn,
			      "Morning");
#else
  //  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),
  //		       hg->skymon_button_morn,
  //		       "Morning",
  //		       NULL);
#endif


  gtk_widget_set_sensitive(hg->skymon_frame_date,FALSE);
  gtk_widget_set_sensitive(hg->skymon_frame_time,FALSE);
  gtk_widget_set_sensitive(hg->skymon_button_fwd,FALSE);
  gtk_widget_set_sensitive(hg->skymon_button_rev,FALSE);
  gtk_widget_set_sensitive(hg->skymon_button_morn,FALSE);
  gtk_widget_set_sensitive(hg->skymon_button_even,FALSE);


  hg->skymon_frame_sz = gtk_frame_new ("Sz.");
  gtk_box_pack_start(GTK_BOX(hbox), hg->skymon_frame_sz, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->skymon_frame_sz), 3);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->skymon_frame_sz), hbox1);

  hg->skymon_adj_objsz 
    = (GtkAdjustment *)gtk_adjustment_new(hg->skymon_objsz,
					  0, 32,
					  1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->skymon_adj_objsz, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 FALSE);
  gtk_box_pack_start(GTK_BOX(hbox1),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (hg->skymon_adj_objsz, "value_changed",
		     cc_get_adj,
		     &hg->skymon_objsz);



  
  // Drawing Area
  ebox=gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), ebox, TRUE, TRUE, 0);
  hg->skymon_dw = gtk_drawing_area_new();
  gtk_widget_set_size_request (hg->skymon_dw, SKYMON_SIZE, SKYMON_SIZE);
  //gtk_box_pack_start(GTK_BOX(vbox), hg->skymon_dw, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(ebox), hg->skymon_dw);
  gtk_widget_set_app_paintable(hg->skymon_dw, TRUE);
  gtk_widget_show(hg->skymon_dw);

  screen_changed(hg->skymon_dw,NULL,NULL);

  my_signal_connect(hg->skymon_dw, 
		    "expose-event", 
		    expose_skymon,
		    (gpointer)hg);

  my_signal_connect(ebox, 
		    "button-press-event", 
		    button_signal,
		    (gpointer)hg);

  

  gtk_widget_show_all(hg->skymon_main);

  gdk_flush();

  skymon_debug_print("Finishing create_skymon_dialog\n");
}


void close_skymon(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;

  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hg->skymon_button_fwd))){
    gtk_timeout_remove(hg->skymon_timer);
  }
  else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hg->skymon_button_rev))){
    gtk_timeout_remove(hg->skymon_timer);
  }

  gtk_widget_destroy(GTK_WIDGET(w));
  flagSkymon=FALSE;
}


void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata)
{
    /* To check if the display supports alpha channels, get the colormap */
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

    if (!colormap)
    {
      //printf("Your screen does not support alpha channels!\n");
      colormap = gdk_screen_get_rgb_colormap(screen);
      supports_alpha = FALSE;
    }
    else
    {
      //printf("Your screen supports alpha channels!\n");
      supports_alpha = TRUE;
    }
    fflush(stdout);

    /* Now we have a colormap appropriate for the screen, use it */
    gtk_widget_set_colormap(widget, colormap);
}

gboolean expose_skymon(GtkWidget *widget, 
		       GdkEventExpose *event, 
		       gpointer userdata){
  typHOE *hg;

  if(!flagSkymon) return (FALSE);

  hg=(typHOE *)userdata;

  draw_skymon(widget, hg, FALSE);
}


gboolean draw_skymon(GtkWidget *widget, typHOE *hg, gboolean force_flag){
  if(!flagSkymon) return (FALSE);

  draw_skymon_cairo(widget, hg, force_flag);
#ifdef USE_XMLRPC
  draw_skymon_with_telstat_cairo(widget, hg);
#endif
}


gboolean draw_skymon_cairo(GtkWidget *widget, 
			   typHOE *hg, gboolean force_flag){
  cairo_t *cr;
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  gint from_set, to_rise,
    min_tw06s,min_tw12s, min_tw18s,min_tw06r,min_tw12r,min_tw18r;
  gint w=0,h=0;
  gdouble r=1.0;
  gdouble e_h;
  gint off_x, off_y;
  gboolean pixbuf_flag=FALSE;
  gboolean as_flag=FALSE;
  double y_ul, y_bl, y_ur, y_br;
  cairo_status_t cr_stat;

  if(!flagSkymon) return (FALSE);

  if(flagDrawing){
    allsky_debug_print("!!! Collision in DrawSkymon, skipped...\n");
    return(FALSE);
  }
  else{
    flagDrawing=TRUE;
  }

  skymon_debug_print("Starting draw_skymon_cairo\n");
  //printf_log(hg,"[SkyMon] drawing sky monitor image.");

  int width, height;
  width= widget->allocation.width;
  height= widget->allocation.height;
  if(width<=1){
    gtk_window_get_size(GTK_WINDOW(hg->skymon_main), &width, &height);
  }

  hg->win_cx=(gdouble)width/2.0;
  hg->win_cy=(gdouble)height/2.0;
  if(width < height){
    hg->win_r=hg->win_cx*0.9;
  }
  else{
    hg->win_r=hg->win_cy*0.9;
  }
  
  if(hg->pixmap_skymon){
    g_object_unref(G_OBJECT(hg->pixmap_skymon));
  }

  hg->pixmap_skymon = gdk_pixmap_new(widget->window,
				     width,
				     height,
				     -1);


  
  //cr = gdk_cairo_create(widget->window);
  cr = gdk_cairo_create(hg->pixmap_skymon);

  cr_stat=cairo_status(cr);
  if(cr_stat!=CAIRO_STATUS_SUCCESS){
    printf_log(hg,"[SkyMon] Error on gdk_cairo_create()");
    allsky_debug_print("Error on gdk_cairo_create()\n");
    cairo_destroy(cr);
    return TRUE;
  }


  cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    
  /*
  if (supports_alpha)
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); // transparen
  else
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); // opaque white
  */

  //cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
  cairo_set_source_rgba(cr, 1.0, 0.9, 0.8, 1.0);
  
  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  /* draw a circle */
  


  // All Sky
  if((hg->allsky_flag) && (hg->allsky_last_i>0) &&
     ( (hg->skymon_mode==SKYMON_CUR) || (hg->skymon_mode==SKYMON_LAST)) ){
    //El =0
    if(hg->allsky_diff_flag){
      cairo_set_source_rgba(cr, 
			    (gdouble)hg->allsky_diff_base/(gdouble)0xFF,
			    (gdouble)hg->allsky_diff_base/(gdouble)0xFF,
			    (gdouble)hg->allsky_diff_base/(gdouble)0xFF, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.0,0.0,0.0,1.0);
    }
    my_cairo_arc_center (cr, width, height, 0.0); 
    cairo_fill(cr);

    as_flag=hg->allsky_flag;
    // read new image
    cairo_save (cr);
    if(hg->skymon_mode==SKYMON_CUR){
      if((strcmp(hg->allsky_date,hg->allsky_date_old))||force_flag){
	if(hg->pixbuf)  g_object_unref(G_OBJECT(hg->pixbuf));
	if(!hg->allsky_pixbuf_flag){
	  if(hg->allsky_limit){
	    hg->pixbuf
	      = gdk_pixbuf_new_from_file_at_size(hg->allsky_file,
						 ALLSKY_LIMIT,
						 ALLSKY_LIMIT,
						 NULL);
	  }else{
	    hg->pixbuf = gdk_pixbuf_new_from_file(hg->allsky_file, NULL);
	  }
	}
	else{
	  if(hg->allsky_diff_flag){
	    if(hg->allsky_last_i==1){
	      if(hg->allsky_last_pixbuf[hg->allsky_last_i-1])
		hg->pixbuf = 
		  gdk_pixbuf_copy(hg->allsky_last_pixbuf[hg->allsky_last_i-1]);
	    }
	    else{
	      if(hg->allsky_diff_pixbuf[hg->allsky_last_i-1])
		hg->pixbuf =
		  gdk_pixbuf_copy(hg->allsky_diff_pixbuf[hg->allsky_last_i-1]);
	    }
	  }
	  else{
	    if(hg->allsky_last_pixbuf[hg->allsky_last_i-1])
	      hg->pixbuf = 
		gdk_pixbuf_copy(hg->allsky_last_pixbuf[hg->allsky_last_i-1]);
	  }
	}

	if(GDK_IS_PIXBUF(hg->pixbuf)){
	  if((hg->allsky_sat<1.0)||(hg->allsky_sat>1.0))
	    gdk_pixbuf_saturate_and_pixelate(hg->pixbuf,hg->pixbuf,
					     (gfloat)hg->allsky_sat,FALSE);
	  if(hg->allsky_date_old) g_free(hg->allsky_date_old);
	  pixbuf_flag=TRUE;
	  hg->allsky_date_old=g_strdup(hg->allsky_date);
	  skymon_debug_print("*** created new pixbuf\n");
	}
	else{
	  printf_log("[AllSky] failed to read allsky image.");
	}
      }
      else{
	skymon_debug_print("*** using old pixbuf\n");
      }

    }
    else if ((hg->skymon_mode==SKYMON_LAST)&&(hg->allsky_last_repeat==0)) {
      if(hg->pixbuf)  g_object_unref(G_OBJECT(hg->pixbuf));

      if(!hg->allsky_pixbuf_flag){
	if(access(hg->allsky_last_file[hg->allsky_last_frame],F_OK)==0){
	  if(hg->allsky_limit){
	    hg->pixbuf
	      = gdk_pixbuf_new_from_file_at_size(hg->allsky_last_file[hg->allsky_last_frame],
						 ALLSKY_LIMIT,
						 ALLSKY_LIMIT,
						 NULL);
	  }else{
	    hg->pixbuf = gdk_pixbuf_new_from_file(hg->allsky_last_file[hg->allsky_last_frame], NULL);
	  }
	  if(GDK_IS_PIXBUF(hg->pixbuf)){
	    pixbuf_flag=TRUE;
	  }
	  else{
	    printf_log("[AllSky] failed to read allsky image.");
	  }
	}
	else{
	  hg->skymon_mode=SKYMON_CUR;
	  //printf("%s not found\n",hg->allsky_last_file[hg->allsky_last_frame]);
	}
      }
      else{
	if ((hg->allsky_diff_flag)
	    &&(hg->allsky_diff_pixbuf[hg->allsky_last_frame])){
	  hg->pixbuf = gdk_pixbuf_copy(hg->allsky_diff_pixbuf[hg->allsky_last_frame]);
	  pixbuf_flag=TRUE;
	}
	else if(hg->allsky_last_pixbuf[hg->allsky_last_frame]){
	  hg->pixbuf = gdk_pixbuf_copy(hg->allsky_last_pixbuf[hg->allsky_last_frame]);
	  pixbuf_flag=TRUE;
	}
	else{
	  hg->skymon_mode=SKYMON_CUR;
	}
      }
      if (pixbuf_flag){
	if((hg->allsky_sat<1,0)||(hg->allsky_sat>1.0))
	   gdk_pixbuf_saturate_and_pixelate(hg->pixbuf,hg->pixbuf,
					    (gfloat)hg->allsky_sat,FALSE);
      }
    }

    if(GDK_IS_PIXBUF(hg->pixbuf)){
      if((pixbuf_flag)||(width!=old_width)||(height!=old_height)){
	if(width>height){
	  r=(gdouble)height/((gdouble)hg->allsky_diameter/0.9);
	}
	else{
	  r=(gdouble)width/((gdouble)hg->allsky_diameter/0.9);
	}
	
	w = gdk_pixbuf_get_width(hg->pixbuf);
	h = gdk_pixbuf_get_height(hg->pixbuf);
      
	w=(gint)((gdouble)w*r);
	h=(gint)((gdouble)h*r);

	if(hg->pixbuf2) g_object_unref(G_OBJECT(hg->pixbuf2));
	hg->pixbuf2=gdk_pixbuf_scale_simple(hg->pixbuf,w,h,GDK_INTERP_BILINEAR);

	old_width=width;
	old_height=height;
	old_r=r;
	old_w=w;
	old_h=h;

	skymon_debug_print("*** created new pixbuf2\n");
      }
      else{
	r=old_r;
	w=old_w;
	h=old_h;
	skymon_debug_print("*** using old pixbuf2\n");
      }

      off_x=(gint)((gdouble)width/2-(gdouble)hg->allsky_centerx*r);
      off_y=(gint)((gdouble)height/2-(gdouble)hg->allsky_centery*r);
      
      cairo_translate(cr,off_x,off_y);
      
      cairo_translate(cr,(gdouble)hg->allsky_centerx*r,
		      (gdouble)hg->allsky_centery*r);
      cairo_rotate (cr,M_PI*hg->allsky_angle/180.);
      cairo_translate(cr,-(gdouble)hg->allsky_centerx*r,
		      -(gdouble)hg->allsky_centery*r);
      gdk_cairo_set_source_pixbuf(cr, hg->pixbuf2, 0, 0);
      
      my_cairo_arc_center2 (cr, width, height, -off_x,-off_y, 0.0); 
      cairo_fill(cr);
      if ((hg->skymon_mode==SKYMON_CUR)&&(!hg->noobj_flag)){
	if(hg->allsky_alpha<0){
	  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 
				-(gdouble)hg->allsky_alpha/100);
	  my_cairo_arc_center2 (cr, width, height, -off_x,-off_y, 0.0); 
	  cairo_fill(cr);
	}
	else if(hg->allsky_alpha>0){
	  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 
				(gdouble)hg->allsky_alpha/100);
	  my_cairo_arc_center2 (cr, width, height, -off_x,-off_y, 0.0); 
	  cairo_fill(cr);
	}
      }
    }
    cairo_restore(cr);
  }
  else{
    //El =0
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    my_cairo_arc_center (cr, width, height, 0.0); 
    cairo_fill(cr);
  }

  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  my_cairo_arc_center (cr, width, height, 0.0); 
  //cairo_fill(cr);
  cairo_stroke(cr);

  //El =15
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  my_cairo_arc_center (cr, width, height, 15.0); 
  cairo_set_line_width(cr,1.0);
  //cairo_fill(cr);
  cairo_stroke(cr);
  cairo_set_line_width(cr,2.0);
  
  //El =30
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  my_cairo_arc_center (cr, width, height, 30.0); 
  //cairo_fill(cr);
  cairo_stroke(cr);
  
  //El =60
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  my_cairo_arc_center (cr, width, height, 60.0); 
  //cairo_fill(cr);
  cairo_stroke(cr);
  
  // ZENITH
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  my_cairo_arc_center (cr, width, height, 89.0); 
  cairo_fill(cr);

  // N-S
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  cairo_move_to ( cr, width/2,
		  (width<height ? height/2-width/2 * 0.9 : height*0.05) ); 
  cairo_line_to ( cr, width/2,
		  (width<height ? height/2+width/2 * 0.9 : height*0.95) ); 
  cairo_set_line_width(cr,1.0);
  cairo_stroke(cr);
  cairo_set_line_width(cr,2.0);
  
  // W-E
  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);
  cairo_move_to ( cr, 
		  (width<height ? width*0.05 : width/2-height/2*0.9),
		   height/2);
  cairo_line_to ( cr, 
		  (width<height ? width*0.95 : width/2+height/2*0.9),
		   height/2);
  cairo_set_line_width(cr,1.0);
  cairo_stroke(cr);
  cairo_set_line_width(cr,2.0);

  cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);

  // N
  cairo_set_source_rgba(cr, 0.8, 0.4, 0.2, 1.0);
  cairo_set_font_size (cr, 14.0);
  cairo_text_extents (cr, "N", &extents);
  x = 0-(extents.width/2 + extents.x_bearing);
  y = 0;
  x += width/2; 
  y += (width<height ? height/2-width/2 * 0.9 : height*0.05) -2; 
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "N");

  // S
  cairo_set_source_rgba(cr, 0.8, 0.4, 0.2, 1.0);
  cairo_set_font_size (cr, 14.0);
  cairo_text_extents (cr, "S", &extents);
  x = 0-(extents.width/2 + extents.x_bearing);
  y = 0+extents.height;
  x += width/2; 
  y += (width<height ? height/2+width/2 * 0.9 : height*0.95)+2; 
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "S");

  // E
  cairo_set_source_rgba(cr, 0.8, 0.4, 0.2, 1.0);
  cairo_set_font_size (cr, 14.0);
  cairo_text_extents (cr, "E", &extents);
  x = 0-extents.width;
  y = 0-(extents.height/2 + extents.y_bearing);
  x += (width<height ? width*0.05 : width/2-height/2*0.9) -2;
  y += height/2;
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "E");

  // W
  cairo_set_source_rgba(cr, 0.8, 0.4, 0.2, 1.0);
  cairo_set_font_size (cr, 14.0);
  cairo_text_extents (cr, "W", &extents);
  x = 0;
  y = 0-(extents.height/2 + extents.y_bearing);
  x += (width<height ? width*0.95 : width/2+height/2*0.9) +2;
  y += height/2;
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "W");

  if((hg->skymon_mode==SKYMON_CUR)
     &&(hg->allsky_flag)
     &&(hg->allsky_diff_flag)
     &&(hg->allsky_last_i==1)){
    cairo_text_extents_t ext_w;
    
    cairo_set_font_size (cr, 10.0);
    cairo_text_extents (cr, "Waiting for the next image to create a differential sky...", &ext_w);

    cairo_move_to(cr, 
		  width/2-ext_w.width/2.,
		  height/2-(width < height ? width : height)*90./100./4.);
    cairo_text_path(cr, "Waiting for the next image to create a differential sky...");
    cairo_set_source_rgba(cr, 0.0,0.0,0.0,
			  (gdouble)hg->alpha_edge/0x10000);
    cairo_set_line_width(cr, (double)hg->size_edge);
    cairo_stroke(cr);
    
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
    cairo_move_to(cr, 
		  width/2-ext_w.width/2.,
		  height/2-(width < height ? width : height)*90./100./4.);
    cairo_show_text(cr, "Waiting for the next image to create a differential sky...");
  }
    
  // Date 
  {
    gchar tmp[64];
    time_t t;
    struct tm *tmpt;
    int year, month, day, hour, min;
    double sec;
    struct ln_zonedate zonedate;
    struct ln_date date;
    gdouble base_height,w_rise,w_digit;
    cairo_text_extents_t ext_s;


    if(hg->skymon_mode==SKYMON_SET){
      year=hg->skymon_year;
      month=hg->skymon_month;
      day=hg->skymon_day;
      
      hour=hg->skymon_hour;
      min=hg->skymon_min;
      sec=0.0;
    }
    else if(hg->skymon_mode==SKYMON_LAST){
      tmpt = localtime(&hg->allsky_last_t[hg->allsky_last_frame]);

      year=tmpt->tm_year+1900;
      month=tmpt->tm_mon+1;
      day=tmpt->tm_mday;
      
      hour=tmpt->tm_hour;
      min=tmpt->tm_min;
      sec=tmpt->tm_sec;
    }
    else{
      get_current_obs_time(hg,&year, &month, &day, &hour, &min, &sec);
    }

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);

#ifdef USE_XMLRPC
    if((hg->stat_obcp)&&(hg->skymon_mode==SKYMON_CUR)){
      sprintf(tmp,"%02d/%02d/%04d  [%s]",month,day,year,hg->stat_obcp);
    }
    else{
      sprintf(tmp,"%02d/%02d/%04d",month,day,year);
    }
#else
    sprintf(tmp,"%02d/%02d/%04d",month,day,year);
#endif
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_set_font_size (cr, 12.0);
    cairo_text_extents (cr, tmp, &extents);
    e_h=extents.height;
    cairo_move_to(cr,0,+e_h);
    cairo_show_text(cr, tmp);

    sprintf(tmp,"%s=%02d:%02d",hg->obs_tzname,hour,min);
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_set_font_size (cr, 12.0);
    cairo_move_to(cr,10,+e_h*2+5);
    cairo_show_text(cr, tmp);

    zonedate.years=year;
    zonedate.months=month;
    zonedate.days=day;
    zonedate.hours=hour;
    zonedate.minutes=min;
    zonedate.seconds=sec;
    zonedate.gmtoff=(long)hg->obs_timezone*3600;

    ln_zonedate_to_date(&zonedate, &date);

    sprintf(tmp,"UT =%02d:%02d",
	    date.hours,date.minutes);
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_set_font_size (cr, 12.0);
    cairo_move_to(cr,10,+e_h*3+10);
    cairo_show_text(cr, tmp);

    cairo_set_font_size (cr, 10.0);
    if(date.days!=day){
      if(hg->obs_timezone>0){
	cairo_show_text(cr, " [-1day]");
      }
      else{
	cairo_show_text(cr, " [+1day]");
      }
    }

    if(hg->skymon_mode!=SKYMON_LAST){
      if(hg->skymon_mode==SKYMON_SET){
	sprintf(tmp,"LST=%02d:%02d",hg->skymon_lst_hour,hg->skymon_lst_min);
      }
      else{
	sprintf(tmp,"LST=%02d:%02d",hg->lst_hour,hg->lst_min);
      }
      cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
      cairo_set_font_size (cr, 12.0);
      cairo_move_to(cr,10,+e_h*4+15);
      cairo_show_text(cr, tmp);
    }

    // For Obj Tree Label
    if(hg->skymon_mode==SKYMON_CUR){
      sprintf(tmp,"***Current*** (%02d/%02d/%04d  %s=%02d:%02d LST=%02d:%02d)",
	      month,day,year,
	      hg->obs_tzname,
	      hour,min,
	      hg->lst_hour,hg->lst_min);
    }
    else{
      sprintf(tmp,"***Set*** (%02d/%02d/%04d  %s=%02d:%02d LST=%02d:%02d)",
	      month,day,year,
	      hg->obs_tzname,
	      hour,min,
	      hg->skymon_lst_hour,hg->skymon_lst_min);
    }
    if(hg->tree_label_text) g_free(hg->tree_label_text);
    hg->tree_label_text=g_strdup(tmp);
    if(flagTree){
      if(!hg->tree_editing){
	gtk_label_set_text(GTK_LABEL(hg->tree_label),hg->tree_label_text);
      }
    }


    base_height=e_h*5+30;
    cairo_set_font_size (cr, 10.0);
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);

    cairo_text_extents (cr, "99:99", &ext_s);

    cairo_move_to(cr,5,base_height);
    cairo_show_text(cr, "Set");

    cairo_move_to(cr,5,base_height+ext_s.height+4);
    cairo_show_text(cr, "Rise");

    cairo_set_font_size (cr, 9.0);
    cairo_move_to(cr,5+ext_s.width,base_height-ext_s.height-2);
    cairo_show_text(cr, "Sun");
    cairo_move_to(cr,5+ext_s.width*2+5,base_height-ext_s.height-2);
    cairo_show_text(cr, "Tw12");
    cairo_move_to(cr,5+ext_s.width*3+10,base_height-ext_s.height-2);
    cairo_show_text(cr, "Tw18");

    cairo_set_font_size (cr, 10.0);
    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"%02d:%02d",
	      hg->sun.s_set.hours,hg->sun.s_set.minutes);
      cairo_move_to(cr,5+ext_s.width,base_height);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw12.s_set.hours,hg->atw12.s_set.minutes);
      cairo_move_to(cr,5+ext_s.width*2+5,base_height);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw18.s_set.hours,hg->atw18.s_set.minutes);
      cairo_move_to(cr,5+ext_s.width*3+10,base_height);
      cairo_show_text(cr, tmp);

      //sprintf(tmp,"SunSet=%02d:%02d",
      //      hg->sun.s_set.hours,hg->sun.s_set.minutes);
    }
    else{
      sprintf(tmp,"%02d:%02d",
	      hg->sun.c_set.hours,hg->sun.c_set.minutes);
      cairo_move_to(cr,5+ext_s.width,base_height);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw12.c_set.hours,hg->atw12.c_set.minutes);
      cairo_move_to(cr,5+ext_s.width*2+5,base_height);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw18.c_set.hours,hg->atw18.c_set.minutes);
      cairo_move_to(cr,5+ext_s.width*3+10,base_height);
      cairo_show_text(cr, tmp);
    }

    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"%02d:%02d",
	      hg->sun.s_rise.hours,hg->sun.s_rise.minutes);
      cairo_move_to(cr,5+ext_s.width,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw12.s_rise.hours,hg->atw12.s_rise.minutes);
      cairo_move_to(cr,5+ext_s.width*2+5,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw18.s_rise.hours,hg->atw18.s_rise.minutes);
      cairo_move_to(cr,5+ext_s.width*3+10,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);
    }
    else{
      sprintf(tmp,"%02d:%02d",
	      hg->sun.c_rise.hours,hg->sun.c_rise.minutes);
      cairo_move_to(cr,5+ext_s.width,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw12.c_rise.hours,hg->atw12.c_rise.minutes);
      cairo_move_to(cr,5+ext_s.width*2+5,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);

      sprintf(tmp,"%02d:%02d",
	      hg->atw18.c_rise.hours,hg->atw18.c_rise.minutes);
      cairo_move_to(cr,5+ext_s.width*3+10,base_height+ext_s.height+4);
      cairo_show_text(cr, tmp);
    }


    if(hg->skymon_mode==SKYMON_LAST){
      gdouble x0, y0, dy;
      gint i_last;
      
      x0=10;
      y0=+e_h*7+35;

      dy=e_h;

      cairo_move_to(cr,x0,y0);

      for(i_last=0;i_last<hg->allsky_last_i;i_last++){
	if(i_last==hg->allsky_last_frame){
	  
	  if((!hg->allsky_cloud_show)||(hg->allsky_cloud_area[i_last]<10.)){
	    cairo_set_source_rgba(cr, 0.3, 1.0, 0.3, 1.0);
	  }
	  else if(hg->allsky_cloud_area[i_last]<50.){
	    cairo_set_source_rgba(cr, 
				  1.0-0.7*(50.-hg->allsky_cloud_area[i_last])/40.,
				  1.0, 0.3, 1.0);
	  }
	  else if(hg->allsky_cloud_area[i_last]<90.){
	    cairo_set_source_rgba(cr, 
				  1.0, 0.3+0.7*(90.-hg->allsky_cloud_area[i_last])/40.,
				  0.3, 1.0);
	  }
	  else{
	    cairo_set_source_rgba(cr, 1.0, 0.3, 0.3, 1.0);
	  }
	}
	else{
	  if((!hg->allsky_cloud_show)||(hg->allsky_cloud_area[i_last]<10.)){
	    cairo_set_source_rgba(cr, 0, 0.8, 0, 1.0);
	  }
	  else if(hg->allsky_cloud_area[i_last]<50.){
	    cairo_set_source_rgba(cr, 
				  0.8-0.8*(50.-hg->allsky_cloud_area[i_last])/40.,
				  0.8, 0, 1.0);
	  }
	  else if(hg->allsky_cloud_area[i_last]<90.){
	    cairo_set_source_rgba(cr, 
				  0.8, 0.8*(90.-hg->allsky_cloud_area[i_last])/40.,
				  0,  1.0);
	  }
	  else{
	    cairo_set_source_rgba(cr, 0.8, 0, 0, 1.0);
	  }
	}
	cairo_rectangle (cr, x0+1, y0+1+dy*(gdouble)(i_last),
			 dy-2, dy-2);
	cairo_fill(cr);
      }

      if(hg->allsky_last_i>=2){
	
	cairo_set_font_size (cr, dy-2);
	cairo_set_source_rgba(cr, 0, 0.5, 0, 0.5);
	cairo_move_to(cr,x0+2+dy,y0-1+dy);
	sprintf(tmp,"Old [%dmin]",hg->allsky_last_time);
	cairo_show_text(cr, tmp);

	cairo_move_to(cr,x0+2+dy,y0-1+dy*(gdouble)(hg->allsky_last_i));
	cairo_show_text(cr, "New");
      }

    }


    // Moon
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_set_font_size (cr, 12.0);

    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"RA=%02d:%02d:%04.1lf Dec=%+03d:%02d:%04.1lf",
	      hg->moon.s_ra.hours,hg->moon.s_ra.minutes,hg->moon.s_ra.seconds,
	      hg->moon.s_dec.neg==1 ? 
	        -hg->moon.s_dec.degrees : hg->moon.s_dec.degrees,
	      hg->moon.s_dec.minutes,hg->moon.s_dec.seconds);
    }
    else{
      sprintf(tmp,"RA=%02d:%02d:%04.1lf Dec=%+03d:%02d:%04.1lf",
	      hg->moon.c_ra.hours,hg->moon.c_ra.minutes,hg->moon.c_ra.seconds,
	      hg->moon.c_dec.neg==1 ? 
	        -hg->moon.c_dec.degrees : hg->moon.c_dec.degrees,
	      hg->moon.c_dec.minutes,hg->moon.c_dec.seconds);
    }
    cairo_move_to(cr,10,height-e_h);
    cairo_show_text(cr, tmp);
    
    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"Illum=%4.1f%%",hg->moon.s_disk*100);
    }
    else{
      sprintf(tmp,"Illum=%4.1f%%",hg->moon.c_disk*100);
    }
    cairo_move_to(cr,10,height-e_h*2-5);
    cairo_show_text(cr, tmp);

    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"Set=%02d:%02d",
	      hg->moon.s_set.hours,hg->moon.s_set.minutes);
    }
    else{
      sprintf(tmp,"Set=%02d:%02d",
	      hg->moon.c_set.hours,hg->moon.c_set.minutes);
    }
    cairo_move_to(cr,10,height-e_h*3-10);
    cairo_show_text(cr, tmp);

    if(hg->skymon_mode==SKYMON_SET){
      sprintf(tmp,"Rise=%02d:%02d",
	      hg->moon.s_rise.hours,hg->moon.s_rise.minutes);
    }
    else{
      sprintf(tmp,"Rise=%02d:%02d",
	      hg->moon.c_rise.hours,hg->moon.c_rise.minutes);
    }
    cairo_move_to(cr,10,height-e_h*4-15);
    cairo_show_text(cr, tmp);

    cairo_move_to(cr,5,height-e_h*5-20);
    cairo_show_text(cr, "Moon");

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 12.0);
    cairo_text_extents (cr, "!!!", &extents);
    y_br=extents.height;

    if(hg->skymon_mode==SKYMON_SET){
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_set_font_size (cr, 12.0);
      cairo_text_extents (cr, "!!! NOT current condition !!!", &extents);
      cairo_move_to(cr,width-extents.width-10,y_br+4);
      cairo_show_text(cr, "!!! NOT current condition !!!");
    }
    else if(hg->skymon_mode==SKYMON_LAST){
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_set_font_size (cr, 12.0);
      cairo_text_extents (cr, "!!! Recent sky condition !!!", &extents);
      cairo_move_to(cr,width-extents.width-10,y_br+4);
      cairo_show_text(cr, "!!! Recent sky condition !!!");
    }
#ifdef USE_XMLRPC
    else if(hg->telstat_error){
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (cr, 12.0);
      cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
      cairo_text_extents (cr, "!!! TelStat Stopped by TimeOut !!!", &extents);
      cairo_move_to(cr,width-extents.width-10,y_br+4);
      cairo_show_text(cr, "!!! TelStat Stopped by TimeOut !!!");
    }
    /*
      else if(work_page){
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 1.0);
      cairo_set_font_size (cr, 12.0);
      cairo_text_extents (cr, "Monitoring TelStat", &extents);
      cairo_move_to(cr,w-extents.width-10,extents.height+4);
      cairo_show_text(cr, "Monitoring TelStat");
      }
    */
#endif

   

    if(hg->skymon_mode==SKYMON_SET){
      from_set=(hour>=24 ? 
		(hour-24)*60+min-hg->sun.s_set.hours*60-hg->sun.s_set.minutes :
		hour*60+min-hg->sun.s_set.hours*60-hg->sun.s_set.minutes);
      min_tw06s=hg->atw06.s_set.hours*60+hg->atw06.s_set.minutes
	-hg->sun.s_set.hours*60-hg->sun.s_set.minutes;
      min_tw12s=hg->atw12.s_set.hours*60+hg->atw12.s_set.minutes
	-hg->sun.s_set.hours*60-hg->sun.s_set.minutes;
      min_tw18s=hg->atw18.s_set.hours*60+hg->atw18.s_set.minutes
	-hg->sun.s_set.hours*60-hg->sun.s_set.minutes;
      to_rise=(hour>=24 ? 
	       hg->sun.s_rise.hours*60+hg->sun.s_rise.minutes-(hour-24)*60-min :
	       hg->sun.s_rise.hours*60+hg->sun.s_rise.minutes-(hour)*60-min);
      min_tw06r=hg->sun.s_rise.hours*60+hg->sun.s_rise.minutes
	-hg->atw06.s_rise.hours*60-hg->atw06.s_rise.minutes;
      min_tw12r=hg->sun.s_rise.hours*60+hg->sun.s_rise.minutes
	-hg->atw12.s_rise.hours*60-hg->atw12.s_rise.minutes;
      min_tw18r=hg->sun.s_rise.hours*60+hg->sun.s_rise.minutes
	-hg->atw18.s_rise.hours*60-hg->atw18.s_rise.minutes;
    }
    else{
      from_set=(hour>=24 ? 
		(hour-24)*60+min-hg->sun.c_set.hours*60-hg->sun.c_set.minutes :
		hour*60+min-hg->sun.c_set.hours*60-hg->sun.c_set.minutes);
      min_tw06s=hg->atw06.c_set.hours*60+hg->atw06.c_set.minutes
	-hg->sun.c_set.hours*60-hg->sun.c_set.minutes;
      min_tw12s=hg->atw12.c_set.hours*60+hg->atw12.c_set.minutes
	-hg->sun.c_set.hours*60-hg->sun.c_set.minutes;
      min_tw18s=hg->atw18.c_set.hours*60+hg->atw18.c_set.minutes
	-hg->sun.c_set.hours*60-hg->sun.c_set.minutes;
      to_rise=(hour>=24 ? 
	       hg->sun.c_rise.hours*60+hg->sun.c_rise.minutes-(hour-24)*60-min :
	       hg->sun.c_rise.hours*60+hg->sun.c_rise.minutes-(hour)*60-min);
      min_tw06r=hg->sun.c_rise.hours*60+hg->sun.c_rise.minutes
	-hg->atw06.c_rise.hours*60-hg->atw06.c_rise.minutes;
      min_tw12r=hg->sun.c_rise.hours*60+hg->sun.c_rise.minutes
	-hg->atw12.c_rise.hours*60-hg->atw12.c_rise.minutes;
      min_tw18r=hg->sun.c_rise.hours*60+hg->sun.c_rise.minutes
	-hg->atw18.c_rise.hours*60-hg->atw18.c_rise.minutes;
    }
  
    if(hg->skymon_mode!=SKYMON_LAST){
      if((from_set<0)&&(to_rise<0)){ 
	if(!hg->noobj_flag){
	  cairo_text_extents_t extents2;
	  
	  cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				  CAIRO_FONT_WEIGHT_BOLD);
	  cairo_set_source_rgba(cr, 0.7, 0.7, 1.0, 0.7);
	  cairo_set_font_size (cr, 80.0);
	  cairo_text_extents (cr, "Daytime", &extents);
	  x = width / 2 -extents.width/2;
	  y = height /2 -(extents.height/2 + extents.y_bearing);
	  cairo_move_to(cr, x, y);
	  cairo_show_text(cr, "Daytime");
	  
	  cairo_set_font_size (cr, 15.0);
	  cairo_text_extents (cr, "Have a good sleep...", &extents2);
	  x = width / 2 +extents.width/2 -extents2.width;
	  y = height /2 + (extents.height/2 ) + (extents2.height) +5;
	  cairo_move_to(cr, x, y);
	  cairo_show_text(cr, "Have a good sleep...");
	}
      }
      else if((from_set>0)&&(from_set<min_tw18s)){
	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 12.0);
	if(from_set<min_tw06s){
	  cairo_set_source_rgba(cr, 0.8, 0.6, 1.0, 1.0);
	  sprintf(tmp,"[Civil Twilight]");
	}
	else if(from_set<min_tw12s){
	  cairo_set_source_rgba(cr, 0.6, 0.4, 0.8, 1.0);
	  sprintf(tmp,"[Nautical Twilight]");
	}
	else{
	  cairo_set_source_rgba(cr, 0.4, 0.2, 0.6, 1.0);
	  sprintf(tmp,"[Astronomical Twilight]");
	}
	cairo_text_extents (cr, tmp, &extents);
	cairo_move_to(cr,width-extents.width-10,y_br*2+4+5);
	cairo_show_text(cr, tmp);

	sprintf(tmp,"%02dmin after SunSet",from_set);
	cairo_text_extents (cr, tmp, &extents);
	cairo_move_to(cr,width-extents.width-10.,y_br*3+4+10);
	cairo_show_text(cr, tmp);
      }
      else if((to_rise>0)&&(to_rise<min_tw18r)){
	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 12.0);
	if(to_rise<min_tw06r){
	cairo_set_source_rgba(cr, 0.8, 0.6, 1.0, 1.0);
	  sprintf(tmp,"[Civil Twilight]");
	}
	else if(to_rise<min_tw12r){
	  cairo_set_source_rgba(cr, 0.6, 0.4, 0.8, 1.0);
	  sprintf(tmp,"[Nautical Twilight]");
	}
	else{
	  cairo_set_source_rgba(cr, 0.4, 0.2, 0.6, 1.0);
	  sprintf(tmp,"[Astronomical Twilight]");
	}
	cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,width-extents.width-10,y_br*2+4+5);
	cairo_show_text(cr, tmp);

	sprintf(tmp,"%02dmin before SunRise",to_rise);
	cairo_text_extents (cr, tmp, &extents);
	cairo_move_to(cr,width-extents.width-10,y_br*3+4+10);
	cairo_show_text(cr, tmp);
      }
    }
    else{
      if(hg->allsky_last_frame==hg->allsky_last_i-1){

	cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
	cairo_set_font_size (cr, 10.0);
	cairo_text_extents (cr, "Latest Sky", &extents);
	cairo_move_to(cr, 
		      (width<height ? width*0.05 : width/2-height/2*0.9)+5,
		      height/2-5);
	cairo_show_text(cr, "Latest Sky");
      }
    }

    // AllSky
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_set_font_size (cr, 12.0);

    cairo_text_extents (cr, hg->allsky_date, &extents);
    cairo_move_to(cr,width-extents.width-5,height-e_h);
    if(hg->skymon_mode==SKYMON_LAST){
      cairo_show_text(cr, hg->allsky_last_date[hg->allsky_last_frame]);
    }
    else{
      cairo_show_text(cr, hg->allsky_date);
    }

    if(hg->allsky_cloud_show){
      gdouble e_w;
      if(hg->skymon_mode==SKYMON_CUR){
	if((hg->allsky_flag) && (hg->allsky_last_i>1)){
	  if(hg->allsky_cloud_area[hg->allsky_last_i-1]>30.){
	    cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
	  }
	  else{
	    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	  }

	  cairo_set_font_size (cr, 10.0);
	  sprintf(tmp," (x%.1lf)",hg->allsky_cloud_abs[hg->allsky_last_i-1]/
		  hg->allsky_cloud_thresh/(gdouble)hg->allsky_diff_mag);
	  cairo_text_extents (cr, tmp, &extents);
	  e_w=extents.width;
	  cairo_move_to(cr,width-e_w-5,height-e_h*4-15);
	  cairo_show_text(cr, tmp);

	  cairo_set_font_size (cr, 12.0);
	  sprintf(tmp,"CC=%04.1lf%%",hg->allsky_cloud_area[hg->allsky_last_i-1]);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,width-extents.width-e_w-5,height-e_h*4-15);
	  cairo_show_text(cr, tmp);

	  draw_stderr_graph(hg,cr,width,height,e_h);

	  cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	}
      }
      else if(hg->skymon_mode==SKYMON_LAST){
	if((hg->allsky_flag) && (hg->allsky_last_i>0)){
	  if(hg->allsky_cloud_area[hg->allsky_last_frame]>30.){
	    cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
	  }
	  else{
	    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	  }

	  cairo_set_font_size (cr, 10.0);
	  sprintf(tmp," (x%.1lf)",hg->allsky_cloud_abs[hg->allsky_last_frame]/
		  hg->allsky_cloud_thresh/(gdouble)hg->allsky_diff_mag);
	  cairo_text_extents (cr, tmp, &extents);
	  e_w=extents.width;
	  cairo_move_to(cr,width-e_w-5,height-e_h*4-15);
	  cairo_show_text(cr, tmp);

	  cairo_set_font_size (cr, 12.0);
	  sprintf(tmp,"CC=%04.1lf%%",hg->allsky_cloud_area[hg->allsky_last_frame]);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,width-extents.width-e_w-5,height-e_h*4-15);
	  cairo_show_text(cr, tmp);
	  
	  draw_stderr_graph(hg,cr,width,height,e_h);

	  cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	}
      }
    }


    if(hg->allsky_diff_flag){
      sprintf(tmp,"Diff: %s (%d)",hg->allsky_name,hg->allsky_last_i);
    }
    else{
      sprintf(tmp,"%s (%d)",hg->allsky_name,hg->allsky_last_i);
    }
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,width-extents.width-5,height-e_h*3-10);
    cairo_show_text(cr, tmp);

    // Time difference of All Sky Image
    if((hg->allsky_flag) && (hg->allsky_last_i>0)){
      time_t t,t0;
      double JD0;
      int ago;
      int bias;
#ifdef USE_WIN32
      TIME_ZONE_INFORMATION TimeZoneInfo;

      GetTimeZoneInformation( &TimeZoneInfo );
      bias=TimeZoneInfo.Bias;
#else
      struct timeval tv;
      struct timezone tz;

      gettimeofday (&tv, &tz);
      bias=tz.tz_minuteswest;
#endif
      JD0=ln_get_julian_from_sys();
      ln_get_timet_from_julian (JD0, &t0);
      t0+=(bias*60+hg->obs_timezone*3600);
      
      if(hg->skymon_mode==SKYMON_CUR){
	if(hg->allsky_last_t[hg->allsky_last_i-1]>0){
	  ago=(t0-hg->allsky_last_t[hg->allsky_last_i-1])/60;
	  if(ago>30){
	    cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
	  }
	  else{
	    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	  }
	  
	  if((hg->allsky_diff_flag)&&(hg->allsky_last_i>1)){
	    gint ago0=(t0-hg->allsky_last_t[hg->allsky_last_i-2])/60;
	    sprintf(tmp,"[%dmin ago] - [%dmin ago]",ago,ago0);
	  }
	  else{
	    sprintf(tmp,"%dmin ago",ago);
	  }
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,width-extents.width-5,height-e_h*2-5);
	  cairo_show_text(cr, tmp);
	}
	
      }
      else if(hg->skymon_mode==SKYMON_LAST){
	if(hg->allsky_last_t[hg->allsky_last_frame]>0){
	  ago=(t0-hg->allsky_last_t[hg->allsky_last_frame])/60;

	  cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	  if((hg->allsky_diff_flag)&&(hg->allsky_last_frame>0)){
	    gint ago0=(t0-hg->allsky_last_t[hg->allsky_last_frame-1])/60;
	    sprintf(tmp,"[%dmin ago] - [%dmin ago]",ago,ago0);
	  }
	  else{
	    sprintf(tmp,"%dmin ago",ago);
	  }
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,width-extents.width-5,height-e_h*2-5);
	  cairo_show_text(cr, tmp);
	}
      }

    }
    
    
  }
  

  if(!hg->noobj_flag){
    // Moon
    if(hg->skymon_mode==SKYMON_SET){
	my_cairo_moon(cr,width,height,
		      hg->moon.s_az,hg->moon.s_el,hg->moon.s_disk);
	my_cairo_sun(cr,width,height,
		     hg->sun.s_az,hg->sun.s_el);

	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->mercury.s_az,hg->mercury.s_el,
			hg->mercury.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->venus.s_az,hg->venus.s_el,
			hg->venus.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->mars.s_az,hg->mars.s_el,
			hg->mars.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->jupiter.s_az,hg->jupiter.s_el,
			hg->jupiter.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->saturn.s_az,hg->saturn.s_el,
			hg->saturn.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->uranus.s_az,hg->uranus.s_el,
			hg->uranus.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->neptune.s_az,hg->neptune.s_el,
			hg->neptune.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->pluto.s_az,hg->pluto.s_el,
			hg->pluto.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
    }
    else if(hg->skymon_mode==SKYMON_CUR){
	my_cairo_moon(cr,width,height,
		      hg->moon.c_az,hg->moon.c_el,hg->moon.c_disk);
	my_cairo_sun(cr,width,height,
		     hg->sun.c_az,hg->sun.c_el);

	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->mercury.c_az,hg->mercury.c_el,
			hg->mercury.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->venus.c_az,hg->venus.c_el,
			hg->venus.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->mars.c_az,hg->mars.c_el,
			hg->mars.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->jupiter.c_az,hg->jupiter.c_el,
			hg->jupiter.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->saturn.c_az,hg->saturn.c_el,
			hg->saturn.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->uranus.c_az,hg->uranus.c_el,
			hg->uranus.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->neptune.c_az,hg->neptune.c_el,
			hg->neptune.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
	my_cairo_planet(cr,hg->fontfamily,width,height,
			hg->pluto.c_az,hg->pluto.c_el,
			hg->pluto.name, as_flag, hg->skymon_objsz,
			hg->size_edge);
    }
    
    
    // Object
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    if(hg->skymon_mode==SKYMON_SET){
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].s_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object(cr,hg->fontfamily,width,height,
			  hg->obj[i_list].s_az,hg->obj[i_list].s_el,
			  hg->obj[i_list].name, hg->obj[i_list].check_sm,
			  hg->obj[i_list].check_lock,
			  hg->obj[i_list].check_std,
			  as_flag, hg->skymon_objsz,
			  hg->col[hg->obj[i_list].ope],
			  hg->col_edge,hg->alpha_edge,hg->size_edge,
			  &hg->obj[i_list].x,&hg->obj[i_list].y);
	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].s_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object2(cr,width,height,
			   hg->obj[i_list].s_az,hg->obj[i_list].s_el,
			   hg->obj[i_list].name, hg->obj[i_list].check_sm,
			   hg->obj[i_list].check_lock,
			   hg->col[hg->obj[i_list].ope],
			   &hg->obj[i_list].x,&hg->obj[i_list].y);
	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].s_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object3(cr,hg->fontfamily,width,height,
			   hg->obj[i_list].s_az,hg->obj[i_list].s_el,
			   hg->obj[i_list].name, hg->obj[i_list].check_sm,
			   hg->obj[i_list].check_lock,
			   hg->skymon_objsz,hg->size_edge,
			   &hg->obj[i_list].x,&hg->obj[i_list].y);

	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      if((hg->stddb_flag)&&(hg->std_i==hg->tree_focus)){
	for(i_list=0;i_list<hg->std_i_max;i_list++){
	  if(hg->std[i_list].s_el>0){
	    if(hg->stddb_tree_focus!=i_list){
	      my_cairo_std(cr,width,height,
			   hg->std[i_list].s_az,hg->std[i_list].s_el,
			   &hg->std[i_list].x,&hg->std[i_list].y);
	    }
	  }
	  else{
	    hg->std[i_list].x=-1;
	    hg->std[i_list].y=-1;
	  }
	}
	for(i_list=0;i_list<hg->std_i_max;i_list++){
	  if(hg->std[i_list].s_el>0){
	    if(hg->stddb_tree_focus==i_list){
	      my_cairo_std2(cr,width,height,
			    hg->std[i_list].s_az,hg->std[i_list].s_el,
			    &hg->std[i_list].x,&hg->std[i_list].y);
	    }
	  }
	  else{
	    hg->std[i_list].x=-1;
	    hg->std[i_list].y=-1;
	  }
	}
      }
    }
    else  if(hg->skymon_mode==SKYMON_CUR){
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].c_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object(cr,hg->fontfamily,width,height,
			  hg->obj[i_list].c_az,hg->obj[i_list].c_el,
			  hg->obj[i_list].name, hg->obj[i_list].check_sm, 
			  hg->obj[i_list].check_lock,
			  hg->obj[i_list].check_std,
			  as_flag, hg->skymon_objsz,
			  hg->col[hg->obj[i_list].ope],
			  hg->col_edge,hg->alpha_edge,hg->size_edge,
			  &hg->obj[i_list].x,&hg->obj[i_list].y);
	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].c_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object2(cr,width,height,
			   hg->obj[i_list].c_az,hg->obj[i_list].c_el,
			   hg->obj[i_list].name, hg->obj[i_list].check_sm,
			   hg->obj[i_list].check_lock,
			   hg->col[hg->obj[i_list].ope],
			   &hg->obj[i_list].x,&hg->obj[i_list].y);
	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].c_el>0) && (hg->obj[i_list].check_disp)){
	  my_cairo_object3(cr,hg->fontfamily,width,height,
			   hg->obj[i_list].c_az,hg->obj[i_list].c_el,
			   hg->obj[i_list].name, hg->obj[i_list].check_sm,
			   hg->obj[i_list].check_lock,
			   hg->skymon_objsz,hg->size_edge,
			   &hg->obj[i_list].x,&hg->obj[i_list].y);
	}
	else{
	  hg->obj[i_list].x=-1;
	  hg->obj[i_list].y=-1;
	}
      }
      if((hg->stddb_flag)&&(hg->std_i==hg->tree_focus)){
	for(i_list=0;i_list<hg->std_i_max;i_list++){
	  if(hg->std[i_list].c_el>0){
	    if(hg->stddb_tree_focus!=i_list){
	      my_cairo_std(cr,width,height,
			   hg->std[i_list].c_az,hg->std[i_list].c_el,
			   &hg->std[i_list].x,&hg->std[i_list].y);
	    }
	  }
	  else{
	    hg->std[i_list].x=-1;
	    hg->std[i_list].y=-1;
	  }
	}
	for(i_list=0;i_list<hg->std_i_max;i_list++){
	  if(hg->std[i_list].c_el>0){
	    if(hg->stddb_tree_focus==i_list){
	      my_cairo_std2(cr,width,height,
			    hg->std[i_list].c_az,hg->std[i_list].c_el,
			    &hg->std[i_list].x,&hg->std[i_list].y);
	    }
	  }
	  else{
	    hg->std[i_list].x=-1;
	    hg->std[i_list].y=-1;
	  }
	}
      }
    }
  }

  cairo_destroy(cr);

#ifdef USE_XMLRPC
  if(!hg->telstat_flag){
    gdk_window_set_back_pixmap(widget->window,
			       hg->pixmap_skymon,
			       FALSE);
  
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      hg->pixmap_skymon,
		      0,0,0,0,
		      width,
		      height);
  }
#else
  gdk_window_set_back_pixmap(widget->window,
			     hg->pixmap_skymon,
			     FALSE);
  
  gdk_draw_drawable(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		    hg->pixmap_skymon,
		    0,0,0,0,
		    width,
		    height);
#endif

  if(hg->skymon_mode==SKYMON_LAST){
    if(hg->allsky_last_i>=2){
      if(hg->allsky_last_frame>=hg->allsky_last_i-1){
	hg->allsky_last_repeat++;
	if(hg->allsky_last_repeat>=4){
	  hg->allsky_last_repeat=0;
	  hg->allsky_last_frame=0;
	}
      }
      else{
	hg->allsky_last_frame++;
      }
    }
    else{
      hg->allsky_last_frame++;

      if(hg->allsky_last_frame>=hg->allsky_last_i){
	hg->allsky_last_frame=0;
      }
    }

  }

  flagDrawing=FALSE;
  skymon_debug_print("Finishing draw_skymon_cairo\n");
  return TRUE;
}

#ifdef USE_XMLRPC
gboolean draw_skymon_with_telstat_cairo(GtkWidget *widget, 
					typHOE *hg){

  cairo_t *cr;
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  gint from_set, to_rise;
  gint w=0,h=0;
  gdouble r=1.0;
  gint off_x, off_y;
  GdkPixmap *pixmap_skymon_with_telstat=NULL;
  gboolean as_flag=FALSE;
  gchar tmp[64];

  if(!flagSkymon) return (FALSE);

  if(!hg->telstat_flag) return (FALSE);

  skymon_debug_print("Starting draw_skymon_cairo\n");

  int width, height;
  width= widget->allocation.width;
  height= widget->allocation.height;
  if(width<=1){
    gtk_window_get_size(GTK_WINDOW(hg->skymon_main), &width, &height);
  }

  if(hg->skymon_mode==SKYMON_CUR){
    pixmap_skymon_with_telstat = gdk_pixmap_new(widget->window,
						width,
						height,
						-1);
    

    if(hg->pixmap_skymon) 
      gdk_draw_drawable(pixmap_skymon_with_telstat,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			hg->pixmap_skymon,
			0,0,0,0,
			width,
			height);
    else
      return(FALSE);
      

    as_flag=hg->allsky_flag;
    
    cr = gdk_cairo_create(pixmap_skymon_with_telstat);

    cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    
    if(hg->skymon_mode==SKYMON_CUR) as_flag=hg->allsky_flag;

    work_page^=1;
    my_cairo_telescope_path(cr,hg->fontfamily,width,height,
			    hg->stat_az,hg->stat_el, 
			    hg->stat_az_cmd,hg->stat_el_cmd, 
			    hg->stat_fixflag);
    my_cairo_telescope_cmd(cr,hg->fontfamily,width,height,
			   hg->stat_az_cmd,hg->stat_el_cmd, 
			   as_flag, 
			   hg->skymon_objsz,hg->size_edge);
    my_cairo_telescope(cr,hg->fontfamily,width,height,
		       hg->stat_az,hg->stat_el, 
		       as_flag, 
		       hg->stat_fixflag, hg->skymon_objsz,
		       hg->size_edge);

    if(!hg->stat_fixflag){
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_set_font_size (cr, 12.0);

      if(hg->stat_reachtime>60){
	sprintf(tmp,"%02dmin %02dsec to reach",
		(gint)(hg->stat_reachtime/60),
		((gint)hg->stat_reachtime%60));
      }
      else{
	sprintf(tmp,"%02dsec to reach",
		(gint)(hg->stat_reachtime));
      }

      cairo_text_extents (cr, tmp, &extents);
      cairo_move_to(cr,width-extents.width-10,extents.height*4+4+5+5+5);
      cairo_show_text(cr, tmp);
    }

    cairo_destroy(cr);


    gdk_window_set_back_pixmap(widget->window,
			       pixmap_skymon_with_telstat,
			       FALSE);
  
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      pixmap_skymon_with_telstat,
		      0,0,0,0,
		      width,
		      height);
    
    g_object_unref(G_OBJECT(pixmap_skymon_with_telstat));
  }
  else{
    gdk_window_set_back_pixmap(widget->window,
			       hg->pixmap_skymon,
			       FALSE);
  
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      hg->pixmap_skymon,
		      0,0,0,0,
		      width,
		      height);

  }

  skymon_debug_print("Finishing draw_skymon_with_telstat_cairo\n");
  return TRUE;
}
#endif
  


void my_cairo_arc_center(cairo_t *cr, gint w, gint h, gdouble r){
  cairo_arc(cr, 
	    w / 2, h / 2, 
	    (w < h ? w : h) / 2 * ((90. - r)/100.) , 
	    0, 2 * M_PI);
  //cairo_fill(cr);
}

void my_cairo_arc_center2(cairo_t *cr, gint w, gint h, gint x, gint y, gdouble r){
  cairo_arc(cr, 
	    w / 2 + x, h / 2 + y, 
	    (w < h ? w : h) / 2 * ((90. - r)/100.) , 
	    0, 2 * M_PI);
  //cairo_fill(cr);
}

void my_cairo_arc_center_path(cairo_t *cr, gint w, gint h){
  gdouble r=-5;

  cairo_arc(cr, 
	    w / 2, h / 2, 
	    (w < h ? w : h) / 2 * ((90. - r)/100.) , 
	    0, 2 * M_PI);
  //cairo_fill(cr);
}

// Normal
void my_cairo_object(cairo_t *cr, gchar* fontname, gint w, gint h, gdouble az, gdouble el, gchar *name, gboolean check_sm, gboolean check_lock, gboolean check_std, gboolean allsky_flag, gint sz, GdkColor *col,GdkColor *col_edge,gint alpha_edge, gint size_edge, gdouble *objx, gdouble *objy){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if(check_sm||check_lock) return;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  *objx=x;
  *objy=y;

  cairo_new_path(cr);

  if(allsky_flag){
    cairo_set_source_rgba(cr, 
			  (gdouble)col_edge->red/0x10000, 
			  (gdouble)col_edge->green/0x10000,
			  (gdouble)col_edge->blue/0x10000, 
			  (gdouble)alpha_edge/0x10000);
    if(check_std){
      cairo_arc(cr, x, y, 4, 0, 2*M_PI);
    }
    else{
      cairo_arc(cr, x, y, 5, 0, 2*M_PI);
    }
    cairo_fill(cr);
    cairo_new_path(cr);
  }

  cairo_set_source_rgba(cr, (gdouble)col->red/0x10000, 
			(gdouble)col->green/0x10000, 
			(gdouble)col->blue/0x10000, 1.0);
  if(check_std){
    //cairo_set_source_rgba(cr, 0.4, 0.4, 0.1, 1.0);
    cairo_arc(cr, x, y, 2, 0, 2*M_PI);
  }
  else{
    //cairo_set_source_rgba(cr, 0.2, 0.4, 0.1, 1.0);
    cairo_arc(cr, x, y, 3, 0, 2*M_PI);
  }
  cairo_fill(cr);

  if(sz>0){
    cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    if(check_std)
      cairo_set_font_size (cr, (gdouble)sz*.8);
    else
      cairo_set_font_size (cr, (gdouble)sz);
    cairo_text_extents (cr, name, &extents);
    if(allsky_flag){
      cairo_move_to(cr,
		    x-(extents.width/2 + extents.x_bearing),
		    y-5);
      //cairo_show_text(cr, name);
      cairo_text_path(cr, name);
      cairo_set_source_rgba(cr, 
			    (gdouble)col_edge->red/0x10000, 
			    (gdouble)col_edge->green/0x10000,
			    (gdouble)col_edge->blue/0x10000, 
			    (gdouble)alpha_edge/0x10000);
      cairo_set_line_width(cr, (double)size_edge);
      cairo_stroke(cr);
      
      cairo_new_path(cr);
    }
    
    /*
    if(check_std){
      cairo_set_source_rgba(cr, 0.4, 0.4, 0.1, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.2, 0.4, 0.1, 1.0);
     }
    */
    cairo_set_source_rgba(cr, (gdouble)col->red/0x10000, 
			  (gdouble)col->green/0x10000, (gdouble)col->blue/0x10000, 1.0);
    cairo_move_to(cr,
		  x-(extents.width/2 + extents.x_bearing),
		  y-5);
    cairo_show_text(cr, name);
  }

}

// High-ligted
void my_cairo_object2(cairo_t *cr, gint w, gint h, gdouble az, gdouble el, gchar *name, gboolean check_sm, gboolean check_lock, GdkColor *col, gdouble *objx, gdouble *objy){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if((!check_sm)&&(!check_lock)) return;
   
  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  *objx=x;
  *objy=y;

  cairo_new_path(cr);

  if(check_lock){
    cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.6);
    cairo_arc(cr, x, y, 10, 0, 2*M_PI);
  }
  else{
    cairo_set_source_rgba(cr, 1.0, 0.75, 0.5, 0.6);
    cairo_arc(cr, x, y, 8, 0, 2*M_PI);
  }
  cairo_fill(cr);
  cairo_new_path(cr);

  cairo_set_source_rgba(cr, (gdouble)col->red/0x10000, 
			(gdouble)col->green/0x10000, (gdouble)col->blue/0x10000, 1.0);
  //cairo_set_source_rgba(cr, 0.2, 0.4, 0.1, 1.0);
  cairo_arc(cr, x, y, 3, 0, 2*M_PI);
  cairo_fill(cr);

}

// Locked
void my_cairo_object3(cairo_t *cr, char *fontname, gint w, gint h, gdouble az, gdouble el, gchar *name, gboolean check_sm, gboolean check_lock, gint sz, gint size_edge, gdouble *objx, gdouble *objy){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if((!check_sm)&&(!check_lock)) return;
   
  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  *objx=x;
  *objy=y;

  cairo_new_path(cr);

  if(sz>0){
    cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    if(check_lock)
      cairo_set_font_size (cr, (gdouble)sz*1.5);
    else
      cairo_set_font_size (cr, (gdouble)sz*1.3);
    cairo_text_extents (cr, name, &extents);
    cairo_move_to(cr,
		  x-(extents.width/2 + extents.x_bearing),
		  y-15);
    //cairo_show_text(cr, name);
    cairo_text_path(cr, name);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);
    cairo_set_line_width(cr, (double)size_edge*1.5);
    cairo_stroke(cr);
    
    cairo_new_path(cr);
    cairo_move_to(cr,
		  x-(extents.width/2 + extents.x_bearing),
		  y-15);
    if(check_lock)
      cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 1.0);
    else
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.2, 1.0);
    cairo_show_text(cr, name);
  }
}


void my_cairo_std(cairo_t *cr, gint w, gint h, gdouble az, gdouble el, gdouble *objx, gdouble *objy){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  *objx=x;
  *objy=y;

  cairo_new_path(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, 0.75);
  cairo_arc(cr, x, y, 5, 0, 2*M_PI);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1, 1, 1, 1.0);
  cairo_arc(cr, x, y, 3, 0, 2*M_PI);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 0.9, 0.25, 0.9, 1.0);
  cairo_arc(cr, x, y, 3, 0, 2*M_PI);
  cairo_set_line_width (cr, 1.5);
  cairo_stroke(cr);

  cairo_new_path(cr);
}


void my_cairo_std2(cairo_t *cr, gint w, gint h, gdouble az, gdouble el, gdouble *objx, gdouble *objy){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  *objx=x;
  *objy=y;

  cairo_new_path(cr);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.75);
  cairo_arc(cr, x, y, 11, 0, 2*M_PI);
  cairo_fill(cr);
  
  cairo_rectangle (cr, x-10, y-10, 20, 20);
  cairo_fill(cr);
  /*
  cairo_move_to (cr, x-10, y-10);
  cairo_line_to (cr, x-4.5, y-4.5);
  cairo_move_to (cr, x+10, y-10);
  cairo_line_to (cr, x+4.5, y-4.5);
  cairo_move_to (cr, x+10, y+10);
  cairo_line_to (cr, x+4.5, y+4.5);
  cairo_move_to (cr, x-10, y+10);
  cairo_line_to (cr, x-4.5, y+4.5);
  cairo_set_line_width (cr, 4);
  cairo_stroke (cr);
  */

  /*
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
  cairo_arc(cr, x, y, 8, 0, 2*M_PI);
  cairo_fill(cr);
  */
  cairo_set_source_rgba(cr, 1.0, 0.25, 0.25, 1.0);
  cairo_move_to (cr, x-8, y-8);
  cairo_line_to (cr, x-4, y-8);
  cairo_move_to (cr, x-8, y-8);
  cairo_line_to (cr, x-8, y-4);

  cairo_move_to (cr, x+8, y-8);
  cairo_line_to (cr, x+4, y-8);
  cairo_move_to (cr, x+8, y-8);
  cairo_line_to (cr, x+8, y-4);

  cairo_move_to (cr, x+8, y+8);
  cairo_line_to (cr, x+4, y+8);
  cairo_move_to (cr, x+8, y+8);
  cairo_line_to (cr, x+8, y+4);

  cairo_move_to (cr, x-8, y+8);
  cairo_line_to (cr, x-4, y+8);
  cairo_move_to (cr, x-8, y+8);
  cairo_line_to (cr, x-8, y+4);
  cairo_set_line_width (cr, 2.5);
  cairo_stroke (cr);


  cairo_set_source_rgba(cr, 1.0, 0.1, 0.1, 1.0);
  cairo_arc(cr, x, y, 4.5, 0, 2*M_PI);
  cairo_fill(cr);
  
  /*
  cairo_set_source_rgba(cr, 1.0, 0.25, 0.25, 1.0);
  cairo_arc(cr, x, y, 8, 0, 2*M_PI);
  cairo_set_line_width (cr, 1.8);
  cairo_stroke (cr);

  cairo_move_to (cr, x-8, y-8);
  cairo_line_to (cr, x-4.5, y-4.5);
  cairo_move_to (cr, x+8, y-8);
  cairo_line_to (cr, x+4.5, y-4.5);
  cairo_move_to (cr, x+8, y+8);
  cairo_line_to (cr, x+4.5, y+4.5);
  cairo_move_to (cr, x-8, y+8);
  cairo_line_to (cr, x-4.5, y+4.5);
  cairo_set_line_width (cr, 1.8);
  cairo_stroke (cr);
  */
  cairo_new_path(cr);
}


void my_cairo_moon(cairo_t *cr, gint w, gint h, gdouble az, gdouble el, gdouble s_disk){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if(el<=0) return;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  cairo_new_path(cr);

  cairo_set_source_rgba(cr, 0.7, 0.7, 0.0, 1.0);
  cairo_arc(cr, x, y, 11, 0, 2*M_PI);
  cairo_fill(cr);

  if(s_disk>=1){
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
    cairo_arc(cr, x, y, 10, 0, 2*M_PI);
    cairo_fill(cr);
  }
  else if(s_disk>0.0){
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
    cairo_arc(cr, x, y, 10, -M_PI/2, M_PI/2);
    cairo_fill(cr);
    
    if(s_disk>0.5){
      cairo_save (cr);
      cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
      cairo_translate (cr, x, y);
      cairo_scale (cr, (s_disk-0.5)*2.*10., 10);
      cairo_arc (cr, 0.0, 0.0, 1., 0, 2*M_PI);
      cairo_fill(cr);
      cairo_restore (cr);
    }
    else if(s_disk<0.5){
      cairo_save (cr);
      cairo_set_source_rgba(cr, 0.7, 0.7, 0.0, 1.0);
      cairo_translate (cr, x, y);
      cairo_scale (cr, (0.5-s_disk)*2.*10., 10);
      cairo_arc (cr, 0.0, 0.0, 1., 0, 2*M_PI);
      cairo_fill(cr);
      cairo_restore (cr);
    }

  }

}
void my_cairo_sun(cairo_t *cr, gint w, gint h, gdouble az, gdouble el){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if(el<=0) return;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  cairo_new_path(cr);

  cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 0.2);
  cairo_arc(cr, x, y, 16, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 0.3);
  cairo_arc(cr, x, y, 13, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 0.5);
  cairo_arc(cr, x, y, 11, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 0.8);
  cairo_arc(cr, x, y, 10, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 1.0);
  cairo_arc(cr, x, y, 9, 0, 2*M_PI);
  cairo_fill(cr);

}

void my_cairo_planet(cairo_t *cr, gchar *fontname, gint w, gint h, gdouble az, gdouble el, gchar *name, gboolean allsky_flag, gint sz, gint size_edge){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  if(el<=0) return;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  cairo_new_path(cr);

  if(allsky_flag){
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
    cairo_arc(cr, x, y, 4, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_new_path(cr);
  }

  cairo_set_source_rgba(cr, 0.8, 0.4, 0.0, 1.0);
  cairo_arc(cr, x, y, 2, 0, 2*M_PI);
  cairo_fill(cr);

  if(sz>0){
    cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)sz*.8);
    cairo_text_extents (cr, name, &extents);
    if(allsky_flag){
      cairo_move_to(cr,
		    x-(extents.width/2 + extents.x_bearing),
		    y-5);
      //cairo_show_text(cr, name);
      cairo_text_path(cr, name);
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
      cairo_set_line_width(cr, (double)size_edge);
      cairo_stroke(cr);
      
      cairo_new_path(cr);
    }
    
    cairo_set_source_rgba(cr, 0.8, 0.4, 0.0, 1.0);
    cairo_move_to(cr,
		  x-(extents.width/2 + extents.x_bearing),
		  y-5);
    cairo_show_text(cr, name);
  }

}



#ifdef USE_XMLRPC
#define SKYMON_TELSIZE 8
void my_cairo_telescope(cairo_t *cr,gchar *fontname, gint w, gint h, gdouble az, gdouble el, 
			gboolean allsky_flag, 
			gboolean fix_flag, gint sz, gint size_edge){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  cairo_new_path(cr);

  if(allsky_flag){
    cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_arc(cr, x, y, 3, 0, 2*M_PI);
    cairo_fill(cr);

    cairo_arc(cr, x, y, SKYMON_TELSIZE, 0, 2*M_PI);
    cairo_set_line_width (cr, 2.5);
    cairo_stroke (cr);

    cairo_move_to (cr, x, y-SKYMON_TELSIZE*1.3);
    cairo_line_to (cr, x, y+SKYMON_TELSIZE*1.3);
    cairo_move_to (cr, x-SKYMON_TELSIZE*1.3, y);
    cairo_line_to (cr, x+SKYMON_TELSIZE*1.3, y);
    cairo_set_line_width (cr, 1.8);
    cairo_stroke (cr);
  }

  if(!work_page){
    cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.3);
    cairo_arc(cr, x, y, SKYMON_TELSIZE, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 1.0);
  }
  else{
    cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.5);
  }

  cairo_arc(cr, x, y, SKYMON_TELSIZE, 0, 2*M_PI);
  cairo_set_line_width (cr, 1.5);
  cairo_stroke (cr);

  if(!work_page){
    cairo_arc(cr, x, y, 2, 0, 2*M_PI);
    cairo_fill(cr);
  }
  else if(fix_flag){
    cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 1.0);
    cairo_arc(cr, x, y, 3, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.5);
  }

  cairo_move_to (cr, x, y-SKYMON_TELSIZE*1.3);
  cairo_line_to (cr, x, y+SKYMON_TELSIZE*1.3);
  cairo_move_to (cr, x-SKYMON_TELSIZE*1.3, y);
  cairo_line_to (cr, x+SKYMON_TELSIZE*1.3, y);
  cairo_set_line_width (cr, 0.8);
  cairo_stroke (cr);

  if(sz>0){
    cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to (cr, x, y);
    cairo_line_to (cr, x+SKYMON_TELSIZE*3, y+SKYMON_TELSIZE*3);
    cairo_line_to (cr, x+SKYMON_TELSIZE*4, y+SKYMON_TELSIZE*3);
    cairo_set_line_width (cr, 0.8);
    cairo_stroke (cr);
    
    cairo_set_font_size (cr, (gdouble)sz);
    cairo_text_extents (cr, "Telescope", &extents);
    if(allsky_flag){
      cairo_move_to(cr,
		    x+SKYMON_TELSIZE*4,
		    y-(extents.height/2+extents.y_bearing)+SKYMON_TELSIZE*3);
      cairo_text_path(cr, "Telescope");
      cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
      cairo_set_line_width(cr, (double)size_edge*0.7);
      cairo_stroke(cr);
      
      cairo_new_path(cr);
    }

    if(fix_flag&&work_page){
      cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
      cairo_move_to(cr,
		    x-SKYMON_TELSIZE*1.8,
		    y-SKYMON_TELSIZE*1.8+2);
      cairo_line_to(cr,
		    x-SKYMON_TELSIZE*1.8+5,
		    y-SKYMON_TELSIZE*1.8+5);
      cairo_line_to(cr,
		    x-SKYMON_TELSIZE*1.8+2,
		    y-SKYMON_TELSIZE*1.8);
      cairo_close_path(cr);
      cairo_fill(cr);

      cairo_move_to(cr,
		    x-SKYMON_TELSIZE*1.8,
		    y+SKYMON_TELSIZE*1.8-2);
      cairo_line_to(cr,
		    x-SKYMON_TELSIZE*1.8+5,
		    y+SKYMON_TELSIZE*1.8-5);
      cairo_line_to(cr,
		    x-SKYMON_TELSIZE*1.8+2,
		    y+SKYMON_TELSIZE*1.8);
      cairo_close_path(cr);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
      cairo_move_to(cr,
		    x+SKYMON_TELSIZE*1.8,
		    y-SKYMON_TELSIZE*1.8+2);
      cairo_line_to(cr,
		    x+SKYMON_TELSIZE*1.8-5,
		    y-SKYMON_TELSIZE*1.8+5);
      cairo_line_to(cr,
		    x+SKYMON_TELSIZE*1.8-2,
		    y-SKYMON_TELSIZE*1.8);
      cairo_close_path(cr);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
      cairo_move_to(cr,
		    x+SKYMON_TELSIZE*1.8,
		    y+SKYMON_TELSIZE*1.8-2);
      cairo_line_to(cr,
		    x+SKYMON_TELSIZE*1.8-5,
		    y+SKYMON_TELSIZE*1.8-5);
      cairo_line_to(cr,
		    x+SKYMON_TELSIZE*1.8-2,
		    y+SKYMON_TELSIZE*1.8);
      cairo_close_path(cr);
      cairo_fill(cr);

    }

    if(work_page){
      cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.5);
    }
    else{
      cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 1.0);
    }
    cairo_move_to(cr,
		  x+SKYMON_TELSIZE*4,
		  y-(extents.height/2+extents.y_bearing)+SKYMON_TELSIZE*3);
    cairo_show_text(cr, "Telescope");
  }

}


void my_cairo_telescope_path(cairo_t *cr, gchar *fontname, gint w, gint h, 
			     gdouble az, gdouble el, 
			     gdouble az_cmd, gdouble el_cmd, 
			     gboolean fix_flag){
  gdouble r;
  gdouble dir_az=1., dir_el=1.;
  gboolean az_end=FALSE, el_end=FALSE;
  gdouble c_az0, c_az1, c_el0, c_el1, dAz, dEl;
  gdouble x0,y0,el_r0,x1,y1,el_r1;
  gdouble x,y;
  cairo_text_extents_t extents;
  gchar *tmp;

  r= w<h ? w/2*0.9 : h/2*0.9;

  if(fix_flag){
    {
      cairo_save (cr);
      
      cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_source_rgba(cr, 1.0, 0, 0, 0.5);
      cairo_translate(cr,w/2,h/2);
      
      if(cos(M_PI/180.*(90-az))>0){
	cairo_rotate(cr,(-az+90)*M_PI/180.);
	
	x = (w<h ? w*0.95 : w/2+h/2*0.9)-w/2;
	
	cairo_move_to(cr, x,  0);
	cairo_line_to(cr, x+15,-2);
	cairo_line_to(cr, x+15, 2);
	cairo_close_path(cr);
	cairo_fill(cr);
	
	cairo_set_font_size (cr, 8.0);
	tmp = g_strdup_printf("%+.0lf",az);
	    cairo_text_extents (cr, tmp, &extents);
	    cairo_move_to(cr,
			  x+15+2,
			  -(extents.height/2 + extents.y_bearing));
      }
      else{
	cairo_rotate(cr,(-(az-180)+90)*M_PI/180.);
	
	x = (w<h ? w*0.05 : w/2-h/2*0.9)-w/2;
	
	cairo_move_to(cr, x,  0);
	cairo_line_to(cr, x-15,-2);
	cairo_line_to(cr, x-15, 2);
	cairo_close_path(cr);
	cairo_fill(cr);
	
	cairo_set_font_size (cr, 8.0);
	tmp = g_strdup_printf("%+.0lf",az);
	    cairo_text_extents (cr, tmp, &extents);
	    cairo_move_to(cr,
			  x-15-2-extents.width,
			  -(extents.height/2 + extents.y_bearing));
      }
      cairo_show_text(cr, tmp);
      g_free(tmp);
      
      
      cairo_restore(cr);
    }
  }
  else{
    if(az_cmd<az) dir_az=-1.;
    if(el_cmd<el) dir_el=-1.;
    
    c_az1=az;
    c_el1=el;
    
    dAz=fabs(c_az1 - az_cmd);
    dEl=fabs(c_el1 - el_cmd);

    if(dAz>0.5){
      cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.3);
      cairo_set_line_width(cr,4.0*2.0);

      if(c_az1>az_cmd){
	if(dAz>360){
	  cairo_arc(cr, 
		    w / 2, h / 2, 
		    (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
		    (-az+90.)*M_PI/180., (-az+90.+360.)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	  cairo_arc(cr, 
		    w / 2, h / 2, 
		    (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
		    (-az+90.+360.)*M_PI/180., (-az_cmd+90)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	}
	else{
	  cairo_arc(cr, 
		    w / 2, h / 2, 
		    (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
		    (-az+90.)*M_PI/180., (-az_cmd+90)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	}
      }
      else{
	if(dAz>360){
	  cairo_arc_negative(cr, 
			     w / 2, h / 2, 
			     (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
			     (-az+90.)*M_PI/180., (-az+90.-360.)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	  cairo_arc_negative(cr, 
			     w / 2, h / 2, 
			     (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
			     (-az+90.-360.)*M_PI/180., (-az_cmd+90)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	}
	else{
	  cairo_arc_negative(cr, 
			     w / 2, h / 2, 
			     (w < h ? w : h) / 2 * (90./100.) + 4.0 , 
			     (-az+90.)*M_PI/180., (-az_cmd+90)*M_PI/180.);
	  //cairo_fill(cr);
	  cairo_stroke(cr);
	}
      }

      {
	cairo_save (cr);
	cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgba(cr, 1.0, 0, 0, 1.0);
	cairo_translate(cr,w/2,h/2);

	if(cos(M_PI/180.*(90-az))>0){
	  cairo_rotate(cr,(-az+90)*M_PI/180.);
	
	  x = (w<h ? w*0.95 : w/2+h/2*0.9)-w/2;
	
	  cairo_move_to(cr, x+4*2,  0);
	  cairo_line_to(cr, x+4*2+5,-2);
	  cairo_line_to(cr, x+4*2+5, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	
	  cairo_set_font_size (cr, 10.0);
	  tmp = g_strdup_printf("%+.0lf",az);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,
			x+4*2+5+2,
			-(extents.height/2 + extents.y_bearing));
	}
	else{
	  cairo_rotate(cr,(-(az-180)+90)*M_PI/180.);
	
	  x = (w<h ? w*0.05 : w/2-h/2*0.9)-w/2;
	
	  cairo_move_to(cr, x-4*2,  0);
	  cairo_line_to(cr, x-4*2-5,-2);
	  cairo_line_to(cr, x-4*2-5, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	
	  cairo_set_font_size (cr, 10.0);
	  tmp = g_strdup_printf("%+.0lf",az);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,
			x-4*2-5-2-extents.width,
			-(extents.height/2 + extents.y_bearing));
	}
	cairo_show_text(cr, tmp);
	g_free(tmp);

	cairo_restore(cr);
      }

      {
	cairo_save (cr);
	cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgba(cr, 0.2, 0.6, 0.2, 1.0);

	cairo_translate(cr,w/2,h/2);
	if(cos(M_PI/180.*(90-az_cmd))>0){
	  cairo_rotate(cr,(-az_cmd+90)*M_PI/180.);

	  x = (w<h ? w*0.95 : w/2+h/2*0.9)-w/2;
	
	  cairo_move_to(cr, x+4*2,  0);
	  cairo_line_to(cr, x+4*2+5,-2);
	  cairo_line_to(cr, x+4*2+5, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	
	  cairo_set_font_size (cr, 10.0);
	  cairo_text_extents (cr, "Cmd.", &extents);
	  cairo_move_to(cr,
			x+4*2+5+2,
			-(extents.height/2 + extents.y_bearing));
	}
	else{
	  cairo_rotate(cr,(-(az_cmd-180)+90)*M_PI/180.);

	  x = (w<h ? w*0.05 : w/2-h/2*0.9)-w/2;
	
	  cairo_move_to(cr, x-4*2,  0);
	  cairo_line_to(cr, x-4*2-5,-2);
	  cairo_line_to(cr, x-4*2-5, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	
	  cairo_set_font_size (cr, 10.0);
	  cairo_text_extents (cr, "Cmd.", &extents);
	  cairo_move_to(cr,
			x-4*2-5-2-extents.width,
			-(extents.height/2 + extents.y_bearing));
	}
	cairo_show_text(cr, "Cmd.");
	
	cairo_restore(cr);
      }


    }
    else{
      {
	cairo_save (cr);
	
	cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgba(cr, 1.0, 0, 0, 0.5);
	cairo_translate(cr,w/2,h/2);
	
	if(cos(M_PI/180.*(90-az))>0){
	  cairo_rotate(cr,(-az+90)*M_PI/180.);
	  
	  x = (w<h ? w*0.95 : w/2+h/2*0.9)-w/2;
	  
	  cairo_move_to(cr, x,  0);
	  cairo_line_to(cr, x+15,-2);
	  cairo_line_to(cr, x+15, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	  
	  cairo_set_font_size (cr, 8.0);
	  tmp = g_strdup_printf("%+.0lf",az);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,
			x+15+2,
			-(extents.height/2 + extents.y_bearing));
	}
	else{
	  cairo_rotate(cr,(-(az-180)+90)*M_PI/180.);
	  
	  x = (w<h ? w*0.05 : w/2-h/2*0.9)-w/2;
	  
	  cairo_move_to(cr, x,  0);
	  cairo_line_to(cr, x-15,-2);
	  cairo_line_to(cr, x-15, 2);
	  cairo_close_path(cr);
	  cairo_fill(cr);
	  
	  cairo_set_font_size (cr, 8.0);
	  tmp = g_strdup_printf("%+.0lf",az);
	  cairo_text_extents (cr, tmp, &extents);
	  cairo_move_to(cr,
			x-15-2-extents.width,
			-(extents.height/2 + extents.y_bearing));
	}
	cairo_show_text(cr, tmp);
	g_free(tmp);
      
	
	cairo_restore(cr);
      }
    }
    
    if(work_page){

      cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.8);
      
      el_r1 = r * (90 - c_el1)/90;
      x1 = w/2 + el_r1*cos(M_PI/180.*(90-c_az1));
      y1 = h/2 + el_r1*sin(M_PI/180.*(90-c_az1));
      
      while((!az_end)||(!el_end)){
	c_az0=c_az1;
	
	if(dAz<0.5){
	  c_az1=az_cmd;
	  az_end=TRUE;
	}
	else{
	  c_az1+=dir_az;
	  dAz=fabs(c_az1 - az_cmd);
	}
	
	c_el0=c_el1;
	if(dEl<0.5){
	  c_el1=el_cmd;
	  el_end=TRUE;
	}
	else{
	  c_el1+=dir_el;
	  dEl=fabs(c_el1 - el_cmd);
	}
	
	x0=x1;
	y0=y1;
	
	el_r1 = r * (90 - c_el1)/90;
	x1 = w/2 + el_r1*cos(M_PI/180.*(90-c_az1));
	y1 = h/2 + el_r1*sin(M_PI/180.*(90-c_az1));
	
	cairo_move_to (cr, x0, y0);
	cairo_line_to (cr, x1, y1);
	cairo_set_line_width (cr, 4.0);
	cairo_stroke (cr);
      }
    }
  }
}


void my_cairo_telescope_cmd(cairo_t *cr, gchar *fontname, gint w, gint h, 
			    gdouble az, gdouble el, 
			    gboolean allsky_flag,
			    gint sz, gint size_edge){
  gdouble r, el_r;
  gdouble x, y;
  cairo_text_extents_t extents;

  r= w<h ? w/2*0.9 : h/2*0.9;

  el_r = r * (90 - el)/90;

  x = w/2 + el_r*cos(M_PI/180.*(90-az));
  y = h/2 + el_r*sin(M_PI/180.*(90-az));

  cairo_new_path(cr);

  if(allsky_flag){
    cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_arc(cr, x, y, 3, 0, 2*M_PI);
    cairo_fill(cr);
    
    cairo_move_to (cr, x, y-SKYMON_TELSIZE*1.8);
    cairo_line_to (cr, x, y+SKYMON_TELSIZE*1.8);
    cairo_move_to (cr, x-SKYMON_TELSIZE*1.8, y);
    cairo_line_to (cr, x+SKYMON_TELSIZE*1.8, y);
    cairo_set_line_width (cr, 1.8);
    cairo_stroke (cr);
    
    cairo_move_to (cr, x-SKYMON_TELSIZE*0.2, y-SKYMON_TELSIZE*1.8);
    cairo_line_to (cr, x+SKYMON_TELSIZE*0.2, y-SKYMON_TELSIZE*1.8);
    cairo_move_to (cr, x-SKYMON_TELSIZE*0.2, y+SKYMON_TELSIZE*1.8);
    cairo_line_to (cr, x+SKYMON_TELSIZE*0.2, y+SKYMON_TELSIZE*1.8);
    cairo_move_to (cr, x-SKYMON_TELSIZE*1.8,y-SKYMON_TELSIZE*0.2);
    cairo_line_to (cr, x-SKYMON_TELSIZE*1.8,y+SKYMON_TELSIZE*0.2);
    cairo_move_to (cr, x+SKYMON_TELSIZE*1.8,y-SKYMON_TELSIZE*0.2);
    cairo_line_to (cr, x+SKYMON_TELSIZE*1.8,y+SKYMON_TELSIZE*0.2);
    cairo_set_line_width (cr, 3.0);
    cairo_stroke (cr);
  }

  cairo_set_source_rgba(cr, 0.2, 0.6, 0.2, 1.0);
  cairo_arc(cr, x, y, 2, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_move_to (cr, x, y-SKYMON_TELSIZE*1.8);
  cairo_line_to (cr, x, y+SKYMON_TELSIZE*1.8);
  cairo_move_to (cr, x-SKYMON_TELSIZE*1.8, y);
  cairo_line_to (cr, x+SKYMON_TELSIZE*1.8, y);
  cairo_set_line_width (cr, 0.8);
  cairo_stroke (cr);

  cairo_move_to (cr, x-SKYMON_TELSIZE*0.2, y-SKYMON_TELSIZE*1.8);
  cairo_line_to (cr, x+SKYMON_TELSIZE*0.2, y-SKYMON_TELSIZE*1.8);
  cairo_move_to (cr, x-SKYMON_TELSIZE*0.2, y+SKYMON_TELSIZE*1.8);
  cairo_line_to (cr, x+SKYMON_TELSIZE*0.2, y+SKYMON_TELSIZE*1.8);
  cairo_move_to (cr, x-SKYMON_TELSIZE*1.8,y-SKYMON_TELSIZE*0.2);
  cairo_line_to (cr, x-SKYMON_TELSIZE*1.8,y+SKYMON_TELSIZE*0.2);
  cairo_move_to (cr, x+SKYMON_TELSIZE*1.8,y-SKYMON_TELSIZE*0.2);
  cairo_line_to (cr, x+SKYMON_TELSIZE*1.8,y+SKYMON_TELSIZE*0.2);
  cairo_set_line_width (cr, 2.0);
  cairo_stroke (cr);


  if(sz>0){
    cairo_select_font_face (cr, fontname, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to (cr, x, y);
    cairo_line_to (cr, x+SKYMON_TELSIZE*2, y-SKYMON_TELSIZE*1);
    cairo_line_to (cr, x+SKYMON_TELSIZE*3, y-SKYMON_TELSIZE*1);
    cairo_set_line_width (cr, 0.8);
    cairo_stroke (cr);
    
    cairo_set_font_size (cr, (gdouble)sz);
    cairo_text_extents (cr, "Target", &extents);
    if(allsky_flag){
      cairo_move_to(cr,
		    x+SKYMON_TELSIZE*3,
		    y-(extents.height/2+extents.y_bearing)-SKYMON_TELSIZE*1);
      cairo_text_path(cr, "Target");
      cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
      cairo_set_line_width(cr, (double)size_edge*0.7);
      cairo_stroke(cr);
      
      cairo_new_path(cr);
    }

    cairo_set_source_rgba(cr, 0.2, 0.6, 0.2, 1.0);
    cairo_move_to(cr,
		  x+SKYMON_TELSIZE*3,
		  y-(extents.height/2+extents.y_bearing)-SKYMON_TELSIZE*1);
    cairo_show_text(cr, "Target");
  }

}
#endif

static void cc_skymon_mode (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->skymon_mode=n;
  }

  if(hg->skymon_mode==SKYMON_SET){
    if(hg->allsky_last_timer!=-1)
      gtk_timeout_remove(hg->allsky_last_timer);
    gtk_widget_set_sensitive(hg->skymon_frame_date,TRUE);
    gtk_widget_set_sensitive(hg->skymon_frame_time,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_even,TRUE);
    
    if(flagSkymon){
      calcpa2_skymon(hg);

      if(flagTree){
	tree_update_azel((gpointer)hg);
      }

      draw_skymon(hg->skymon_dw,hg, FALSE);
      //gdk_window_raise(hg->skymon_main->window);
    }
  }
  else if(hg->skymon_mode==SKYMON_CUR){
    if(hg->allsky_last_timer!=-1)
      gtk_timeout_remove(hg->allsky_last_timer);
 
    gtk_widget_set_sensitive(hg->skymon_frame_date,FALSE);
    gtk_widget_set_sensitive(hg->skymon_frame_time,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_even,FALSE);
    
    if(flagSkymon){
      if(flagTree){
	tree_update_azel((gpointer)hg);
      }

      draw_skymon(hg->skymon_dw,hg, TRUE);
      //gdk_window_raise(hg->skymon_main->window);
    }
  }
  else if(hg->skymon_mode==SKYMON_LAST){
    if(hg->allsky_last_timer!=-1)
      gtk_timeout_remove(hg->allsky_last_timer);

    gtk_widget_set_sensitive(hg->skymon_frame_date,FALSE);
    gtk_widget_set_sensitive(hg->skymon_frame_time,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_even,FALSE);

    hg->allsky_last_repeat=0;

    if(hg->allsky_last_i<1){
      hg->skymon_mode=SKYMON_CUR;
      
      if(flagSkymon){
	if(flagTree){
	  tree_update_azel((gpointer)hg);
	}

	draw_skymon(hg->skymon_dw,hg, FALSE);
      }
    }
    else{
      hg->allsky_last_frame=0;

      if(flagSkymon){
	draw_skymon(hg->skymon_dw,hg, FALSE);
      }

      hg->allsky_last_timer=g_timeout_add(hg->allsky_last_interval, 
					  (GSourceFunc)skymon_last_go, 
					  (gpointer)hg);
    }
  }
}

static void cc_get_adj (GtkWidget *widget, gint * gdata)
{
  *gdata=GTK_ADJUSTMENT(widget)->value;
}

static void skymon_set_and_draw (GtkWidget *widget,   gpointer gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;

  
  if(flagSkymon){
    if(hg->skymon_mode==SKYMON_SET){
      calcpa2_skymon(hg);

      if(flagTree){
	tree_update_azel((gpointer)hg);
      }

      draw_skymon(hg->skymon_dw,hg, FALSE);
      //gdk_window_raise(hg->skymon_main->window);
    }
    else{
      gchar tmp[6];

      skymon_set_time_current(hg);

      gtk_adjustment_set_value(hg->skymon_adj_year, (gdouble)hg->skymon_year);
      gtk_adjustment_set_value(hg->skymon_adj_month,(gdouble)hg->skymon_month);
      gtk_adjustment_set_value(hg->skymon_adj_day,  (gdouble)hg->skymon_day);
      gtk_adjustment_set_value(hg->skymon_adj_hour, (gdouble)hg->skymon_hour);
      gtk_adjustment_set_value(hg->skymon_adj_min,  (gdouble)hg->skymon_min);

    }
  }
}


static void skymon_set_noobj (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;
  
  hg->noobj_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  
  draw_skymon(hg->skymon_dw,hg,FALSE);
}


static void skymon_set_hide (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;
  gint i_list;

  hg=(typHOE *)gdata;
  
  hg->hide_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  
  if(hg->hide_flag){
    for(i_list=0;i_list<hg->i_max;i_list++){
      hg->obj[i_list].check_disp=hg->obj[i_list].check_used;
    }
  }
  else{
    for(i_list=0;i_list<hg->i_max;i_list++){
      hg->obj[i_list].check_disp=TRUE;
    }
  }

  do_update_azel(NULL,(gpointer)hg);
  draw_skymon(hg->skymon_dw,hg,FALSE);
}


static void skymon_set_allsky (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;
#ifndef USE_WIN32
  pid_t child_pid=0;
#endif

  hg=(typHOE *)gdata;
  
  hg->allsky_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  
  if(hg->allsky_flag){
    if(hg->allsky_timer!=-1)
      gtk_timeout_remove(hg->allsky_timer);
    get_allsky(hg);

    hg->allsky_timer=g_timeout_add(hg->allsky_interval*1000, 
				 (GSourceFunc)update_allsky,
				 (gpointer)hg);
  }
  else{
    if(hg->allsky_timer!=-1){
      gtk_timeout_remove(hg->allsky_timer);
      if(flag_getting_allsky){

#ifndef USE_WIN32
	kill(allsky_pid, SIGKILL);
	do{
	  int child_ret;
	  child_pid=waitpid(allsky_pid, &child_ret,WNOHANG);
	} while((child_pid>0)||(child_pid!=-1));
	
	allsky_pid=0;
#endif
	flag_getting_allsky=FALSE;
	allsky_debug_print("Parent: killed child process\n");
      }
      if(hg->allsky_check_timer!=-1){
	gtk_timeout_remove(hg->allsky_check_timer); 
	allsky_debug_print("Parent: terminated checking timeout\n");
     }
    }
  }

  draw_skymon(hg->skymon_dw,hg,FALSE);

}

#ifdef USE_XMLRPC
static void skymon_set_telstat (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;
  
  hg->telstat_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  hg->telstat_error=FALSE;
  
  if(hg->telstat_flag){
    if(hg->telstat_timer!=-1)
      gtk_timeout_remove(hg->telstat_timer);

    printf_log(hg,"[TelStat] starting to fetch telescope status from %s",
	       hg->ro_ns_host);

    hg->telstat_timer=g_timeout_add(TELSTAT_INTERVAL, 
				    (GSourceFunc)update_telstat,
				    (gpointer)hg);
  }
  else{
    printf_log(hg,"[TelStat] stop Telstat.");

    if(hg->stat_initflag)  close_telstat(hg);
  }

  draw_skymon(hg->skymon_dw,hg,FALSE);
}
#endif

static void skymon_morning (GtkWidget *widget,   gpointer gdata)
{
  typHOE *hg;
  gchar tmp[6];

  hg=(typHOE *)gdata;

  
  if(flagSkymon){
    if(hg->skymon_mode==SKYMON_SET){

      if(hg->skymon_hour>10){
	add_day(hg, &hg->skymon_year, &hg->skymon_month, &hg->skymon_day, +1);

	gtk_adjustment_set_value(hg->skymon_adj_year, (gdouble)hg->skymon_year);
	gtk_adjustment_set_value(hg->skymon_adj_month,(gdouble)hg->skymon_month);
	gtk_adjustment_set_value(hg->skymon_adj_day,  (gdouble)hg->skymon_day);
      }
      hg->skymon_hour=hg->sun.s_rise.hours;
      hg->skymon_min=hg->sun.s_rise.minutes-SUNRISE_OFFSET;
      if(hg->skymon_min<0){
	hg->skymon_min+=60;
	hg->skymon_hour-=1;
      }

      gtk_adjustment_set_value(hg->skymon_adj_hour, (gdouble)hg->skymon_hour);
      gtk_adjustment_set_value(hg->skymon_adj_min,  (gdouble)hg->skymon_min);

      calcpa2_skymon(hg);

      if(flagTree){
	tree_update_azel((gpointer)hg);
      }

      draw_skymon(hg->skymon_dw,hg,FALSE);
    }
  }
}


static void skymon_evening (GtkWidget *widget,   gpointer gdata)
{
  typHOE *hg;
  gchar tmp[6];

  hg=(typHOE *)gdata;

  
  if(flagSkymon){
    if(hg->skymon_mode==SKYMON_SET){

      if(hg->skymon_hour<10){
	add_day(hg, &hg->skymon_year, &hg->skymon_month, &hg->skymon_day, -1);

	gtk_adjustment_set_value(hg->skymon_adj_year, (gdouble)hg->skymon_year);
	gtk_adjustment_set_value(hg->skymon_adj_month,(gdouble)hg->skymon_month);
	gtk_adjustment_set_value(hg->skymon_adj_day,  (gdouble)hg->skymon_day);
      }
      hg->skymon_hour=hg->sun.s_set.hours;
      hg->skymon_min=hg->sun.s_set.minutes+SUNSET_OFFSET;
      if(hg->skymon_min>=60){
	hg->skymon_min-=60;
	hg->skymon_hour+=1;
      }
      
      gtk_adjustment_set_value(hg->skymon_adj_hour, (gdouble)hg->skymon_hour);
      gtk_adjustment_set_value(hg->skymon_adj_min,  (gdouble)hg->skymon_min);

      calcpa2_skymon(hg);

      if(flagTree){
	tree_update_azel((gpointer)hg);
      }

      draw_skymon(hg->skymon_dw,hg,FALSE);
    }
  }
}


static void skymon_fwd (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;
  
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
    gtk_widget_set_sensitive(hg->skymon_frame_mode,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_set,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_even,FALSE);
    hg->skymon_timer=g_timeout_add(SKYMON_INTERVAL, 
				   (GSourceFunc)skymon_go, 
				   (gpointer)hg);
  }
  else{
    gtk_timeout_remove(hg->skymon_timer);
  
    gtk_widget_set_sensitive(hg->skymon_frame_mode,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_set,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_even,TRUE);
  }

}

gint skymon_go(typHOE *hg){
  gchar tmp[4];

  hg->skymon_min+=SKYMON_STEP;
  if(hg->skymon_min>=60){
    hg->skymon_min-=60;
    hg->skymon_hour+=1;
  }
  if(hg->skymon_hour>=24){
    hg->skymon_hour-=24;
    add_day(hg, &hg->skymon_year, &hg->skymon_month, &hg->skymon_day, +1);

    gtk_adjustment_set_value(hg->skymon_adj_year, (gdouble)hg->skymon_year);
    gtk_adjustment_set_value(hg->skymon_adj_month,(gdouble)hg->skymon_month);
    gtk_adjustment_set_value(hg->skymon_adj_day,  (gdouble)hg->skymon_day);
  }

  gtk_adjustment_set_value(hg->skymon_adj_hour, (gdouble)hg->skymon_hour);
  gtk_adjustment_set_value(hg->skymon_adj_min,  (gdouble)hg->skymon_min);


  if((hg->skymon_hour==7)||(hg->skymon_hour==7+24)){
    gtk_timeout_remove(hg->skymon_timer);
    gtk_widget_set_sensitive(hg->skymon_frame_mode,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_set,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_rev,TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->skymon_button_fwd),FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_even,TRUE);
    return FALSE;
  }
  
  if(flagSkymon){
    calcpa2_skymon(hg);

    if(flagTree){
      tree_update_azel((gpointer)hg);
    }

    draw_skymon(hg->skymon_dw,hg,FALSE);
  }

  return TRUE;

}

gint skymon_last_go(typHOE *hg){
  gchar tmp[4];

  
  if(flagSkymon){
    draw_skymon(hg->skymon_dw,hg,FALSE);
  }

  return TRUE;

}



static void skymon_rev (GtkWidget *w,   gpointer gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;
  
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){

    calcpa2_skymon(hg);
    
    gtk_widget_set_sensitive(hg->skymon_frame_mode,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_set,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,FALSE);
    gtk_widget_set_sensitive(hg->skymon_button_even,FALSE);
    hg->skymon_timer=g_timeout_add(SKYMON_INTERVAL, 
				   (GSourceFunc)skymon_back, 
				   (gpointer)hg);
  }
  else{
    gtk_timeout_remove(hg->skymon_timer);
  
    gtk_widget_set_sensitive(hg->skymon_frame_mode,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_set,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_even,TRUE);
  }

}


gint skymon_back(typHOE *hg){
  gchar tmp[4];

  hg->skymon_min-=SKYMON_STEP;
  if(hg->skymon_min<0){
    hg->skymon_min+=60;
    hg->skymon_hour-=1;
  }
  if(hg->skymon_hour<0){
    hg->skymon_hour+=24;
    add_day(hg, &hg->skymon_year, &hg->skymon_month, &hg->skymon_day, -1);

    gtk_adjustment_set_value(hg->skymon_adj_year, (gdouble)hg->skymon_year);
    gtk_adjustment_set_value(hg->skymon_adj_month,(gdouble)hg->skymon_month);
    gtk_adjustment_set_value(hg->skymon_adj_day,  (gdouble)hg->skymon_day);
  }

  gtk_adjustment_set_value(hg->skymon_adj_hour, (gdouble)hg->skymon_hour);
  gtk_adjustment_set_value(hg->skymon_adj_min,  (gdouble)hg->skymon_min);
	     

  if((hg->skymon_hour==18)||(hg->skymon_hour==18-24)){
    gtk_timeout_remove(hg->skymon_timer);
    gtk_widget_set_sensitive(hg->skymon_frame_mode,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_set,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_fwd,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_morn,TRUE);
    gtk_widget_set_sensitive(hg->skymon_button_even,TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->skymon_button_rev),FALSE);
    return FALSE;
  }
  
  if(flagSkymon){
    calcpa2_skymon(hg);

    if(flagTree){
      tree_update_azel((gpointer)hg);
    }

    draw_skymon(hg->skymon_dw,hg,FALSE);
  }

  return TRUE;

}

void skymon_set_time_current(typHOE *hg){
  int year, month, day, hour, min;
  double sec;

  get_current_obs_time(hg,&year, &month, &day, &hour, &min, &sec);

  hg->skymon_year=year;
  hg->skymon_month=month;
  hg->skymon_day=day;
    
  hg->skymon_hour=hour;
  hg->skymon_min=min;
}

#endif

void skymon_debug_print(const gchar *format, ...)
{
#ifdef SKYMON_DEBUG
        va_list args;
        gchar buf[BUFFSIZE];

        va_start(args, format);
        g_vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);

        fprintf(stderr,"%s", buf);
	fflush(stderr);
#endif
}

gint button_signal(GtkWidget *widget, 
		   GdkEventButton *event, 
		   gpointer userdata){
  typHOE *hg;
  gint x,y;
  gint i_list, i_sel=-1, i;
  gdouble sep=10.0, r_min=1000.0, r;
  

  hg=(typHOE *)userdata;

  //  if (event->type==GDK_2BUTTON_PRESS && event->button==1 ) {
  if ( event->button==1 ) {
    gdk_window_get_pointer(widget->window,&x,&y,NULL);

    if((x-hg->win_cx)*(x-hg->win_cx)+(y-hg->win_cy)*(y-hg->win_cy)<
       (hg->win_r*hg->win_r)){
      for(i_list=0;i_list<hg->i_max;i_list++){
	if((hg->obj[i_list].x>0)&&(hg->obj[i_list].y>0)){
	  if((fabs(hg->obj[i_list].x-x)<sep)
	     &&(fabs(hg->obj[i_list].y-y)<sep)){
	    r=(hg->obj[i_list].x-x)*(hg->obj[i_list].x-x)
	      +(hg->obj[i_list].y-y)*(hg->obj[i_list].y-y);
	    if(r<r_min){
	      i_sel=i_list;
	      r_min=r;
	    }
	  }
	}
      }
      
      if(i_sel>=0){
	if(!flagTree){
	  make_tree(hg->skymon_main,hg);
	}
	{
	  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->tree));
	  GtkTreePath *path;
	  GtkTreeIter  iter;

	  path=gtk_tree_path_new_first();

	  for(i=0;i<hg->i_max;i++){
	    gtk_tree_model_get_iter (model, &iter, path);
	    gtk_tree_model_get (model, &iter, COLUMN_OBJ_NUMBER, &i_list, -1);
	    i_list--;

	    if(i_list==i_sel){
	      gtk_tree_view_set_cursor(GTK_TREE_VIEW(hg->tree), path, NULL, FALSE);
	      break;
	    }
	    else{
	      gtk_tree_path_next(path);
	    }
	  }
	  gtk_tree_path_free(path);
	}

	skymon_debug_print(" Object %d is selected\n",i_sel+1);

      }
    }
  }
  
  return FALSE;
}


void draw_stderr_graph(typHOE *hg, cairo_t *cr, gint width, gint height, 
		       gdouble e_h){
  gdouble gw=80.0, gh=50.0;
  gdouble max=(ALLSKY_SE_MIN);
  gdouble dw=gw/(gdouble)(ALLSKY_LAST_MAX);
  gdouble x,y,x1,y1,x0,y0, se, contrast, r_abs;
  gint i;
  cairo_text_extents_t extents;
  gchar tmp[20];

  if(hg->allsky_last_i==0)  return;

  cairo_save(cr);

  x0=width-5-gw;
  y0=height-e_h*5-20;

  r_abs=hg->allsky_cloud_thresh*(gdouble)hg->allsky_diff_mag;

  for(i=0;i<hg->allsky_last_i;i++){
    if(hg->allsky_cloud_se[i]>max){
      max=(gdouble)((((gint)hg->allsky_cloud_se[i]+15)/10)*10);
    }
  }
  if(max>(ALLSKY_SE_MAX)){
    max=(ALLSKY_SE_MAX);
  }
  hg->allsky_cloud_se_max=max;

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
  cairo_rectangle(cr,x0,y0-gh, gw,gh);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0, 0.6, 0.4, 1.0);

  cairo_move_to(cr,x0, y0);
  cairo_line_to(cr,x0+gw,   y0);
  cairo_line_to(cr,x0+gw,   y0-gh);
  cairo_line_to(cr,x0,      y0-gh);
  cairo_line_to(cr,x0,      y0);
  cairo_set_line_width(cr,1.0);
  cairo_stroke(cr);

  cairo_move_to(cr,x0,   y0-gh/4.);
  cairo_line_to(cr,x0+gw,y0-gh/4.);
  cairo_set_line_width(cr,0.5);
  cairo_stroke(cr);

  cairo_move_to(cr,x0,   y0-gh/2.);
  cairo_line_to(cr,x0+gw,y0-gh/2.);
  cairo_set_line_width(cr,0.5);
  cairo_stroke(cr);

  cairo_move_to(cr,x0,   y0-gh/4.*3.);
  cairo_line_to(cr,x0+gw,y0-gh/4.*3.);
  cairo_set_line_width(cr,0.5);
  cairo_stroke(cr);

  //cairo_rectangle(cr,x0,height-e_h*5-20-gh, gw,gh);
  //cairo_clip(cr);
  
  // Contrast
  cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.8);
  if(hg->allsky_last_i>1){
    x=x0;
    if(hg->allsky_cloud_abs[0]/r_abs>4.){
      contrast=4.;
    }
    else{
      contrast=hg->allsky_cloud_abs[0]/r_abs;
    }
    y=y0-gh*contrast/4.;
    cairo_move_to(cr,x,y);

    for(i=0;i<hg->allsky_last_i-1;i++){
      x1=x+dw;
      if(hg->allsky_cloud_abs[i+1]/r_abs>4.){
	contrast=4.;
      }
      else{
	contrast=hg->allsky_cloud_abs[i+1]/r_abs;
      }
      y1=y0-gh*contrast/4.;
      cairo_line_to(cr,x1,y1);
    
      x=x1;
      y=y1;
    }
    cairo_set_line_width(cr,1.0);
    cairo_stroke(cr);
  }

  // StdErr
  cairo_set_source_rgba(cr, 0.2, 0.8, 0.2, 0.8);
  if(hg->allsky_last_i>1){
    x=x0;
    if(hg->allsky_cloud_se[0]>max){
      se=max;
    }
    else{
      se=hg->allsky_cloud_se[0];
    }
    y=y0-gh*se/max;
    cairo_move_to(cr,x,y);

    for(i=0;i<hg->allsky_last_i-1;i++){
      x1=x+dw;
      if(hg->allsky_cloud_se[i+1]>max){
	se=max;
      }
      else{
	se=hg->allsky_cloud_se[i+1];
      }
      y1=y0-gh*se/max;
      cairo_line_to(cr,x1,y1);
    
      x=x1;
      y=y1;
    }
    cairo_set_line_width(cr,1.0);
    cairo_stroke(cr);
  }

  cairo_set_source_rgba(cr, 0.0, 0.6, 0.0, 1.0);
  cairo_set_font_size (cr, 9);
  sprintf(tmp,"StdErr [<%d]", (gint)max);
  cairo_text_extents (cr, tmp, &extents);
  cairo_move_to(cr,x0+gw-extents.width,y0-gh-3);
  cairo_show_text(cr, tmp);
  
  /*
  cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);

  if(hg->skymon_mode==SKYMON_LAST){
    x=x0+dw*(gdouble)(hg->allsky_last_frame);
    if(hg->allsky_cloud_se[hg->allsky_last_frame]>max){
      se=max;
    }
    else{
      se=hg->allsky_cloud_se[hg->allsky_last_frame];
    }
    y=y0-gh*se/max;
  }
  cairo_arc(cr, x, y, 2.0, 0, 2*M_PI);
  cairo_fill(cr);
  */

  // CC
  cairo_set_source_rgba(cr, 0.2, 0.2, 0.8, 0.8);
  if(hg->allsky_last_i>1){
    x=x0;
    y=y0-gh*hg->allsky_cloud_area[0]/100.;
    cairo_move_to(cr,x,y);

    for(i=0;i<hg->allsky_last_i-1;i++){
      x1=x+dw;
      y1=y0-gh*hg->allsky_cloud_area[i+1]/100.;
      cairo_line_to(cr,x1,y1);
    
      x=x1;
      y=y1;
    }
    cairo_set_line_width(cr,1.0);
    cairo_stroke(cr);
  }

  cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);

  if(hg->skymon_mode==SKYMON_LAST){
    x=x0+dw*(gdouble)(hg->allsky_last_frame);
    y=y0-gh*hg->allsky_cloud_area[hg->allsky_last_frame]/100.;
  }
  cairo_arc(cr, x, y, 2.0, 0, 2*M_PI);
  cairo_fill(cr);


  cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 1.0);
  cairo_set_font_size (cr, 9);
  cairo_text_extents (cr, "Contrast", &extents);
  cairo_move_to(cr,x0+gw-extents.width,y0-gh-extents.height-6);
  cairo_show_text(cr, "Contrast");
  
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.6, 1.0);
  cairo_set_font_size (cr, 9);
  cairo_text_extents (cr, "CC", &extents);
  cairo_move_to(cr,x0+gw-extents.width,y0-gh-extents.height*2-9);
  cairo_show_text(cr, "CC");
  
  //cairo_reset_clip(cr);

  cairo_restore(cr);
}
