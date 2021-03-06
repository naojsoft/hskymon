//    HSkymon
//      adc.c  --- Atmospheric Dispersion Chart
//   
//                                           2012.10.22  A.Tajitsu


#include"main.h"    // 設定ヘッダ
#include"version.h"
#include <cairo.h>

void adc_item();
void adc_item2();
void cc_get_adc_inst();
void do_adc();
void create_adc_dialog();
void close_adc();
gboolean draw_adc_cairo();
static void refresh_adc();
gboolean update_adc();


extern void cc_get_toggle();
extern void cc_get_adj();
extern void cc_get_adj_double();
extern void my_entry_set_width_chars();

extern GtkWidget* gtkut_button_new_from_stock();
extern void get_current_obs_time();

extern void calcpa2_main();

gboolean flagADC=FALSE;


void adc_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  adc_item2(hg);
}

void adc_item2 (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->tree));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->tree));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
    gint i, i_list;
    GtkTreePath *path;
    
    path = gtk_tree_model_get_path (model, &iter);
    //i = gtk_tree_path_get_indices (path)[0];
    gtk_tree_model_get (model, &iter, COLUMN_OBJ_NUMBER, &i, -1);
    i--;

    hg->plot_i=i;

    do_adc(widget,(gpointer)hg);

    gtk_tree_path_free (path);
  }
}


void cc_get_adc_inst (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->adc_inst=n;
  }

  if(hg->adc_inst==ADC_INST_HDSAUTO){
    hg->adc_flip=FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->adc_button_flip),
				 hg->adc_flip);
    gtk_widget_set_sensitive(hg->adc_button_flip,FALSE);
  }
  else if(hg->adc_inst==ADC_INST_HDSZENITH){
    hg->adc_flip=TRUE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->adc_button_flip),
				 hg->adc_flip);
    gtk_widget_set_sensitive(hg->adc_button_flip,FALSE);
  }
  else{
    gtk_widget_set_sensitive(hg->adc_button_flip,TRUE);
  }
}
void do_adc(typHOE *hg){
  if(flagADC){
    gdk_window_deiconify(hg->adc_main->window);
    gdk_window_raise(hg->adc_main->window);
    draw_adc_cairo(hg->adc_dw,NULL,
		   (gpointer)hg);
    return;
  }
  else{
    flagADC=TRUE;
  }
  
  create_adc_dialog(hg);
}

void create_adc_dialog(typHOE *hg)
{
  GtkWidget *vbox, *hbox, *hbox1, *hbox2, *table;
  GtkWidget *frame, *check, *label, *button, *spinner;
  GtkAdjustment *adj;
  GtkWidget *menubar;
  GdkPixbuf *icon;

  // Win構築は重いので先にExposeイベント等をすべて処理してから
  while (my_main_iteration(FALSE));
  gdk_flush();

  hg->adc_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(hg->adc_main), "Sky Monitor : AD Chart");
  
  my_signal_connect(hg->adc_main,
		    "destroy",
		    close_adc, 
		    (gpointer)hg);

  gtk_widget_set_app_paintable(hg->adc_main, TRUE);
  
  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->adc_main), vbox);

  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


  frame = gtk_frame_new ("Action");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtk_table_new(2,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_CANCEL);
#else
  button = gtk_button_new_with_label ("Close");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (close_adc), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Close");
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_REFRESH);
#else
  button = gtk_button_new_with_label ("Redraw");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (refresh_adc), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 1, 2, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Redraw");
#endif


  frame = gtk_frame_new ("Instrument");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtk_table_new(3,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ImR / InR",
		       1, ADC_INST_IMR, -1);
    if(hg->adc_inst==ADC_INST_IMR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS (w/oImR)",
		       1, ADC_INST_HDSAUTO, -1);
    if(hg->adc_inst==ADC_INST_HDSAUTO) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS (Zenith)",
		       1, ADC_INST_HDSZENITH, -1);
    if(hg->adc_inst==ADC_INST_HDSZENITH) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 0, 1, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_adc_inst, (gpointer)hg);
  }

  frame = gtk_frame_new ("PA [deg]");
  gtk_table_attach (GTK_TABLE(table), frame, 1, 2, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  hg->adc_adj_pa = (GtkAdjustment *)gtk_adjustment_new(hg->adc_pa,
							   -360, 360,
							   1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->adc_adj_pa, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (hg->adc_adj_pa, "value_changed",
		     cc_get_adj,
		     &hg->adc_pa);

  hg->adc_button_flip=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (hg->adc_button_flip), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),hg->adc_button_flip,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->adc_button_flip),hg->adc_flip);
  my_signal_connect(hg->adc_button_flip,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->adc_flip);



  frame = gtk_frame_new ("Slit [\"]");
  gtk_table_attach (GTK_TABLE(table), frame, 2, 3, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->adc_slit_width,
					    0.1, 4.0, 
					    0.1,0.1,0);
  spinner =  gtk_spin_button_new (adj, 2, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,1);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj_double,
		     &hg->adc_slit_width);


  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new ("Wavelength [A]");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  label=gtk_label_new("Target");
  gtk_box_pack_start(GTK_BOX(hbox2),label,FALSE,FALSE,1);
  
  adj = (GtkAdjustment *)gtk_adjustment_new(hg->wave1,
					    2800, 30000, 
					    100.0,100.0,0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 FALSE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,1);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),5);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->wave1);


  label=gtk_label_new("   Guide");
  gtk_box_pack_start(GTK_BOX(hbox2),label,FALSE,FALSE,1);
  
  adj = (GtkAdjustment *)gtk_adjustment_new(hg->wave0,
					    2800, 30000, 
					    100.0,100.0,0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 FALSE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,1);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),5);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->wave0);


  frame = gtk_frame_new ("Seeing [\"]");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);


  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->adc_seeing,
					    0.1, 2.0, 
					    0.1,0.1,0);
  spinner =  gtk_spin_button_new (adj, 2, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,1);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj_double,
		     &hg->adc_seeing);

  frame = gtk_frame_new ("Plot [\"]");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);


  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->adc_size,
					    1.0, 15.0, 
					    0.1,0.1,0);
  spinner =  gtk_spin_button_new (adj, 2, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,1);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj_double,
		     &hg->adc_size);


  // Drawing Area
  hg->adc_dw = gtk_drawing_area_new();
  gtk_widget_set_size_request (hg->adc_dw, ADC_WIDTH, ADC_HEIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), hg->adc_dw, TRUE, TRUE, 0);
  gtk_widget_set_app_paintable(hg->adc_dw, TRUE);
  gtk_widget_show(hg->adc_dw);

  screen_changed(hg->adc_dw,NULL,NULL);

  my_signal_connect(hg->adc_dw, 
		    "expose-event", 
		    draw_adc_cairo,
		    (gpointer)hg);

  gtk_widget_show_all(hg->adc_main);

  hg->adc_timer=g_timeout_add(PLOT_INTERVAL, 
			      (GSourceFunc)update_adc,
			      (gpointer)hg);

  gdk_window_raise(hg->adc_main->window);

  gdk_flush();
}


void close_adc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;

  if(hg->adc_timer!=-1)
    gtk_timeout_remove(hg->adc_timer);

  gtk_widget_destroy(GTK_WIDGET(hg->adc_main));
  flagADC=FALSE;
}


gboolean draw_adc_cairo(GtkWidget *widget, 
		       GdkEventExpose *event, 
		       gpointer userdata){
  cairo_t *cr;
  cairo_surface_t *surface;
  typHOE *hg;
  cairo_text_extents_t extents;
  double x,y;
  GdkPixmap *pixmap_adc;
  int width, height;

  gdouble ra_0, dec_0;
  gchar tmp[2048];
  gfloat x_ccd, y_ccd, gap_ccd;
  //struct ln_hms ra_hms;
  //struct ln_dms dec_dms;
  gdouble size, scale;
  gdouble z_pa, ad;
  gboolean flag_rise;
  
  struct lnh_equ_posn hobject;
  
  if(!flagADC) return (FALSE);

  hg=(typHOE *)userdata;
  

  width= widget->allocation.width;
  height= widget->allocation.height;
  if(width<=1){
    gtk_window_get_size(GTK_WINDOW(hg->adc_main), &width, &height);
  }
  
  if(width>height){
    size=height;
  }
  else{
    size=width;
  }
  scale=hg->adc_size/size;
  
  pixmap_adc = gdk_pixmap_new(widget->window,
			     size,
			     size,
			     -1);
  
  cr = gdk_cairo_create(pixmap_adc);

  cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);

  
  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);


  cairo_rectangle(cr, 0,0,
		  size,
		  size);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
  cairo_fill(cr);

  flag_rise=FALSE;
  if(hg->skymon_mode==SKYMON_SET){
    if(hg->obj[hg->plot_i].s_el>0) flag_rise=TRUE;
  }
  else{
    if(hg->obj[hg->plot_i].c_el>0) flag_rise=TRUE;
  }

  // Slit
  cairo_set_source_rgba(cr, 1.0, 0.4, 0.0, 0.3);  
  cairo_move_to(cr, size/2,0);
  cairo_set_line_width (cr, hg->adc_slit_width/scale);
  cairo_line_to(cr, size/2,size);
  cairo_stroke(cr);

  cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_objsz);
  cairo_set_source_rgba(cr, 1.0, 0.4, 0.0, 1.0);
  cairo_move_to(cr, size/2+hg->adc_slit_width/scale/2.+5.,5.);
  sprintf(tmp,"%.2lf\" Slit",hg->adc_slit_width);
  cairo_text_extents (cr,tmp, &extents);
  cairo_rel_move_to(cr,0, extents.height);
  cairo_show_text(cr, tmp);


  // Direction
  {
    cairo_save (cr);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    cairo_text_extents (cr, "N", &extents);

    cairo_translate (cr, 50, 50);


    if(hg->adc_inst==ADC_INST_HDSAUTO){
      if(hg->skymon_mode==SKYMON_SET){
	gtk_adjustment_set_value(hg->adc_adj_pa, 
				 (gdouble)((int)hg->obj[hg->plot_i].s_hpa));
      }
      else{
	gtk_adjustment_set_value(hg->adc_adj_pa, 
				 (gdouble)((int)hg->obj[hg->plot_i].c_hpa));
      }
    }
    else if(hg->adc_inst==ADC_INST_HDSZENITH){
      if(hg->skymon_mode==SKYMON_SET){
	gtk_adjustment_set_value(hg->adc_adj_pa, 
				 (gdouble)((int)hg->obj[hg->plot_i].s_pa));
      }
      else{
	gtk_adjustment_set_value(hg->adc_adj_pa, 
				 (gdouble)((int)hg->obj[hg->plot_i].c_pa));
      }
    }

    if(hg->adc_flip){
      cairo_rotate (cr,-M_PI*(gdouble)hg->adc_pa/180.);
    }
    else{
      cairo_rotate (cr,M_PI*(gdouble)hg->adc_pa/180.);
    }

    cairo_move_to(cr,
		  -extents.width/2,
		  -32);
    cairo_show_text(cr,"N");
    if(hg->adc_flip){
      cairo_text_extents (cr, "W", &extents);
      cairo_move_to(cr,
		    -32-extents.width,
		    +extents.height/2);
      cairo_show_text(cr,"W");
    }
    else{
      cairo_text_extents (cr, "E", &extents);
      cairo_move_to(cr,
		    -32-extents.width,
		    +extents.height/2);
      cairo_show_text(cr,"E");
    }

    cairo_set_line_width (cr, 1.5);
    cairo_move_to(cr,
		  0,
		  -30);
    cairo_line_to(cr, 0, 0);
    cairo_line_to(cr,
		  -30, 0);
    cairo_stroke(cr);

    if(hg->adc_flip){
      cairo_move_to(cr,0,0);
      cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_text_extents (cr, "(flipped)", &extents);
      cairo_rel_move_to(cr,-extents.width/2.,extents.height+5);
      cairo_show_text(cr,"(flipped)");
    }

    
    cairo_restore(cr);
  }  

  // Size & Time
  {
    gint year, month, day, hour, min;
    gdouble sec;
    struct tm *tmpt;

    cairo_save(cr);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    sprintf(tmp,"%.1lfx%.1lf arcsec",size*scale,size*scale);
    
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,
		  size-extents.width-5,
		  extents.height+5);
    cairo_show_text(cr,tmp);

    cairo_translate (cr, size-5, extents.height+5);

    if(hg->skymon_mode==SKYMON_SET){
      year=hg->skymon_year;
      month=hg->skymon_month;
      day=hg->skymon_day;
      
      hour=hg->skymon_hour;
      min=hg->skymon_min;
      sec=0;
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

    sprintf(tmp,"%02d/%02d/%04d",month,day,year);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,-extents.width,+extents.height+5);
    cairo_show_text(cr, tmp);

    cairo_translate (cr, 0, extents.height+5);

    sprintf(tmp,"%s=%02d:%02d",hg->obs_tzname,hour,min);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,-extents.width,+extents.height+5);
    cairo_show_text(cr, tmp);

    cairo_restore(cr);
  }

  // Object Name
  {
    ra_0=hg->obj[hg->plot_i].ra;
    hobject.ra.hours=(gint)(ra_0/10000);
    ra_0=ra_0-(gdouble)(hobject.ra.hours)*10000;
    hobject.ra.minutes=(gint)(ra_0/100);
    hobject.ra.seconds=ra_0-(gdouble)(hobject.ra.minutes)*100;

    if(hg->obj[hg->plot_i].dec<0){
      hobject.dec.neg=1;
      dec_0=-hg->obj[hg->plot_i].dec;
    }
    else{
      hobject.dec.neg=0;
      dec_0=hg->obj[hg->plot_i].dec;
    }
    hobject.dec.degrees=(gint)(dec_0/10000);
    dec_0=dec_0-(gfloat)(hobject.dec.degrees)*10000;
    hobject.dec.minutes=(gint)(dec_0/100);
    hobject.dec.seconds=dec_0-(gfloat)(hobject.dec.minutes)*100;

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    cairo_move_to(cr,5,size-5);
    sprintf(tmp,"RA=%02d:%02d:%05.2lf  Dec=%s%02d:%02d:%05.2lf (%.1lf)",
	    hobject.ra.hours,hobject.ra.minutes,
	    hobject.ra.seconds,
	    (hobject.dec.neg) ? "-" : "+", 
	    hobject.dec.degrees, hobject.dec.minutes,
	    hobject.dec.seconds,
	    hg->obj[hg->plot_i].epoch);
    cairo_text_extents (cr, tmp, &extents);
    cairo_show_text(cr,tmp);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr,5, size-5-extents.height-5);
    cairo_show_text(cr,hg->obj[hg->plot_i].name);
  }


  // ADPA
  if(hg->skymon_mode==SKYMON_SET){
    z_pa=hg->obj[hg->plot_i].s_pa;
  }
  else{
    z_pa=hg->obj[hg->plot_i].c_pa;
  }
  if(flag_rise){
    cairo_save (cr);

    cairo_translate (cr, size/2., size/2.);
    cairo_rotate (cr,M_PI*(z_pa-(gdouble)hg->adc_pa)/180.);

    cairo_set_source_rgba(cr, 0.0, 0.6, 0.0, 0.6);
    
    cairo_set_line_width (cr, 2);
    cairo_move_to(cr,0, -size);
    cairo_line_to(cr, 0, size);
    cairo_stroke(cr);

    cairo_set_source_rgba(cr, 0.0, 0.6, 0.0, 1.0);
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.1);
    cairo_rotate (cr,M_PI/2.);
    sprintf(tmp,"Zenith = %.0lf", z_pa);
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr, +size/2.-extents.height-extents.width-5,
		  -extents.height/2.);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);
  }

  // Guide Star
  if(hg->skymon_mode==SKYMON_SET){
    ad=hg->obj[hg->plot_i].s_ad;
  }
  else{
    ad=hg->obj[hg->plot_i].c_ad;
  }
  if(hg->wave0 < hg->wave1){ // Guide < Target
    ad=-ad;
  }
  if(flag_rise){
    cairo_save(cr);

    cairo_translate (cr, size/2., size/2.);

    if(ad>0){
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.3);
    }
    else{
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.3);
    }
    cairo_arc(cr,0, 0, hg->adc_seeing/scale/2.,0, M_PI*2);
    cairo_fill(cr);

    if(ad>0){
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
    }
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz);
    cairo_move_to(cr, (hg->adc_seeing/2./sqrt(2.0))/scale+5,
		  (hg->adc_seeing/2./sqrt(2.0))/scale+5);
    sprintf(tmp,"Guide Star (%dA)",hg->wave0);
    cairo_show_text(cr,tmp);
    
    cairo_restore(cr);
  }

  // Obs Star
  if(flag_rise){
    cairo_save(cr);

    cairo_translate (cr, size/2., size/2.);
    cairo_rotate (cr,M_PI*(z_pa-(gdouble)hg->adc_pa)/180.);
 
    if(ad>0){
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.3);
    }
    else{
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.3);
    }
    cairo_arc(cr,0, -ad/scale, hg->adc_seeing/scale/2.,0, M_PI*2);
    cairo_fill(cr);

    if(ad>0){
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    }
    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz);
    sprintf(tmp,"Target (%dA)",hg->wave1);
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr, -(hg->adc_seeing/2./sqrt(2.0))/scale-5
		  -extents.width,
		  (hg->adc_seeing/2./sqrt(2.0))/scale+5-ad/scale);
    cairo_show_text(cr,tmp);

    cairo_move_to(cr, -(hg->adc_seeing/2./sqrt(2.0))/scale-5
		  -extents.width,
		  (hg->adc_seeing/2./sqrt(2.0))/scale+5-ad/scale
		  -extents.height-2);
    sprintf(tmp,"d = %.2lf\"",fabs(ad));
    cairo_show_text(cr,tmp);

    cairo_restore(cr);
  }


  if(!flag_rise){
    cairo_save(cr);

    cairo_translate(cr,size/2.,size/2.);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*2);
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    sprintf(tmp,"Object is below the horizon!!");
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,-extents.width/2., -extents.height/2.);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);
  }
  else if(ad==100){

    cairo_translate(cr,size/2.,size/2.);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.5);
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    sprintf(tmp,"Object is too close to the horizon!!");
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,-extents.width/2., -extents.height/2.);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);
  }
  else if(fabs(ad)>hg->adc_size/2.){

    cairo_translate(cr,size/2.,size/2.);

    cairo_select_font_face (cr, hg->fontfamily, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_objsz*1.5);
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    sprintf(tmp,"Atmopheric Dispersion (%.1lf\") is out of range!!",fabs(ad));
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,-extents.width/2., -extents.height/2.);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);
  }
    

  cairo_destroy(cr);
  
    
  if(width>height){
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      pixmap_adc,
		      0,0,(width-height)/2,0,
		      size,
		      size);
  }
  else{
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      pixmap_adc,
		      0,0,0,(height-width)/2,
		      size,
		      size);
  }
  
  g_object_unref(G_OBJECT(pixmap_adc));

  return TRUE;

}


static void refresh_adc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagADC){
    if(hg->skymon_mode==SKYMON_SET){
      calcpa2_skymon(hg);
    }
    else{
      calcpa2_main(hg);
    }
    draw_adc_cairo(hg->adc_dw,NULL,
		   (gpointer)hg);
  }
}

gboolean update_adc (gpointer data){
  typHOE *hg = (typHOE *)data;

  if(flagADC){
    draw_adc_cairo(hg->adc_dw,NULL,
		   (gpointer)hg);
  }

  return(TRUE);
}
