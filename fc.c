//    HSkymon
//      fc.c  --- Finding Chart
//   
//                                           2010.3.15  A.Tajitsu


#include"main.h"    // 設定ヘッダ
#include"version.h"
#include "hsc.h"
#include <cairo.h>
#include <cairo-pdf.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <signal.h>


void fc_item();
void fc_dl_draw ();
static gboolean progress_timeout();
void do_fc();
void create_fc_dialog();
void close_fc();
#ifndef USE_WIN32
static void cancel_fc();
#endif
gboolean draw_fc_cairo();
gboolean resize_draw_fc();
gboolean button_draw_fc();
void rot_pa();
static void refresh_fc();
void cc_get_fc_inst();
void cc_get_fc_mode();
void pdf_fc();
void do_print_fc();
static void draw_page();
#ifndef USE_WIN32
void dss_signal();
#endif

glong get_file_size();

void set_fc_mode();
void set_fc_frame_col();

void show_fc_help();
static void close_fc_help();

void fcdb_item2();
static void fcdb_item();
void fcdb_dl();
#ifndef USE_WIN32
void fcdb_signal();
static void cancel_fcdb();
#endif
void fcdb_tree_update_azel_item();
void fcdb_make_tree();
void fcdb_clear_tree();

gdouble current_yrs();
static void fcdb_toggle ();

extern int  get_dss();
extern int get_fcdb();
extern gboolean my_main_iteration();
extern void cc_get_toggle();
extern void cc_get_adj();
extern void cc_get_combo_box();
#ifdef __GTK_STOCK_H__
extern GtkWidget* gtkut_button_new_from_stock();
#endif
extern GtkWidget* gtkut_button_new_from_pixbuf();
extern void do_save_fc_pdf();

extern gboolean is_separator();

extern void fcdb_vo_parse();
extern double get_julian_day_of_epoch();

extern pid_t fc_pid;
extern gboolean flagTree;
extern pid_t fcdb_pid;

gboolean flagFC=FALSE, flag_getDSS=FALSE, flag_getFCDB=FALSE;
GdkPixbuf *pixbuf_fc=NULL, *pixbuf2_fc=NULL;


void fc_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

#ifdef USE_XMLRPC
  if(hg->fc_inst==FC_INST_NO_SELECT){ // First Time
    if(hg->stat_obcp){
      if(strcmp(hg->stat_obcp,"HDS")==0){
	hg->fc_inst=FC_INST_HDSAUTO;
	hg->dss_arcmin=HDS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"IRCS")==0){
	hg->fc_inst=FC_INST_IRCS;
	hg->dss_arcmin=IRCS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"COMICS")==0){
	hg->fc_inst=FC_INST_COMICS;
	hg->dss_arcmin=COMICS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"FOCAS")==0){
	hg->fc_inst=FC_INST_FOCAS;
	hg->dss_arcmin=FOCAS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"MOIRCS")==0){
	hg->fc_inst=FC_INST_MOIRCS;
	hg->dss_arcmin=MOIRCS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"FMOS")==0){
	hg->fc_inst=FC_INST_FMOS;
	hg->dss_arcmin=FMOS_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"SPCAM")==0){
	hg->fc_inst=FC_INST_SPCAM;
	hg->dss_arcmin=SPCAM_SIZE;
      }
      else if(strcmp(hg->stat_obcp,"HSC")==0){
	hg->fc_inst=FC_INST_HSCA;
	hg->dss_arcmin=HSC_SIZE;
      }
      else{
	hg->fc_inst=FC_INST_NONE;
      }
    }
    else{
      hg->fc_inst=FC_INST_NONE;
    }
  }
#endif

  fc_dl_draw(hg);

  if(hg->fcdb_auto) fcdb_item(NULL, (gpointer)hg);
}

void fc_dl_draw (typHOE *hg)
{
  GtkTreeIter iter;
  gchar tmp[2048];
  GtkWidget *dialog, *vbox, *label, *button;
#ifndef USE_WIN32
  static struct sigaction act;
#endif
  guint timer;
  
  if(flag_getDSS) return;
  flag_getDSS=TRUE;
  
  if(flagTree){
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->tree));

    if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
      gint i, i_list;
      GtkTreePath *path;
    
      path = gtk_tree_model_get_path (model, &iter);
      //i = gtk_tree_path_get_indices (path)[0];
      gtk_tree_model_get (model, &iter, COLUMN_OBJ_NUMBER, &i, -1);
      i--;
      
      hg->dss_i=i;

      gtk_tree_path_free (path);
    }
    else{
#ifdef GTK_MSG
      popup_message(POPUP_TIMEOUT,
		    "Error: Please select a target in the Object List.",
		    NULL);
#else
      fprintf(stderr," Error: Please select a target in the Object List.\n");
#endif
      flag_getDSS=FALSE;
      return;
    }
  }
  else if(hg->dss_i>=hg->i_max){
#ifdef GTK_MSG
    popup_message(POPUP_TIMEOUT,
		  "Error: Please select a target in the Object List.",
		  NULL);
#else
    fprintf(stderr," Error: Please select a target in the Object List.\n");
#endif
    flag_getDSS=FALSE;
    return;
  }

  while (my_main_iteration(FALSE));
  gdk_flush();

  dialog = gtk_dialog_new();
  
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"Sky Monitor : Message");
  gtk_window_set_decorated(GTK_WINDOW(dialog),TRUE);
  
#ifdef USE_GTK2  
  gtk_dialog_set_has_separator(GTK_DIALOG(dialog),TRUE);
#endif
  
  switch(hg->fc_mode){
  case FC_STSCI_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_ESO_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_ESO "\" ...");
    break;
    
  case FC_ESO_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_ESO "\" ...");
      break;
      
  case FC_ESO_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_ESO "\" ...");
    break;
    
  case FC_ESO_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_ESO "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSJ:
    label=gtk_label_new("Retrieving 2MASS (J-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSH:
    label=gtk_label_new("Retrieving 2MASS (H-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSK:
    label=gtk_label_new("Retrieving 2MASS (K-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_SDSSU:
    label=gtk_label_new("Retrieving SDSS (DR7/u-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_SDSSG:
    label=gtk_label_new("Retrieving SDSS (DR7/g-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_SDSSR:
    label=gtk_label_new("Retrieving SDSS (DR7/r-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_SDSSI:
    label=gtk_label_new("Retrieving SDSS (DR7/i-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_SDSSZ:
    label=gtk_label_new("Retrieving SDSS (DR7/z-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SDSS:
    label=gtk_label_new("Retrieving SDSS (DR7/color) image from \"" FC_HOST_SDSS "\" ...");
    break;
    
  case FC_SDSS13:
    label=gtk_label_new("Retrieving SDSS (DR13/color) image from \"" FC_HOST_SDSS13 "\" ...");
    break;
    
  }
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,TRUE,TRUE,0);
  gtk_widget_show(label);
  
  hg->pbar=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hg->pbar,TRUE,TRUE,0);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hg->pbar), 
				    GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(hg->pbar),0.05);
  gtk_widget_show(hg->pbar);
  
  unlink(hg->dss_file);
  
  timer=g_timeout_add(100, 
		      (GSourceFunc)progress_timeout,
		      (gpointer)hg);
  
  hg->plabel=gtk_label_new("Retrieving image from website ...");
  gtk_misc_set_alignment (GTK_MISC (hg->plabel), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     hg->plabel,FALSE,FALSE,0);
  
#ifndef USE_WIN32
#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#else
  button=gtk_button_new_with_label("Cancel");
#endif
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button,FALSE,FALSE,0);
  my_signal_connect(button,"pressed",
		    cancel_fc, 
		    (gpointer)hg);
#endif
  
  gtk_widget_show_all(dialog);
  
  //#ifdef USE_WIN32
  //while (my_main_iteration(FALSE));
  //#else
#ifndef USE_WIN32
  act.sa_handler=dss_signal;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  if(sigaction(SIGUSR1, &act, NULL)==-1)
    fprintf(stderr,"Error in sigaction (SIGUSR1).\n");
#endif
  
  gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
  
  get_dss(hg);
  //#ifndef USE_WIN32  
  gtk_main();
  //#endif
  gtk_timeout_remove(timer);
  gtk_widget_destroy(dialog);
  
  hg->dss_arcmin_ip=hg->dss_arcmin;
  hg->fc_mode_get=hg->fc_mode;
#ifndef USE_WIN32
  if(fc_pid){
#endif
    if(pixbuf_fc)  g_object_unref(G_OBJECT(pixbuf_fc));
    pixbuf_fc = gdk_pixbuf_new_from_file(hg->dss_file, NULL);
    
    do_fc(hg);
#ifndef USE_WIN32
  }
#endif

  fcdb_clear_tree(hg);

  flag_getDSS=FALSE;
}

static gboolean progress_timeout( gpointer data ){
  typHOE *hg=(typHOE *)data;
  glong sz;
  gchar *tmp;

  if(GTK_WIDGET_REALIZED(hg->pbar)){
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
    
    sz=get_file_size(hg->dss_file);
    if(sz>1024){
      sz=sz/1024;
      if(sz>1024){
	tmp=g_strdup_printf("Downloaded %.2f MB",(gfloat)sz/1024.);
      }
      else{
	tmp=g_strdup_printf("Downloaded %ld kB",sz);
      }
    }
    else if (sz>0){
      tmp=g_strdup_printf("Downloaded %ld bytes",sz);
    }
    else{
      tmp=g_strdup_printf("Waiting for WWW responce ...");
    }
    gtk_label_set_text(GTK_LABEL(hg->plabel), tmp);
    g_free(tmp);
    
    return TRUE;
  }
  else{
    return FALSE;
  }
}


void do_fc(typHOE *hg){
  if(flagFC){
    gdk_window_deiconify(hg->fc_main->window);
    gdk_window_raise(hg->fc_main->window);
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
    return;
  }
  else{
    flagFC=TRUE;
  }
  
  create_fc_dialog(hg);
}

void create_fc_dialog(typHOE *hg)
{
  GtkWidget *vbox, *vbox1, *hbox, *hbox1, *hbox2, *table;
  GtkWidget *frame, *check, *label, *button, *spinner;
  GtkAdjustment *adj;
  GtkWidget *menubar;
  GdkPixbuf *icon;

  // Win構築は重いので先にExposeイベント等をすべて処理してから
  while (my_main_iteration(FALSE));
  gdk_flush();

  hg->fc_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(hg->fc_main), "Sky Monitor : Finding Chart");
  //gtk_widget_set_usize(hg->skymon_main, SKYMON_SIZE, SKYMON_SIZE);
  
  my_signal_connect(hg->fc_main,
		    "destroy",
		    close_fc, 
		    (gpointer)hg);

  gtk_widget_set_app_paintable(hg->fc_main, TRUE);
  
  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_main), vbox);


  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new ("Image Source");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtk_table_new(5,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

#ifdef __GTK_STOCK_H__
  //button=gtkut_button_new_from_stock(NULL,GTK_STOCK_NETWORK);
  icon = gdk_pixbuf_new_from_inline(sizeof(icon_dl), icon_dl, 
				    FALSE, NULL);

  button=gtkut_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
#else
  button = gtk_button_new_with_label ("Download & Redraw");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (fc_item), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Download & Redraw");
#endif

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    GtkWidget *bar;
    
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "STScI: DSS1 (Red)",
		       1, FC_STSCI_DSS1R, 2, TRUE, -1);
    if(hg->fc_mode==FC_STSCI_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "STScI: DSS1 (Blue)",
		       1, FC_STSCI_DSS1B, 2, TRUE, -1);
    if(hg->fc_mode==FC_STSCI_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "STScI: DSS2 (Red)",
		       1, FC_STSCI_DSS2R, 2, TRUE, -1);
    if(hg->fc_mode==FC_STSCI_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "STScI: DSS2 (Blue)",
		       1, FC_STSCI_DSS2B, 2, TRUE, -1);
    if(hg->fc_mode==FC_STSCI_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "STScI: DSS2 (IR)",
		       1, FC_STSCI_DSS2IR, 2, TRUE, -1);
    if(hg->fc_mode==FC_STSCI_DSS2IR) iter_set=iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			0, NULL,
			1, FC_SEP1,2, FALSE, -1);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ESO: DSS1 (Red)",
		       1, FC_ESO_DSS1R, 2, TRUE, -1);
    if(hg->fc_mode==FC_ESO_DSS1R) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ESO: DSS2 (Red)",
		       1, FC_ESO_DSS2R, 2, TRUE, -1);
    if(hg->fc_mode==FC_ESO_DSS2R) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ESO: DSS2 (Blue)",
		       1, FC_ESO_DSS2B, 2, TRUE, -1);
    if(hg->fc_mode==FC_ESO_DSS2B) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ESO: DSS2 (IR)",
		       1, FC_ESO_DSS2IR, 2, TRUE, -1);
    if(hg->fc_mode==FC_ESO_DSS2IR) iter_set=iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			0, NULL, 1, FC_SEP2, 2, FALSE, -1);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: DSS1 (Red)",
		       1, FC_SKYVIEW_DSS1R, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: DSS1 (Blue)",
		       1, FC_SKYVIEW_DSS1B, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: DSS2 (Red)",
		       1, FC_SKYVIEW_DSS2R, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: DSS2 (Blue)",
		       1, FC_SKYVIEW_DSS2B, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: DSS2 (IR)",
		       1, FC_SKYVIEW_DSS2IR, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2IR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: 2MASS (J)",
		       1, FC_SKYVIEW_2MASSJ, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSJ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: 2MASS (H)",
		       1, FC_SKYVIEW_2MASSH, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSH) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: 2MASS (K)",
		       1, FC_SKYVIEW_2MASSK, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSK) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: SDSS (u)",
		       1, FC_SKYVIEW_SDSSU, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_SDSSU) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: SDSS (g)",
		       1, FC_SKYVIEW_SDSSG, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_SDSSG) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: SDSS (r)",
		       1, FC_SKYVIEW_SDSSR, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_SDSSR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: SDSS (i)",
		       1, FC_SKYVIEW_SDSSI, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_SDSSI) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SkyView: SDSS (z)",
		       1, FC_SKYVIEW_SDSSZ, 2, TRUE, -1);
    if(hg->fc_mode==FC_SKYVIEW_SDSSZ) iter_set=iter;
	
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			0, NULL, 1, FC_SEP3, 2, FALSE, -1);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR7 (color)",
		       1, FC_SDSS, 2, TRUE, -1);
    if(hg->fc_mode==FC_SDSS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR13 (color)",
		       1, FC_SDSS13, 2, TRUE, -1);
    if(hg->fc_mode==FC_SDSS13) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);	

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_mode,
		       (gpointer)hg);
  }

  frame = gtk_frame_new ("Size [\']");
  gtk_table_attach (GTK_TABLE(table), frame, 2, 3, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  hg->fc_adj_dss_arcmin = (GtkAdjustment *)gtk_adjustment_new(hg->dss_arcmin,
		            DSS_ARCMIN_MIN, DSS_ARCMIN_MAX,
   			    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->fc_adj_dss_arcmin, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),3);
  my_signal_connect (hg->fc_adj_dss_arcmin, "value_changed",
		     cc_get_adj,
		     &hg->dss_arcmin);


  hg->fc_frame_col = gtk_frame_new ("Scale/Color");
  gtk_table_attach (GTK_TABLE(table), hg->fc_frame_col, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->fc_frame_col), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_frame_col), hbox2);

  set_fc_frame_col(hg);
  /*
  button=gtk_check_button_new_with_label("Log");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_log);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_log);
  */

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Linear",
		       1, FC_SCALE_LINEAR, -1);
    if(hg->dss_scale==FC_SCALE_LINEAR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Log",
		       1, FC_SCALE_LOG, -1);
    if(hg->dss_scale==FC_SCALE_LOG) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Sqrt",
		       1, FC_SCALE_SQRT, -1);
    if(hg->dss_scale==FC_SCALE_SQRT) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HistEq",
		       1, FC_SCALE_HISTEQ, -1);
    if(hg->dss_scale==FC_SCALE_HISTEQ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "LogLog",
		       1, FC_SCALE_LOGLOG, -1);
    if(hg->dss_scale==FC_SCALE_LOGLOG) iter_set=iter;
	
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox2),combo,FALSE,FALSE,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->dss_scale);
  }

  button=gtk_check_button_new_with_label("Inv");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_invert);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_invert);


  frame = gtk_frame_new ("SIMBAD");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtk_table_new(2,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

  label=gtk_label_new("  ");
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 0, 1,
		    GTK_SHRINK,GTK_FILL,0,0);

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_FIND);
#else
  button = gtk_button_new_with_label ("Catalog matching");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (fcdb_item), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Catalog Matching");
#endif

  vbox1 = gtk_vbox_new(FALSE,0);
  gtk_table_attach (GTK_TABLE(table), vbox1, 1, 2, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);

  hg->fcdb_button=gtk_check_button_new_with_label("Disp");
  gtk_container_set_border_width (GTK_CONTAINER (hg->fcdb_button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hg->fcdb_button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fcdb_button),
			       hg->fcdb_flag);
  my_signal_connect(hg->fcdb_button,"toggled",
		    G_CALLBACK(fcdb_toggle), 
		    (gpointer)hg);

  button=gtk_check_button_new_with_label("Auto");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->fcdb_auto);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->fcdb_auto);


  frame = gtk_frame_new ("Instrument");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtk_table_new(4,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);


#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_REFRESH);
#else
  button = gtk_button_new_with_label ("Redraw");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (refresh_fc), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Redraw");
#endif


  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "None",
		       1, FC_INST_NONE, -1);
    if(hg->fc_inst==FC_INST_NONE) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS",
		       1, FC_INST_HDS, -1);
    if(hg->fc_inst==FC_INST_HDS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS (w/oImR)",
		       1, FC_INST_HDSAUTO, -1);
    if(hg->fc_inst==FC_INST_HDSAUTO) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS (Zenith)",
		       1, FC_INST_HDSZENITH, -1);
    if(hg->fc_inst==FC_INST_HDSZENITH) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "IRCS",
		       1, FC_INST_IRCS, -1);
    if(hg->fc_inst==FC_INST_IRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "COMICS",
		       1, FC_INST_COMICS, -1);
    if(hg->fc_inst==FC_INST_COMICS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "FOCAS",
		       1, FC_INST_FOCAS, -1);
    if(hg->fc_inst==FC_INST_FOCAS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "MOIRCS",
		       1, FC_INST_MOIRCS, -1);
    if(hg->fc_inst==FC_INST_MOIRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "FMOS",
		       1, FC_INST_FMOS, -1);
    if(hg->fc_inst==FC_INST_FMOS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SupCam",
		       1, FC_INST_SPCAM, -1);
    if(hg->fc_inst==FC_INST_SPCAM) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HSC (Det-ID)",
		       1, FC_INST_HSCDET, -1);
    if(hg->fc_inst==FC_INST_HSCDET) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HSC (HSCA)",
		       1, FC_INST_HSCA, -1);
    if(hg->fc_inst==FC_INST_HSCA) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_inst, (gpointer)hg);
  }

  button=gtk_check_button_new_with_label("Detail");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_table_attach (GTK_TABLE(table), button, 2, 3, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_draw_slit);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_draw_slit);



  frame = gtk_frame_new ("PA [deg]");
  gtk_table_attach (GTK_TABLE(table), frame, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  hg->fc_adj_dss_pa = (GtkAdjustment *)gtk_adjustment_new(hg->dss_pa,
						       -360, 360,
						       1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->fc_adj_dss_pa, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (hg->fc_adj_dss_pa, "value_changed",
		     cc_get_adj,
		     &hg->dss_pa);


  hg->fc_button_flip=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (hg->fc_button_flip), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),hg->fc_button_flip,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),hg->dss_flip);
  my_signal_connect(hg->fc_button_flip,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_flip);

  
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 3);


#ifdef __GTK_STOCK_H__
  icon = gdk_pixbuf_new_from_inline(sizeof(icon_pdf), icon_pdf, 
				    FALSE, NULL);
  button=gtkut_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
#else
  button = gtk_button_new_with_label ("PDF");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_save_fc_pdf), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Save as PDF");
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_PRINT);
#else
  button = gtk_button_new_with_label ("Print");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_print_fc), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Print out");
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_INFO);
#else
  button = gtk_button_new_with_label ("Help");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (show_fc_help), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Show Help");
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_CANCEL);
#else
  button = gtk_button_new_with_label ("Close");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (close_fc), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Close");
#endif


  hg->fc_mag=1;
  hg->fc_magmode=0;
  hg->fc_ptn=0;

  // Drawing Area
  hg->fc_dw = gtk_drawing_area_new();
  gtk_widget_set_size_request (hg->fc_dw, FC_WIDTH, FC_HEIGHT);
  gtk_box_pack_start(GTK_BOX(hbox), hg->fc_dw, TRUE, TRUE, 0);
  gtk_widget_set_app_paintable(hg->fc_dw, TRUE);
  gtk_widget_show(hg->fc_dw);

  screen_changed(hg->fc_dw,NULL,NULL);

  my_signal_connect(hg->fc_dw, 
		    "expose-event", 
		    draw_fc_cairo,
		    (gpointer)hg);
  my_signal_connect(hg->fc_dw,
		    "scroll-event", 
		    resize_draw_fc,
		    (gpointer)hg);
  my_signal_connect(hg->fc_dw, 
		    "button-press-event", 
		    button_draw_fc,
		    (gpointer)hg);
  gtk_widget_set_events(hg->fc_dw, GDK_SCROLL_MASK |
			GDK_SHIFT_MASK | 
                        GDK_BUTTON_RELEASE_MASK | 
                        GDK_BUTTON_PRESS_MASK | 
                        GDK_EXPOSURE_MASK);

  gtk_widget_show_all(hg->fc_main);

  gdk_window_raise(hg->fc_main->window);

  gdk_flush();
}


gboolean resize_draw_fc(GtkWidget *widget, 
			GdkEventScroll *event, 
			gpointer userdata){
  typHOE *hg;
  GdkScrollDirection direction;
  gint x,y;
  gint magx0, magy0, mag0;
  gint width, height;

  direction = event->direction;
  hg=(typHOE *)userdata;

  if(flagFC){
    if(event->state & GDK_SHIFT_MASK){
      if(direction & GDK_SCROLL_DOWN){
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa-5));
      }
      else{
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa+5));
      }
      hg->fc_output=FC_OUTPUT_WINDOW;
      draw_fc_cairo(hg->fc_dw,NULL,
		    (gpointer)hg);
    }
    else{
      gdk_window_get_pointer(widget->window,&x,&y,NULL);
      
      mag0=hg->fc_mag;
      magx0=hg->fc_magx;
      magy0=hg->fc_magy;
      
      if(direction & GDK_SCROLL_DOWN){
	hg->fc_mag--;
	hg->fc_ptn=0;
      }
      else{
	hg->fc_mag++;
	hg->fc_ptn=0;
      }
      
      if(hg->fc_mag<1){
	hg->fc_mag=1;
	hg->fc_magmode=0;
      }
      else if(hg->fc_mag>5){
	hg->fc_mag=5;
      }
      else{
	if(mag0==1){
	  hg->fc_magmode=0;
	}
	else if(hg->fc_magmode==0){
	  if((magx0!=x)||(magy0!=y)){
	    hg->fc_magmode=1;
	  }
	}
	
	if(hg->fc_magmode==0){
	  hg->fc_magx=x;
	  hg->fc_magy=y;
	}
	else{
	  width= widget->allocation.width;
	  height=widget->allocation.height;
	  
	  hg->fc_magx=magx0+(x-width/2)/mag0;
	  hg->fc_magy=magy0+(y-height/2)/mag0;
	}
	gtk_drawing_area_size (GTK_DRAWING_AREA(hg->fc_dw),
			       hg->fc_dw->allocation.width*hg->fc_mag,
			       hg->fc_dw->allocation.height*hg->fc_mag);
	//hg->fc_output=FC_OUTPUT_WINDOW;
	//draw_fc_cairo(hg->fc_dw,NULL,
	//		    (gpointer)hg);
      }
    }
  }

  return(TRUE);
}
  
gboolean button_draw_fc(GtkWidget *widget, 
			GdkEventScroll *event, 
			gpointer userdata){
  typHOE *hg;
  GdkScrollDirection direction;
  gint x,y;

  direction = event->direction;
  hg=(typHOE *)userdata;

  if(flagFC){
    gdk_window_get_pointer(widget->window,&x,&y,NULL);

    if(hg->fc_ptn==2){
      hg->fc_ptn=0;
    }
    else if(hg->fc_ptn==0){
      hg->fc_ptn=1;
      hg->fc_ptx1=x;
      hg->fc_pty1=y;
    }
    else if(hg->fc_ptn==1){
      hg->fc_ptn=2;
      hg->fc_ptx2=x;
      hg->fc_pty2=y;
    }

    //hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }

  return(TRUE);
}
  


void close_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;


  gtk_widget_destroy(GTK_WIDGET(hg->fc_main));
  flagFC=FALSE;
}


#ifndef USE_WIN32
void fcdb_signal(int sig){
  pid_t child_pid=0;

  gtk_main_quit();

  do{
    int child_ret;
    child_pid=waitpid(fcdb_pid, &child_ret,WNOHANG);
  } while((child_pid>0)||(child_pid!=-1));
}
#endif

#ifndef USE_WIN32
static void cancel_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  pid_t child_pid=0;

  hg=(typHOE *)gdata;

  if(fc_pid){
    kill(fc_pid, SIGKILL);
    gtk_main_quit();

    do{
      int child_ret;
      child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
    } while((child_pid>0)||(child_pid!=-1));
 
    fc_pid=0;
  }
}
#endif


void translate_to_center(cairo_t *cr, int width, int height, int width_file, int height_file, gfloat r, typHOE *hg)
{
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    cairo_translate (cr, (gdouble)width_file*r/2,
		     (gdouble)height_file*r/2);

    switch(hg->fc_inst){
    case FC_INST_NONE:
    case FC_INST_HDS:
    case FC_INST_IRCS:
    case FC_INST_COMICS:
    case FC_INST_FOCAS:
    case FC_INST_MOIRCS:
    case FC_INST_FMOS:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
      }

      break;

    case FC_INST_HDSAUTO:
      if(hg->skymon_mode==SKYMON_SET){
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)((int)hg->obj[hg->dss_i].s_hpa));
      }
      else{
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)((int)hg->obj[hg->dss_i].c_hpa));
      }
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
      }
      break;

    case FC_INST_HDSZENITH:
      if(hg->skymon_mode==SKYMON_SET){
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)((int)hg->obj[hg->dss_i].s_pa));
      }
      else{
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)((int)hg->obj[hg->dss_i].c_pa));
      }
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
      }
      break;

    case FC_INST_SPCAM:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }
      break;

    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)(270-hg->dss_pa)/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)(270-hg->dss_pa)/180.);
      }
      break;
    }
}


gboolean draw_fc_cairo(GtkWidget *widget, 
		       GdkEventExpose *event, 
		       gpointer userdata){
  cairo_t *cr;
  cairo_surface_t *surface;
  typHOE *hg;
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  GdkPixmap *pixmap_fc;
  gint from_set, to_rise;
  int width, height;
  int width_file, height_file;
  gfloat r_w,r_h, r;

  gdouble ra_0, dec_0;
  gchar tmp[2048];
  GdkPixbuf *pixbuf_flip=NULL;
  gfloat x_ccd, y_ccd, gap_ccd;
  //struct ln_hms ra_hms;
  //struct ln_dms dec_dms;
  gdouble scale;

  struct lnh_equ_posn hobject;
  
  if(!flagFC) return (FALSE);

  hg=(typHOE *)userdata;
  
  while (my_main_iteration(FALSE));
  gdk_flush();
  //printf("Drawing!\n");

  if(hg->fc_output==FC_OUTPUT_PDF){
    width= PLOT_HEIGHT;
    height= PLOT_HEIGHT;
    scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);

    surface = cairo_pdf_surface_create(hg->filename_pdf, width, height);
    cr = cairo_create(surface); 

    cairo_set_source_rgb(cr, 1, 1, 1);
  }
  else if (hg->fc_output==FC_OUTPUT_PRINT){
    width =  (gint)gtk_print_context_get_width(hg->context);
    height =  (gint)gtk_print_context_get_height(hg->context);
#ifdef USE_WIN32
    scale=(gdouble)width/PLOT_HEIGHT
      *(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);
#else
    scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);
#endif

    cr = gtk_print_context_get_cairo_context (hg->context);

    cairo_set_source_rgb(cr, 1, 1, 1);
  }
  else{
    width= widget->allocation.width*hg->fc_mag;
    height= widget->allocation.height*hg->fc_mag;
    if(width<=1){
      gtk_window_get_size(GTK_WINDOW(hg->fc_main), &width, &height);
    }
    scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);

    pixmap_fc = gdk_pixmap_new(widget->window,
			       width,
			       height,
			       -1);
  
    cr = gdk_cairo_create(pixmap_fc);

    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
    }
  }

 
  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);


  if(!pixbuf_fc){
    gdouble l_h;

    cairo_rectangle(cr, 0,0,
		    width,
		    height);
    if(hg->fc_output==FC_OUTPUT_WINDOW){
      cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    }
    cairo_fill(cr);

    if(hg->fc_output==FC_OUTPUT_WINDOW){
      cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 1.0);
    }
    cairo_set_font_size (cr, 12.0*scale);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    sprintf(tmp,"Error : Failed to load the image for the finding chart!");
    cairo_text_extents (cr,tmp, &extents);
    l_h=extents.height;
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2-l_h*1.5);
    cairo_show_text(cr, tmp);

 
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 10.0*scale);
    sprintf(tmp,"The position might be out of the surveyed area.");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*1.5);
    cairo_show_text(cr, tmp);
    
    sprintf(tmp,"or");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*3);
    cairo_show_text(cr, tmp);
    
    sprintf(tmp,"An HTTP error might be occured in the server side.");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*4.5);
    cairo_show_text(cr, tmp);
  }
  else{
    width_file = gdk_pixbuf_get_width(pixbuf_fc);
    height_file = gdk_pixbuf_get_height(pixbuf_fc);
    
    r_w =  (gfloat)width/(gfloat)width_file;
    r_h =  (gfloat)height/(gfloat)height_file;
    
    if(pixbuf2_fc) g_object_unref(G_OBJECT(pixbuf2_fc));
    
    if(r_w>r_h){
      r=r_h;
    }
    else{
      r=r_w;
    }
    
    if(hg->dss_flip){
      pixbuf_flip=gdk_pixbuf_flip(pixbuf_fc,TRUE);
      pixbuf2_fc=gdk_pixbuf_scale_simple(pixbuf_flip,
					 (gint)((gdouble)width_file*r),
					 (gint)((gdouble)height_file*r),
					 GDK_INTERP_BILINEAR);
      g_object_unref(G_OBJECT(pixbuf_flip));
    }
    else{
      pixbuf2_fc=gdk_pixbuf_scale_simple(pixbuf_fc,
					 (gint)((gdouble)width_file*r),
					 (gint)((gdouble)height_file*r),
					 GDK_INTERP_BILINEAR);
    }

      
    cairo_save (cr);

    translate_to_center(cr,width,height,width_file,height_file,r,hg);

    cairo_translate (cr, -(gdouble)width_file*r/2,
		     -(gdouble)height_file*r/2);
    gdk_cairo_set_source_pixbuf(cr, pixbuf2_fc, 0, 0);
    
    cairo_rectangle(cr, 0,0,
		    (gint)((gdouble)width_file*r),
		    (gint)((gdouble)height_file*r));
    cairo_fill(cr);

    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
      cairo_set_line_width (cr, 1.0*scale);
      cairo_rectangle(cr, 0,0,
		      (gint)((gdouble)width_file*r),
		      (gint)((gdouble)height_file*r));
      cairo_stroke(cr);
    }

    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    switch(hg->fc_inst){
    case FC_INST_HDS:
    case FC_INST_HDSAUTO:
    case FC_INST_HDSZENITH:
      if(hg->dss_draw_slit){
	cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
		  ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
		  0,M_PI*2);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.3);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
	cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*HDS_SLIT_MASK_ARCSEC);
	
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
	
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
	cairo_stroke(cr);
	
	cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*HDS_SLIT_WIDTH/500.);
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);
      }
      
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      
      cairo_set_line_width (cr, 3.0*scale);
      
      cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
		0,M_PI*2);
      cairo_stroke(cr);
      
      cairo_move_to(cr,
		    ((gdouble)width_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_set_line_width (cr, 1.5*scale);
      cairo_line_to(cr,
		    ((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_stroke(cr);
      
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);
      cairo_move_to(cr,
		    ((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_show_text(cr, "HDS SV FOV (1arcmin)");
      
      break;


    case FC_INST_IRCS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);

      sprintf(tmp,"IRCS FOV (%dx%darcsec)",(gint)IRCS_X_ARCSEC, (gint)IRCS_Y_ARCSEC);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.-5*scale);
      cairo_show_text(cr, tmp);

      break;


    case FC_INST_COMICS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_X_ARCSEC/60.)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_X_ARCSEC/60.,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);

      sprintf(tmp,"COMICS FOV (%dx%darcsec)",(gint)COMICS_X_ARCSEC, (gint)COMICS_Y_ARCSEC);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.)/2.-5*scale);
      cairo_show_text(cr, tmp);

      break;


    case FC_INST_FOCAS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);
      
      cairo_arc(cr,0,0,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
		0,M_PI*2);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);
	
      sprintf(tmp,"FOCAS FOV (%darcmin)",FOCAS_R_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,
		    -extents.width/2,
		    -FOCAS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	cairo_new_path(cr);
	cairo_arc(cr,0,0,
		  ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
		  0,M_PI*2);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	cairo_set_line_width (cr, FOCAS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_move_to(cr,-(gdouble)width/2,0);
	cairo_line_to(cr,(gdouble)width/2,0);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);

	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0*scale);
	cairo_text_extents (cr,"Chip 2", &extents);

	cairo_move_to(cr,
		      cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5*scale,
		      -sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN-5*scale);
	cairo_show_text(cr,"Chip 2");
	
	cairo_move_to(cr,
		      cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5*scale,
		      sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+extents.height+5*scale);
	cairo_show_text(cr,"Chip 1");
      }

      break;


    case FC_INST_MOIRCS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);

      sprintf(tmp,"MOIRCS FOV (%dx%darcmin)",(gint)MOIRCS_X_ARCMIN, (gint)MOIRCS_Y_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.-5*scale);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	cairo_new_path(cr);
	cairo_rectangle(cr,
			-((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
			-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
			(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
			(gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	cairo_set_line_width (cr, MOIRCS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_move_to(cr,-(gdouble)width/2,0);
	cairo_line_to(cr,(gdouble)width/2,0);
	cairo_stroke(cr);
	
	cairo_move_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+MOIRCS_VIG1X_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip),
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.-MOIRCS_VIG1Y_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_close_path(cr);
	cairo_fill_preserve(cr);

	cairo_move_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+MOIRCS_VIG2X_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip),
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.+MOIRCS_VIG2Y_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_close_path(cr);
	cairo_fill_preserve(cr);

	cairo_new_path(cr);

	cairo_reset_clip(cr);

	cairo_set_line_width(cr,1.5*scale);
	cairo_arc(cr,0,0,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.,
		  0,M_PI*2);
	cairo_stroke(cr);

	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0*scale);
	cairo_text_extents (cr,"Detector 2", &extents);

	cairo_move_to(cr,
		      ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5*scale,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.+extents.height);
	cairo_show_text(cr,"Detector 2");
	
	cairo_move_to(cr,
		      ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5*scale,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_show_text(cr,"Detector 1");

	cairo_rotate (cr,-M_PI/2);
	cairo_text_extents (cr,"6 arcmin from the center", &extents);
	cairo_move_to(cr,-extents.width/2.,
		      -(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.-5*scale);
	cairo_show_text(cr,"6 arcmin from the center");
      }


      break;


    case FC_INST_SPCAM:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);

      sprintf(tmp,"Suprime-Cam FOV (%dx%darcmin)",SPCAM_X_ARCMIN, SPCAM_Y_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.-5*scale);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.3);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
	cairo_set_line_width (cr, 1.5*scale);

	x_ccd=0.20/60.*2048.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	y_ccd=0.20/60.*4096.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	gap_ccd=SPCAM_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	//2 fio
	cairo_rectangle(cr,-x_ccd/2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//5 satsuki
	cairo_rectangle(cr,-x_ccd/2.,+gap_ccd/2.,
			x_ccd,y_ccd);

	//7 clarisse
	cairo_rectangle(cr,-x_ccd/2*3.-gap_ccd,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//9 san
	cairo_rectangle(cr,-x_ccd/2.*3.-gap_ccd,+gap_ccd/2.,
			x_ccd,y_ccd);

	//6 chihiro
	cairo_rectangle(cr,-x_ccd/2*5.-gap_ccd*2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//8 ponyo
	cairo_rectangle(cr,-x_ccd/2.*5.-gap_ccd*2.,+gap_ccd/2.,
			x_ccd,y_ccd);

	//2 fio
	cairo_rectangle(cr,x_ccd/2.+gap_ccd,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//5 satsuki
	cairo_rectangle(cr,x_ccd/2.+gap_ccd,+gap_ccd/2.,
			x_ccd,y_ccd);

	//0 nausicca
	cairo_rectangle(cr,x_ccd/2.*3.+gap_ccd*2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//3 sophie
	cairo_rectangle(cr,x_ccd/2.*3.+gap_ccd*2,+gap_ccd/2.,
			x_ccd,y_ccd);


	cairo_stroke(cr);

	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0*scale);
	cairo_text_extents (cr,"2. fio", &extents);

	//2 fio
	cairo_move_to(cr,-x_ccd/2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
	cairo_show_text(cr,"2. fio");

	//5 satsuki
	cairo_move_to(cr,-x_ccd/2.+15*scale,+gap_ccd/2.+y_ccd-15*scale);
	cairo_show_text(cr,"5. satsuki");

	//7 clarisse
	cairo_move_to(cr,-x_ccd/2*3.-gap_ccd+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
	cairo_show_text(cr,"7. clarisse");

	//9 san
	cairo_move_to(cr,-x_ccd/2.*3.-gap_ccd+15*scale,+gap_ccd/2.+y_ccd-15*scale);
	cairo_show_text(cr,"9. san");

	//6 chihiro
	cairo_move_to(cr,-x_ccd/2*5.-gap_ccd*2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
	cairo_show_text(cr,"6. chihiro");

	//8 ponyo
	cairo_move_to(cr,-x_ccd/2.*5.-gap_ccd*2.+15*scale,+gap_ccd/2.+y_ccd-15*scale);
	cairo_show_text(cr,"8. ponyo");

	//1 kiki
	cairo_move_to(cr,x_ccd/2.+gap_ccd+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
	cairo_show_text(cr,"1. kiki");

	//4 sheeta
	cairo_move_to(cr,x_ccd/2.+gap_ccd+15*scale,+gap_ccd/2.+y_ccd-15*scale);
	cairo_show_text(cr,"4. sheeta");

	//0 nausicaa
	cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
	cairo_show_text(cr,"0. nausicaa");

	//3 sophie
	cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2+15*scale,+gap_ccd/2.+y_ccd-15*scale);
	cairo_show_text(cr,"3. sophie");
      }

      break;

    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);
      
      cairo_arc(cr,0,0,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*HSC_R_ARCMIN,
		0,M_PI*2);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);
	
      if(!hg->dss_draw_slit){
	sprintf(tmp,"HSC FOV (%darcmin)",HSC_R_ARCMIN);
	cairo_text_extents (cr,tmp, &extents);
	cairo_move_to(cr,
		      -extents.width/2,
		      -HSC_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
	cairo_show_text(cr, tmp);
      }
      else{
	gint i_chip;
	gdouble pscale;
	gdouble x_0,y_0;
	
	pscale=(1.5*60.*60./(497./0.015))/60.*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	// HSC pix scale 1.5deg = 497mm phi

	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	cairo_set_line_width (cr, 0.8*scale);

	cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);

	// Dead chips
	{
	  gint i_dead;
	  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
	  else cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3);

	  for(i_dead=0;i_dead<HSC_DEAD_ALL;i_dead++){
	  
	    y_0=(-(gdouble)hsc_dead[i_dead].crpix1*(gdouble)hsc_dead[i_dead].cd1_1/0.015-(gdouble)hsc_dead[i_dead].crpix2*(gdouble)hsc_dead[i_dead].cd1_2/0.015)*pscale;
	    x_0=(-(gdouble)hsc_dead[i_dead].crpix1*(gdouble)hsc_dead[i_dead].cd2_1/0.015-(gdouble)hsc_dead[i_dead].crpix2*(gdouble)hsc_dead[i_dead].cd2_2/0.015)*pscale;
	    if((hsc_dead[i_dead].cd1_2<0)&&(hsc_dead[i_dead].cd2_1<0)){
	      cairo_rectangle(cr, x_0-2048*pscale/4*(hsc_dead[i_dead].ch),
			      y_0-4224*pscale, 2048*pscale/4, 4224*pscale );
	    }
	    else if((hsc_dead[i_dead].cd1_2>0)&&(hsc_dead[i_dead].cd2_1>0)){
	      cairo_rectangle(cr,x_0+2048*pscale/4*(hsc_dead[i_dead].ch-1), y_0, 2048*pscale/4, 4224*pscale);
	    }
	    else if((hsc_dead[i_dead].cd1_1>0)&&(hsc_dead[i_dead].cd2_2<0)){
	      cairo_rectangle(cr,x_0-4224*pscale, y_0+2048*pscale/4*(hsc_dead[i_dead].ch-1),  4224*pscale, 2048*pscale/4);
	    }
	    else{
	      cairo_rectangle(cr,x_0, y_0-2048*pscale/4*(hsc_dead[i_dead].ch), 4224*pscale, 2048*pscale/4);
	    }
	    cairo_fill(cr);
	  }
	}

	for(i_chip=0;i_chip<HSC_CHIP_ALL;i_chip++){

	  if(hsc_param[i_chip].bees==2){
	    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.6, 0.0, 0.6);
	    else cairo_set_source_rgba(cr, 0.4, 1.0, 0.4, 0.6);
	  }
	  else if(hsc_param[i_chip].bees==0){
	    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.0, 0.5, 0.6);
	    else cairo_set_source_rgba(cr, 0.8, 0.4, 0.8, 0.6);
	  }
	  else{
	    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
	    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	  }
	  
	  cairo_set_font_size (cr, 600*pscale);
	  cairo_text_extents (cr,"000", &extents);

	  y_0=(-(gdouble)hsc_param[i_chip].crpix1*(gdouble)hsc_param[i_chip].cd1_1/0.015-(gdouble)hsc_param[i_chip].crpix2*(gdouble)hsc_param[i_chip].cd1_2/0.015)*pscale;
	  x_0=(-(gdouble)hsc_param[i_chip].crpix1*(gdouble)hsc_param[i_chip].cd2_1/0.015-(gdouble)hsc_param[i_chip].crpix2*(gdouble)hsc_param[i_chip].cd2_2/0.015)*pscale;

	  if((hsc_param[i_chip].cd1_2<0)&&(hsc_param[i_chip].cd2_1<0)){
	    cairo_rectangle(cr, x_0-2048*pscale, y_0-4224*pscale, 2048*pscale, 4224*pscale );
	    cairo_move_to(cr, x_0-2048*pscale+2044*pscale*0.05, y_0-4224*pscale+2044*pscale*0.05-extents.y_bearing);
	  }
	  else if((hsc_param[i_chip].cd1_2>0)&&(hsc_param[i_chip].cd2_1>0)){
	    cairo_rectangle(cr,x_0, y_0, 2048*pscale, 4224*pscale);
	    cairo_move_to(cr, x_0+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
	  }
	  else if((hsc_param[i_chip].cd1_1>0)&&(hsc_param[i_chip].cd2_2<0)){
	    cairo_rectangle(cr,x_0-4224*pscale, y_0,  4224*pscale, 2048*pscale);
	    cairo_move_to(cr, x_0-4224*pscale+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
	  }
	  else{
	    cairo_rectangle(cr,x_0, y_0-2048*pscale, 4224*pscale, 2048*pscale );
	    cairo_move_to(cr, x_0+2048*pscale*0.05, y_0-2048*pscale+2048*pscale*0.05-extents.y_bearing);
	  }

	  cairo_set_font_size (cr, 600*pscale);
	  if(hg->fc_inst==FC_INST_HSCDET){
	    sprintf(tmp,"%d",hsc_param[i_chip].det_id);
	  }
	  else{
	    
	    sprintf(tmp,"%02d",hsc_param[i_chip].hsca);
	  }
	  cairo_show_text(cr,tmp);

	  if(hsc_param[i_chip].hsca==35){
	    cairo_set_font_size (cr, 1600*pscale);
	    sprintf(tmp,"BEES%d",hsc_param[i_chip].bees);
	    cairo_text_extents (cr,tmp, &extents);
	  
	    if(hsc_param[i_chip].bees==0){
	      cairo_move_to(cr, x_0+4224*pscale-2048*pscale*0.5-extents.width, y_0+2048*pscale*0.2-extents.y_bearing);
	    }
	    else{
	      cairo_move_to(cr, x_0-4224*pscale+2048*pscale*0.5, y_0-2048*pscale*0.2);
	    }
	    cairo_show_text(cr,tmp);
	  }

	  cairo_stroke(cr);

	}
      }
      break;

    case FC_INST_FMOS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0*scale);
      
      cairo_arc(cr,0,0,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FMOS_R_ARCMIN,
		0,M_PI*2);
      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0*scale);
	
      sprintf(tmp,"FMOS FOV (%darcmin)",FMOS_R_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,
		    -extents.width/2,
		    -FMOS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
      cairo_show_text(cr, tmp);

      break;
    }

    
    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);
    
    ra_0=hg->obj[hg->dss_i].ra;
    hobject.ra.hours=(gint)(ra_0/10000);
    ra_0=ra_0-(gdouble)(hobject.ra.hours)*10000;
    hobject.ra.minutes=(gint)(ra_0/100);
    hobject.ra.seconds=ra_0-(gdouble)(hobject.ra.minutes)*100;

    if(hg->obj[hg->dss_i].dec<0){
      hobject.dec.neg=1;
      dec_0=-hg->obj[hg->dss_i].dec;
    }
    else{
      hobject.dec.neg=0;
      dec_0=hg->obj[hg->dss_i].dec;
    }
    hobject.dec.degrees=(gint)(dec_0/10000);
    dec_0=dec_0-(gfloat)(hobject.dec.degrees)*10000;
    hobject.dec.minutes=(gint)(dec_0/100);
    hobject.dec.seconds=dec_0-(gfloat)(hobject.dec.minutes)*100;

    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.3, 0.45, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.4, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 11.0*scale);
    cairo_move_to(cr,5*scale,(gdouble)height_file*r-5*scale);
    sprintf(tmp,"RA=%02d:%02d:%05.2lf  Dec=%s%02d:%02d:%05.2lf (%.1lf)",
	    hobject.ra.hours,hobject.ra.minutes,
	    hobject.ra.seconds,
	    (hobject.dec.neg) ? "-" : "+", 
	    hobject.dec.degrees, hobject.dec.minutes,
	    hobject.dec.seconds,
	    hg->obj[hg->dss_i].epoch);
    cairo_text_extents (cr, tmp, &extents);
    cairo_show_text(cr,tmp);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr,5*scale,(gdouble)height_file*r-5*scale-extents.height-5*scale);
    cairo_show_text(cr,hg->obj[hg->dss_i].name);


    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 10.0*scale);
    switch(hg->fc_mode_get){
    case FC_STSCI_DSS1R:
    case FC_ESO_DSS1R:
    case FC_SKYVIEW_DSS1R:
      sprintf(tmp,"DSS1 (Red)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_STSCI_DSS1B:
    case FC_SKYVIEW_DSS1B:
      sprintf(tmp,"DSS1 (Blue)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_STSCI_DSS2R:
    case FC_ESO_DSS2R:
    case FC_SKYVIEW_DSS2R:
      sprintf(tmp,"DSS2 (Red)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_STSCI_DSS2B:
    case FC_ESO_DSS2B:
    case FC_SKYVIEW_DSS2B:
      sprintf(tmp,"DSS2 (Blue)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_STSCI_DSS2IR:
    case FC_ESO_DSS2IR:
    case FC_SKYVIEW_DSS2IR:
      sprintf(tmp,"DSS2 (IR)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_2MASSJ:
      sprintf(tmp,"2MASS (J)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_2MASSH:
      sprintf(tmp,"2MASS (H)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_2MASSK:
      sprintf(tmp,"2MASS (K)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_SDSSU:
      sprintf(tmp,"SDSS DR7 (u)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_SDSSG:
      sprintf(tmp,"SDSS DR7 (g)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_SDSSR:
      sprintf(tmp,"SDSS DR7 (r)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_SDSSI:
      sprintf(tmp,"SDSS DR7 (i)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SKYVIEW_SDSSZ:
      sprintf(tmp,"SDSS DR7 (z)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;

    case FC_SDSS:
      sprintf(tmp,"SDSS DR7 (color)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;
      
    case FC_SDSS13:
      sprintf(tmp,"SDSS DR13 (color)  %dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
      break;
      
    default:
      sprintf(tmp,"%dx%d arcmin",
	      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
    }
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,
		  (gdouble)width_file*r-extents.width-5*scale,
		  extents.height+5*scale);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);


    cairo_save (cr);

    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 11.0*scale);
    cairo_text_extents (cr, "N", &extents);

    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);
    cairo_translate (cr, 
		     5+(gdouble)width_file*r*0.05+extents.width*1.5,
		     5+(gdouble)width_file*r*0.05+extents.height*1.5);

    rot_pa(cr, hg);

    // Position Angle
    if(hg->fc_mag==1){
      cairo_move_to(cr,
		    -extents.width/2,
		    -(gdouble)width_file*r*0.05);
      cairo_show_text(cr,"N");
      cairo_move_to(cr,
		    -(gdouble)width_file*r*0.05-extents.width,
		    +extents.height/2);
      if(hg->dss_flip){
	cairo_show_text(cr,"W");
      }
      else{
	cairo_show_text(cr,"E");
      }
      
      cairo_set_line_width (cr, 1.5*scale*hg->fc_mag);
      cairo_move_to(cr,
		    0,
		    -(gdouble)width_file*r*0.05);
      cairo_line_to(cr, 0, 0);
      cairo_line_to(cr,
		    -(gdouble)width_file*r*0.05, 0);
      
      cairo_stroke(cr);
      
      if(hg->dss_flip){
	cairo_move_to(cr,0,0);
	cairo_text_extents (cr, "(flipped)", &extents);
	cairo_rel_move_to(cr,-extents.width/2.,extents.height+5*scale);
	cairo_show_text(cr,"(flipped)");
      }
    } // Position Angle
    
    cairo_restore(cr);

   // Position Angle  for mag
    if(hg->fc_mag!=1){
      gdouble wh_small;
      gdouble xsec,ysec;
      gdouble pscale;

      cairo_save (cr);

      wh_small=(gdouble)(width>height?height:width)/(gdouble)hg->fc_mag;
      pscale=(gdouble)hg->dss_arcmin_ip*60./wh_small;
      xsec=(gdouble)width*pscale/(gdouble)hg->fc_mag/(gdouble)hg->fc_mag;
      ysec=(gdouble)height*pscale/(gdouble)hg->fc_mag/(gdouble)hg->fc_mag;
	
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);

      cairo_set_font_size (cr, 14.0*scale);
      if((xsec>60.) && (ysec>60.)){
	sprintf(tmp,"x%d : %.2lfx%.2lf arcmin",hg->fc_mag,
		xsec/60.,
		ysec/60.);
      }
      else{
	sprintf(tmp,"x%d : %.1lfx%.1lf arcsec",hg->fc_mag,xsec,ysec);
      }
      cairo_text_extents (cr, tmp, &extents);
      cairo_translate(cr,
           	      width/(gdouble)hg->fc_mag+(hg->fc_magx*hg->fc_mag-width/2/hg->fc_mag),
		      height/(gdouble)hg->fc_mag+(hg->fc_magy*hg->fc_mag-height/2/hg->fc_mag));
      cairo_move_to(cr,
		    -extents.width-wh_small*0.02,
		    -wh_small*0.02);
      cairo_show_text(cr,tmp);

      cairo_translate(cr,
		      -width/(gdouble)hg->fc_mag,
		      -height/(gdouble)hg->fc_mag);

      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);

      cairo_set_font_size (cr, 11.0*scale);
      cairo_text_extents (cr, "N", &extents);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
      
      cairo_translate(cr,
           	      extents.height+wh_small*0.07,
		      extents.height+wh_small*0.07);

      rot_pa(cr, hg);

      cairo_move_to(cr,
		    -extents.width/2,
		    -wh_small*0.05);
      cairo_show_text(cr,"N");
      cairo_move_to(cr,
		    -wh_small*0.05-extents.width,
		    +extents.height/2);
      if(hg->dss_flip){
	cairo_show_text(cr,"W");
      }
      else{
	cairo_show_text(cr,"E");
      }
      
      cairo_set_line_width (cr, 1.5*scale);
      cairo_move_to(cr,
		    0,
		    -wh_small*0.05);
      cairo_line_to(cr, 0, 0);
      cairo_line_to(cr,
		    -wh_small*0.05, 0);

      cairo_stroke(cr);
      
      if(hg->dss_flip){
	cairo_move_to(cr,0,0);
	cairo_text_extents (cr, "(flipped)", &extents);
	cairo_rel_move_to(cr,-extents.width/2.,extents.height+5*scale);
	cairo_show_text(cr,"(flipped)");
      }

      cairo_restore(cr);
    } // Position Angle
  }

  {
    gdouble fcx, fcy;
    gdouble pmx, pmy;
    gdouble yrs;

    if((hg->fcdb_flag)&&(hg->fcdb_i==hg->dss_i)){
      cairo_save(cr);

      translate_to_center(cr,width,height,width_file,height_file,r,hg);
      yrs=current_yrs(hg);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
      cairo_set_line_width (cr, 2*scale);
      for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
	if(hg->fcdb_tree_focus!=i_list){
	  fcx=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0)*60.
	    *cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  fcy=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0)*60.
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  if(hg->dss_flip) fcx=-fcx;
	  cairo_rectangle(cr,fcx-6,fcy-6,12,12);
	  cairo_stroke(cr);
	}
      }

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.7, 0.0, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
      cairo_set_line_width (cr, 4*scale);
      for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
	if(hg->fcdb_tree_focus==i_list){
	  fcx=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0)*60.
	    *cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  fcy=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0)*60.
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  if(hg->dss_flip) fcx=-fcx;
	  cairo_rectangle(cr,fcx-10,fcy-10,16,16);
	  cairo_stroke(cr);
	}
      }

      // Proper Motion
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 0.2, 1.0, 0.2, 1.0);
      cairo_set_line_width (cr, 1.5*scale);
      for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
	if(hg->fcdb[i_list].pm){
	  fcx=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0)*60.
	    *cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  fcy=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0)*60.
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  pmx=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0
		+hg->fcdb[i_list].pmra/1000/60/60*yrs)*60.
	    *cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  pmy=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0
		+hg->fcdb[i_list].pmdec/1000/60/60*yrs)*60.
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  if(hg->dss_flip) {
	    fcx=-fcx;
	    pmx=-pmx;
	  }
	  cairo_move_to(cr,fcx,fcy);
	  cairo_line_to(cr,pmx,pmy);
	  cairo_stroke(cr);
	  cairo_arc(cr,pmx,pmy,5,0,2*M_PI);
	  cairo_fill(cr);
	}
      }
      cairo_restore(cr);
    }
  }

  {  // Points and Distance
    gdouble distance;
    gdouble arad;

    if(hg->fc_ptn>=1){
      cairo_save(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.8);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.8);

      cairo_set_line_width (cr, 2*scale);

      if(hg->fc_mag!=1){
	cairo_translate(cr,
			(hg->fc_magx*hg->fc_mag-width/2/hg->fc_mag),
			(hg->fc_magy*hg->fc_mag-height/2/hg->fc_mag));
      }

      cairo_move_to(cr,hg->fc_ptx1-5,hg->fc_pty1-5);
      cairo_rel_line_to(cr,10,10);
      
      cairo_move_to(cr,hg->fc_ptx1-5,hg->fc_pty1+5);
      cairo_rel_line_to(cr,10,-10);

      cairo_stroke(cr);
    }

    if(hg->fc_ptn==2){
      cairo_move_to(cr,hg->fc_ptx2-5,hg->fc_pty2-5);
      cairo_rel_line_to(cr,10,10);
      
      cairo_move_to(cr,hg->fc_ptx2-5,hg->fc_pty2+5);
      cairo_rel_line_to(cr,10,-10);

      cairo_stroke(cr);

      cairo_set_line_width (cr, 0.8*scale);
      
      cairo_move_to(cr,hg->fc_ptx1,hg->fc_pty1);
      cairo_line_to(cr,hg->fc_ptx2,hg->fc_pty2);

      cairo_stroke(cr);

      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 12.0*scale);

      distance=sqrt((gdouble)((hg->fc_ptx1-hg->fc_ptx2)
			      *(hg->fc_ptx1-hg->fc_ptx2))
		    +(gdouble)((hg->fc_pty1-hg->fc_pty2)
			       *(hg->fc_pty1-hg->fc_pty2)))
	/((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)*60.0;

      if(distance > 300){
	sprintf(tmp,"%.2lf'",distance/60.0);
      }
      else{
	sprintf(tmp,"%.2lf\"",distance);
      }
      cairo_text_extents (cr, tmp, &extents);

      arad=atan2((hg->fc_ptx1-hg->fc_ptx2),(hg->fc_pty1-hg->fc_pty2));
      cairo_translate(cr,
		      (hg->fc_ptx1+hg->fc_ptx2)/2,
		      (hg->fc_pty1+hg->fc_pty2)/2);
      cairo_rotate (cr,-(arad+M_PI/2));
      
      cairo_move_to(cr,-extents.width/2.,-extents.height*0.8);
      cairo_show_text(cr,tmp);
    }

    cairo_restore(cr);
  }
  
  if(hg->fc_output==FC_OUTPUT_PDF){
    cairo_show_page(cr); 
    cairo_surface_destroy(surface);
  }

  if(hg->fc_output!=FC_OUTPUT_PRINT){
    cairo_destroy(cr);
  }

  if(hg->fc_output==FC_OUTPUT_WINDOW){
    if(hg->fc_mag==1){
      gdk_draw_drawable(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			pixmap_fc,
			0,0,0,0,
			width,
			height);
    }
    else{
      gdk_draw_drawable(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			pixmap_fc,
			0,
			0,
			-(hg->fc_magx*hg->fc_mag-width/2/hg->fc_mag),
			-(hg->fc_magy*hg->fc_mag-height/2/hg->fc_mag),
			width,
			height);
    }
    g_object_unref(G_OBJECT(pixmap_fc));
  }

  return TRUE;

}


static void refresh_fc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagFC){
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }
}


void rot_pa(cairo_t *cr, typHOE *hg){
  switch(hg->fc_inst){
  case FC_INST_NONE:
  case FC_INST_HDS:
  case FC_INST_HDSAUTO:
  case FC_INST_HDSZENITH:
  case FC_INST_IRCS:
  case FC_INST_COMICS:
  case FC_INST_FOCAS:
  case FC_INST_MOIRCS:
  case FC_INST_FMOS:
    if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
    }
    else{
      cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
    }
    break;
    
  case FC_INST_SPCAM:
    if(hg->dss_flip){
      cairo_rotate (cr,-M_PI*(gdouble)(90-hg->dss_pa)/180.);
    }
    else{
      cairo_rotate (cr,M_PI*(gdouble)(90-hg->dss_pa)/180.);
    }
    break;
  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    if(hg->dss_flip){
      cairo_rotate (cr,-M_PI*(gdouble)(270-hg->dss_pa)/180.);
    }
    else{
      cairo_rotate (cr,M_PI*(gdouble)(270-hg->dss_pa)/180.);
    }
    break;
  }
}

void cc_get_fc_inst (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_inst=n;
  }

  switch(hg->fc_inst){
  case FC_INST_HDS:
  case FC_INST_HDSAUTO:
  case FC_INST_HDSZENITH:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(HDS_SIZE));
    break;

  case FC_INST_IRCS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(IRCS_SIZE));
    break;

  case FC_INST_COMICS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(COMICS_SIZE));
    break;

  case FC_INST_FOCAS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(FOCAS_SIZE));
    break;

  case FC_INST_MOIRCS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(MOIRCS_SIZE));
    break;

  case FC_INST_FMOS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(FMOS_SIZE));
    break;
			     
  case FC_INST_SPCAM:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(SPCAM_SIZE));
    break;
			     
  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(HSC_SIZE));
    break;

  default:
    break;
  }

  if(hg->fc_inst==FC_INST_HDSAUTO){
    hg->dss_flip=FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),
				 hg->dss_flip);
    gtk_widget_set_sensitive(hg->fc_button_flip,FALSE);
  }
  else if(hg->fc_inst==FC_INST_HDSZENITH){
    hg->dss_flip=TRUE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),
				 hg->dss_flip);
    gtk_widget_set_sensitive(hg->fc_button_flip,FALSE);
  }
  else{
    gtk_widget_set_sensitive(hg->fc_button_flip,TRUE);
  }
}


void cc_get_fc_mode (GtkWidget *widget,  gpointer gdata)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_mode=n;

    set_fc_frame_col(hg);
    set_fc_mode(hg);
  }
}

void pdf_fc (typHOE *hg)
{
  hg->fc_output=FC_OUTPUT_PDF;

  if(flagFC){
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }

  hg->fc_output=FC_OUTPUT_WINDOW;
}

void do_print_fc (GtkWidget *widget, gpointer gdata)
{
  typHOE *hg;
  GtkPrintOperation *op; 
  GtkPrintOperationResult res; 
  //GError *error;
  //GtkWidget *error_dialog;
  //GtkPrintSettings *settings;

  hg=(typHOE *)gdata;

  op = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (op, 1); 
  //gtk_print_operation_set_unit (op, GTK_UNIT_MM); 
  g_signal_connect (op, "draw_page", G_CALLBACK (draw_page), (gpointer)hg); 
  //res = gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
  //			 GTK_WINDOW(hg->fc_main), &error);
  res = gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				 NULL,NULL);

  /*
  if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
    error_dialog = gtk_message_dialog_new (GTK_WINDOW(hg->fc_main),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   GTK_MESSAGE_ERROR,
					   GTK_BUTTONS_CLOSE,
					   "Error printing file:\n%s",
					   error->message);
    g_signal_connect (error_dialog, "response", 
		      G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show (error_dialog);
    g_error_free (error);
  }
  else if (res == GTK_PRINT_OPERATION_RESULT_APPLY)   {
    if (settings != NULL)
      g_object_unref (settings);
    settings = g_object_ref (gtk_print_operation_get_print_settings (op));
  }
  */
  g_object_unref(G_OBJECT(op));
}

static void draw_page (GtkPrintOperation *operation, 
		       GtkPrintContext *context,
		       gint page_nr, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;

  hg->fc_output=FC_OUTPUT_PRINT;
  hg->context=context;
  if(flagFC){
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }

  hg->fc_output=FC_OUTPUT_WINDOW;
  hg->context=NULL;
} 


#ifndef USE_WIN32
void dss_signal(int sig){
  pid_t child_pid=0;

  gtk_main_quit();

  do{
    int child_ret;
    child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
  } while((child_pid>0)||(child_pid!=-1));
}
#endif


glong get_file_size(gchar *fname)
{
  FILE *fp;
  long sz;

  fp = fopen( fname, "rb" );
  if( fp == NULL ){
    return -1;
  }

  fseek( fp, 0, SEEK_END );
  sz = ftell( fp );

  fclose( fp );
  return sz;
}


void set_fc_mode (typHOE *hg)
{
  switch(hg->fc_mode){
  case FC_STSCI_DSS1R:
  case FC_STSCI_DSS1B:
  case FC_STSCI_DSS2R:
  case FC_STSCI_DSS2B:
  case FC_STSCI_DSS2IR:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_STSCI);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file             =g_strconcat(hg->temp_dir,
					  G_DIR_SEPARATOR_S,
					  FC_FILE_GIF,NULL);
    
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_STSCI);
    
    if(hg->dss_src) g_free(hg->dss_src);
    switch(hg->fc_mode){
    case FC_STSCI_DSS1R:
      hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS1R);
      break;
    case FC_STSCI_DSS1B:
      hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS1B);
      break;
    case FC_STSCI_DSS2R:
      hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2R);
      break;
    case FC_STSCI_DSS2B:
      hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2B);
      break;
    case FC_STSCI_DSS2IR:
      hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2IR);
      break;
    }
    break;
    
  case FC_ESO_DSS1R:
  case FC_ESO_DSS2R:
  case FC_ESO_DSS2B:
  case FC_ESO_DSS2IR:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_ESO);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_ESO);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file             =g_strconcat(hg->temp_dir,
					  G_DIR_SEPARATOR_S,
					  FC_FILE_GIF,NULL);
    if(hg->dss_tmp) g_free(hg->dss_tmp);
    hg->dss_tmp=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FC_FILE_HTML,NULL);
    if(hg->dss_src) g_free(hg->dss_src);
    switch(hg->fc_mode){
    case FC_ESO_DSS1R:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS1R);
      break;
    case FC_ESO_DSS2R:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2R);
      break;
    case FC_ESO_DSS2B:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2B);
      break;
    case FC_ESO_DSS2IR:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2IR);
      break;
    }
    break;
    
  case FC_SKYVIEW_DSS1R:
  case FC_SKYVIEW_DSS1B:
  case FC_SKYVIEW_DSS2R:
  case FC_SKYVIEW_DSS2B:
  case FC_SKYVIEW_DSS2IR:
  case FC_SKYVIEW_2MASSJ:
  case FC_SKYVIEW_2MASSH:
  case FC_SKYVIEW_2MASSK:
  case FC_SKYVIEW_SDSSU:
  case FC_SKYVIEW_SDSSG:
  case FC_SKYVIEW_SDSSR:
  case FC_SKYVIEW_SDSSI:
  case FC_SKYVIEW_SDSSZ:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SKYVIEW);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SKYVIEW);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    if(hg->dss_tmp) g_free(hg->dss_tmp);
    hg->dss_tmp=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FC_FILE_HTML,NULL);
    if(hg->dss_src) g_free(hg->dss_src);
    switch(hg->fc_mode){
    case FC_SKYVIEW_DSS1R:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS1R);
      break;
    case FC_SKYVIEW_DSS1B:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS1B);
      break;
    case FC_SKYVIEW_DSS2R:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2R);
      break;
    case FC_SKYVIEW_DSS2B:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2B);
      break;
    case FC_SKYVIEW_DSS2IR:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2IR);
      break;
    case FC_SKYVIEW_2MASSJ:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSJ);
      break;
    case FC_SKYVIEW_2MASSH:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSH);
      break;
    case FC_SKYVIEW_2MASSK:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSK);
      break;
    case FC_SKYVIEW_SDSSU:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSU);
      break;
    case FC_SKYVIEW_SDSSG:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSG);
      break;
    case FC_SKYVIEW_SDSSR:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSR);
      break;
    case FC_SKYVIEW_SDSSI:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSI);
      break;
    case FC_SKYVIEW_SDSSZ:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSZ);
      break;
    }
    break;
    
  case FC_SDSS:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SDSS);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SDSS);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    break;
    
  case FC_SDSS13:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SDSS13);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SDSS13);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    break;
  }
}

void set_fc_frame_col(typHOE *hg){
  if((hg->fc_mode>=FC_SKYVIEW_DSS1R)&&(hg->fc_mode<=FC_SKYVIEW_SDSSZ)){
    gtk_widget_set_sensitive(hg->fc_frame_col,TRUE);
  }
  else{
    gtk_widget_set_sensitive(hg->fc_frame_col,FALSE);
  }
}

void show_fc_help (GtkWidget *widget, gpointer gdata)
{
  GtkWidget *dialog, *label, *button, *pixmap, *vbox, *hbox, *table;
#ifdef USE_GTK2
  GdkPixbuf *icon, *pixbuf;
#endif  

  while (my_main_iteration(FALSE));
  gdk_flush();

  dialog = gtk_dialog_new();
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"Sky Monitor : Help for Finding Chart");

  my_signal_connect(dialog,"destroy",
		    close_fc_help, 
		    GTK_WIDGET(dialog));
  
  table = gtk_table_new(2,5,FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     table,FALSE, FALSE, 0);

  icon = gdk_pixbuf_new_from_inline(sizeof(icon_dl), icon_dl, 
				    FALSE, NULL);
  pixbuf=gdk_pixbuf_scale_simple(icon,16,16,GDK_INTERP_BILINEAR);

  pixmap = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(icon);
  g_object_unref(pixbuf);
  gtk_table_attach (GTK_TABLE(table), pixmap, 0, 1, 0, 1,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);
  //g_object_unref(pixmap);

  label = gtk_label_new ("  Download new image and redraw w/instrument");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 1, 2, 0, 1,
		    GTK_FILL,GTK_SHRINK,0,0);
  

  pixmap=gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
  gtk_table_attach (GTK_TABLE(table), pixmap, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);
  //g_object_unref(pixmap);

  label = gtk_label_new ("  Redraw selected instrument and PA");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 1, 2, 1, 2,
		    GTK_FILL,GTK_SHRINK,0,0);


  label = gtk_label_new ("<wheel-scroll>");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 2, 3,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Enlarge view around cursor (upto x5)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 1, 2, 2, 3,
		    GTK_FILL,GTK_SHRINK,0,0);
  

  label = gtk_label_new ("<shift>+<wheel-scroll>");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 3, 4,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Rotate position angle (w/5 deg step)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 1, 2, 3, 4,
		    GTK_FILL,GTK_SHRINK,0,0);
  

  label = gtk_label_new ("<left-click>");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 4, 5,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Measure the distance between 2-points (The 3rd click to clear)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 1, 2, 4, 5,
		    GTK_FILL,GTK_SHRINK,0,0);
  

  label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label,FALSE, FALSE, 0);


  label = gtk_label_new ("Please use SkyView or SDSS for large FOV (> 60\') to save the traffic.");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label,FALSE, FALSE, 0);

  label = gtk_label_new ("ESO and STSci cannot change their pixel scale.");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label,FALSE, FALSE, 0);

  label = gtk_label_new ("Because the maximum pixel sizes for SkyView (1000pix) and SDSS (2000pix) are limited,");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label,FALSE, FALSE, 0);

  label = gtk_label_new ("the downloaded FC image for large FOV (> 13\' for SDSS) should be degraded from the original.");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label,FALSE, FALSE, 0);



  button=gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button,FALSE,FALSE,0);
  my_signal_connect(button,"pressed",
		    close_fc_help, 
		    GTK_WIDGET(dialog));

  gtk_widget_show_all(dialog);

  gtk_main();

}

static void close_fc_help(GtkWidget *w, GtkWidget *dialog)
{
  gtk_main_quit();
  gtk_widget_destroy(dialog);
}

#ifndef USE_WIN32
static void cancel_fcdb(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  pid_t child_pid=0;

  hg=(typHOE *)gdata;

  if(fcdb_pid){
    kill(fcdb_pid, SIGKILL);
    gtk_main_quit();

    do{
      int child_ret;
      child_pid=waitpid(fcdb_pid, &child_ret,WNOHANG);
    } while((child_pid>0)||(child_pid!=-1));
 
    fcdb_pid=0;
  }
}
#endif

void fcdb_dl(typHOE *hg)
{
  GtkTreeIter iter;
  gchar tmp[2048];
  GtkWidget *dialog, *vbox, *label, *button;
#ifndef USE_WIN32
  static struct sigaction act;
#endif
  guint timer;
  
  if(flag_getFCDB) return;
  flag_getFCDB=TRUE;

  while (my_main_iteration(FALSE));
  gdk_flush();

  dialog = gtk_dialog_new();
  
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"Sky Monitor : Message");
  gtk_window_set_decorated(GTK_WINDOW(dialog),TRUE);
  
#ifdef USE_GTK2  
  gtk_dialog_set_has_separator(GTK_DIALOG(dialog),TRUE);
#endif
  
  label=gtk_label_new("Searching objects in SIMBAD ...");

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,TRUE,TRUE,0);
  gtk_widget_show(label);
  
  hg->pbar=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hg->pbar,TRUE,TRUE,0);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hg->pbar), 
				    GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(hg->pbar),0.05);
  gtk_widget_show(hg->pbar);
  
  unlink(hg->std_file);
  
  timer=g_timeout_add(100, 
		      (GSourceFunc)progress_timeout,
		      (gpointer)hg);
  
  hg->plabel=gtk_label_new("Searching objects in SIMBAD ...");
  gtk_misc_set_alignment (GTK_MISC (hg->plabel), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     hg->plabel,FALSE,FALSE,0);
  
#ifndef USE_WIN32
#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#else
  button=gtk_button_new_with_label("Cancel");
#endif
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button,FALSE,FALSE,0);
  my_signal_connect(button,"pressed",
		    cancel_fcdb, 
		    (gpointer)hg);
#endif
  
  gtk_widget_show_all(dialog);
  
#ifndef USE_WIN32
  act.sa_handler=fcdb_signal;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  if(sigaction(SIGUSR1, &act, NULL)==-1)
    fprintf(stderr,"Error in sigaction (SIGUSR1).\n");
#endif
  
  gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
  
  get_fcdb(hg);
  gtk_main();

  gtk_timeout_remove(timer);
  gtk_widget_destroy(dialog);

  flag_getFCDB=FALSE;
}



void fcdb_item2 (typHOE *hg)
{
  gdouble ra_0, dec_0, d_ra0, d_dec0;
  gchar tmp[2048], *mag_str, *otype_str;
  struct lnh_equ_posn hobject;
  struct ln_equ_posn object;
  struct ln_equ_posn object_prec;

  hg->fcdb_i=hg->dss_i;

  ra_0=hg->obj[hg->fcdb_i].ra;
  hobject.ra.hours=(gint)(ra_0/10000);
  ra_0=ra_0-(gdouble)(hobject.ra.hours)*10000;
  hobject.ra.minutes=(gint)(ra_0/100);
  hobject.ra.seconds=ra_0-(gdouble)(hobject.ra.minutes)*100;
  
  if(hg->obj[hg->fcdb_i].dec<0){
    hobject.dec.neg=1;
    dec_0=-hg->obj[hg->fcdb_i].dec;
  }
  else{
    hobject.dec.neg=0;
    dec_0=hg->obj[hg->fcdb_i].dec;
  }
  hobject.dec.degrees=(gint)(dec_0/10000);
  dec_0=dec_0-(gfloat)(hobject.dec.degrees)*10000;
  hobject.dec.minutes=(gint)(dec_0/100);
  hobject.dec.seconds=dec_0-(gfloat)(hobject.dec.minutes)*100;

  ln_hequ_to_equ (&hobject, &object);
  ln_get_equ_prec2 (&object, 
		    get_julian_day_of_epoch(hg->obj[hg->fcdb_i].epoch),
		    JD2000, &object_prec);

  switch(hg->fcdb_band){
  case FCDB_BAND_NOP:
    mag_str=g_strdup("%0D%0A");
    break;
  case FCDB_BAND_U:
    mag_str=g_strdup_printf("%%26Umag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_B:
    mag_str=g_strdup_printf("%%26Bmag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_V:
    mag_str=g_strdup_printf("%%26Vmag<%d",hg->fcdb_mag);
    break; 
  case FCDB_BAND_R:
    mag_str=g_strdup_printf("%%26Rmag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_I:
    mag_str=g_strdup_printf("%%26Imag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_J:
    mag_str=g_strdup_printf("%%26Jmag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_H:
    mag_str=g_strdup_printf("%%26Hmag<%d",hg->fcdb_mag);
    break;
  case FCDB_BAND_K:
    mag_str=g_strdup_printf("%%26Kmag<%d",hg->fcdb_mag);
    break;
  }

  switch(hg->fcdb_otype){
  case FCDB_OTYPE_ALL:
    otype_str=g_strdup("%0D%0A");
    break;
  case FCDB_OTYPE_STAR:
    otype_str=g_strdup("%26maintypes%3Dstar");
    break;
  case FCDB_OTYPE_ISM:
    otype_str=g_strdup("%26maintypes%3Dism");
    break;
  case FCDB_OTYPE_GALAXY:
    otype_str=g_strdup("%26maintypes%3Dgalaxy");
    break;
  case FCDB_OTYPE_QSO:
    otype_str=g_strdup("%26maintypes%3Dqso");
    break;
  case FCDB_OTYPE_GAMMA:
    otype_str=g_strdup("%26maintypes%3Dgamma");
    break;
  case FCDB_OTYPE_X:
    otype_str=g_strdup("%26maintypes%3DX");
    break;
  case FCDB_OTYPE_IR:
    otype_str=g_strdup("%26maintypes%3DIR");
    break;
  case FCDB_OTYPE_RADIO:
    otype_str=g_strdup("%26maintypes%3Dradio");
    break;
  }
  
  hg->fcdb_d_ra0=object_prec.ra;
  hg->fcdb_d_dec0=object_prec.dec;
  if(hg->fcdb_host) g_free(hg->fcdb_host);
  hg->fcdb_host=g_strdup(STDDB_HOST_SIMBAD);
  if(hg->fcdb_path) g_free(hg->fcdb_path);
  //hg->fcdb_path=g_strdup_printf(FCDB_PATH,hg->fcdb_d_ra0,hg->fcdb_d_dec0,
  //				(gdouble)hg->dss_arcmin/2.*1.2,MAX_FCDB);
  if(hg->fcdb_d_dec0>0){
    hg->fcdb_path=g_strdup_printf(FCDB_PATH,hg->fcdb_d_ra0,
				  "%2B",hg->fcdb_d_dec0,
				  (gdouble)hg->dss_arcmin,
				  (gdouble)hg->dss_arcmin,
				  mag_str,otype_str,
				  MAX_FCDB);
  }
  else{
    hg->fcdb_path=g_strdup_printf(FCDB_PATH,hg->fcdb_d_ra0,
				  "%2D",-hg->fcdb_d_dec0,
				  (gdouble)hg->dss_arcmin,
				  (gdouble)hg->dss_arcmin,
				  mag_str,otype_str,
				  MAX_FCDB);
  }
  g_free(mag_str);
  g_free(otype_str);

  if(hg->fcdb_file) g_free(hg->fcdb_file);
  hg->fcdb_file=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FCDB_FILE_XML,NULL);

  fcdb_dl(hg);

  fcdb_vo_parse(hg);

  if(flagTree) fcdb_make_tree(NULL, hg);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fcdb_button),
			       TRUE);
  hg->fcdb_flag=TRUE;

  if(flagFC)  draw_fc_cairo(hg->fc_dw,NULL, (gpointer)hg);
}

static void fcdb_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  
  fcdb_item2(hg);
}


void fcdb_tree_update_azel_item(typHOE *hg, 
				GtkTreeModel *model, 
				GtkTreeIter iter, 
				gint i_list)
{
  gchar tmp[24];
  gint i;
  gdouble s_rt=-1;

  // Num/Name
  gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		      COLUMN_FCDB_NUMBER,
		      i_list+1,
		      -1);
  gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		      COLUMN_FCDB_NAME,
		      hg->fcdb[i_list].name,
		      -1);

  // RA
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_RA, hg->fcdb[i_list].ra, -1);
  
  // DEC
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_DEC, hg->fcdb[i_list].dec, -1);

  // SEP
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_SEP, hg->fcdb[i_list].sep, -1);

  // O-Type
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_OTYPE, hg->fcdb[i_list].otype, -1);

  // SpType
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_SP, hg->fcdb[i_list].sp, -1);

  // UBVRIJHK
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
		     COLUMN_FCDB_U, hg->fcdb[i_list].u,
		     COLUMN_FCDB_B, hg->fcdb[i_list].b,
		     COLUMN_FCDB_V, hg->fcdb[i_list].v,
		     COLUMN_FCDB_R, hg->fcdb[i_list].r,
		     COLUMN_FCDB_I, hg->fcdb[i_list].i,
		     COLUMN_FCDB_J, hg->fcdb[i_list].j,
		     COLUMN_FCDB_H, hg->fcdb[i_list].h,
		     COLUMN_FCDB_K, hg->fcdb[i_list].k,
		     -1);
}


void fcdb_make_tree(GtkWidget *widget, gpointer gdata){
  gint i;
  typHOE *hg;
  GtkTreeModel *model;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->fcdb_tree));

  gtk_list_store_clear (GTK_LIST_STORE(model));
  
  while (my_main_iteration(FALSE));
  gdk_flush();

  for (i = 0; i < hg->fcdb_i_max; i++){
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);
    fcdb_tree_update_azel_item(hg, GTK_TREE_MODEL(model), iter, i);
  }

  if(hg->fcdb_label_text) g_free(hg->fcdb_label_text);
  hg->fcdb_label_text
    =g_strdup_printf("Objects around [%d-%d] %s (%d objects found)",
		     hg->obj[hg->fcdb_i].ope+1,hg->obj[hg->fcdb_i].ope_i+1,
		     hg->obj[hg->fcdb_i].name,hg->fcdb_i_max);
  gtk_label_set_text(GTK_LABEL(hg->fcdb_label), hg->fcdb_label_text);

  gtk_notebook_set_current_page (GTK_NOTEBOOK(hg->obj_note),2);
}

void fcdb_clear_tree(typHOE *hg){
  GtkTreeModel *model;

  if(hg->dss_i!=hg->fcdb_i){
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->fcdb_tree));

    gtk_list_store_clear (GTK_LIST_STORE(model));
    hg->fcdb_i_max=0;
  }

}


gdouble current_yrs(typHOE *hg){
  double JD;
  struct ln_zonedate zonedate;

  zonedate.years=hg->skymon_year;
  zonedate.months=hg->skymon_month;
  zonedate.days=hg->skymon_day;
  zonedate.hours=hg->skymon_hour;
  zonedate.minutes=hg->skymon_min;
  zonedate.seconds=0;
  zonedate.gmtoff=(long)hg->obs_timezone*3600;
  JD = ln_get_julian_local_date(&zonedate);
  return((JD-JD2000)/365.25);
}
      
static void
fcdb_toggle (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  hg->fcdb_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if(flagFC)  draw_fc_cairo(hg->fc_dw,NULL, (gpointer)hg);
}

