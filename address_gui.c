/* $Id: address_gui.c,v 1.133 2007/10/23 18:29:14 judd Exp $ */

/*******************************************************************************
 * address_gui.c
 * A module of J-Pilot http://jpilot.org
 *
 * Copyright (C) 1999-2006 by Judd Montgomery
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ******************************************************************************/

#include "config.h"
#include "i18n.h"
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <time.h>
/* For open, read */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "address.h"
#include "log.h"
#include "prefs.h"
#include "print.h"
#include "password.h"
#include "export.h"
#include <pi-dlp.h>
#include "stock_buttons.h"
#include "jp-pi-contact.h"

/* There are a large number of calls to gtk_text_insert in the code.  To
 * add ifdef/endif blocks around all of them would make the code unreadable.
 * Instead, I use a macro substitution to convert old GTK 1.X calls to
 * GTK 2.X calls. */
#ifdef ENABLE_GTK2
#define GTK_TEXT(arg1) GTK_TEXT_BUFFER(gtk_txt_buf_ ## arg1)
#define gtk_text_insert(buffer,arg2,arg3,arg4,string,length) gtk_text_buffer_insert_at_cursor(buffer,string,length)
#endif

#define CONNECT_SIGNALS 400
#define DISCONNECT_SIGNALS 401
#define NUM_MENU_ITEM1 8
#define NUM_MENU_ITEM2 8
#define NUM_ADDRESS_CAT_ITEMS 16
#define NUM_ADDRESS_ENTRIES 19
//#define NUM_ADDRESS_LABELS 22
//#define NUM_PHONE_ENTRIES 5
#define NUM_PHONE_ENTRIES 7

/* There are 3 extra fields for Japanese Palm OS's KANA extension in address book.
 * Kana means 'pronounce of name'.
 */
#define NUM_ADDRESS_EXT_ENTRIES 3

#define ADDRESS_GUI_LABEL_TEXT 2
#define ADDRESS_GUI_DIAL_SHOW_MENU_TEXT 3
#define ADDRESS_GUI_IM_MENU_TEXT 4
#define ADDRESS_GUI_ADDR_MENU_TEXT 5
#define ADDRESS_GUI_WEBSITE 6
#define ADDRESS_GUI_BIRTHDAY 7

#define PHOTO_X 139
#define PHOTO_Y 144

/*
 * This describes how to draw a GUI entry for each field in an address record
 */
typedef struct {
   int record_field;
   int notebook_page;
   int type;
} address_schema_entry;

static address_schema_entry *schema;

#define NUM_IMS 2
#define NUM_ADDRESSES 3
int schema_size;
static address_schema_entry contact_schema[NUM_CONTACT_FIELDS]={
     {contLastname, 0, ADDRESS_GUI_LABEL_TEXT},
     {contFirstname, 0, ADDRESS_GUI_LABEL_TEXT},
     {contCompany, 0, ADDRESS_GUI_LABEL_TEXT},
     {contTitle, 0, ADDRESS_GUI_LABEL_TEXT},
     {contPhone1, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone2, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone3, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone4, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone5, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone6, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone7, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contIM1, 0, ADDRESS_GUI_IM_MENU_TEXT},
     {contIM2, 0, ADDRESS_GUI_IM_MENU_TEXT},
     {contWebsite, 0, ADDRESS_GUI_WEBSITE},
     {contAddress1, 1, ADDRESS_GUI_ADDR_MENU_TEXT},
     {contCity1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contState1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contZip1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contCountry1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contAddress2, 2, ADDRESS_GUI_ADDR_MENU_TEXT},
     {contCity2, 2, ADDRESS_GUI_LABEL_TEXT},
     {contState2, 2, ADDRESS_GUI_LABEL_TEXT},
     {contZip2, 2, ADDRESS_GUI_LABEL_TEXT},
     {contCountry2, 2, ADDRESS_GUI_LABEL_TEXT},
     {contAddress3, 3, ADDRESS_GUI_ADDR_MENU_TEXT},
     {contCity3, 3, ADDRESS_GUI_LABEL_TEXT},
     {contState3, 3, ADDRESS_GUI_LABEL_TEXT},
     {contZip3, 3, ADDRESS_GUI_LABEL_TEXT},
     {contCountry3, 3, ADDRESS_GUI_LABEL_TEXT},
     {contBirthday, 4, ADDRESS_GUI_BIRTHDAY},
     {contCustom1, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom2, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom3, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom4, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom5, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom6, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom7, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom8, 4, ADDRESS_GUI_LABEL_TEXT},
     {contCustom9, 4, ADDRESS_GUI_LABEL_TEXT},
     {contNote, 4, ADDRESS_GUI_LABEL_TEXT}
};

static address_schema_entry address_schema[19]={
     {contLastname, 0, ADDRESS_GUI_LABEL_TEXT},
     {contFirstname, 0, ADDRESS_GUI_LABEL_TEXT},
     {contTitle, 0, ADDRESS_GUI_LABEL_TEXT},
     {contCompany, 0, ADDRESS_GUI_LABEL_TEXT},
     {contPhone1, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone2, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone3, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone4, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contPhone5, 0, ADDRESS_GUI_DIAL_SHOW_MENU_TEXT},
     {contAddress1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contCity1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contState1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contZip1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contCountry1, 1, ADDRESS_GUI_LABEL_TEXT},
     {contCustom1, 2, ADDRESS_GUI_LABEL_TEXT},
     {contCustom2, 2, ADDRESS_GUI_LABEL_TEXT},
     {contCustom3, 2, ADDRESS_GUI_LABEL_TEXT},
     {contCustom4, 2, ADDRESS_GUI_LABEL_TEXT},
     {contNote, 2, ADDRESS_GUI_LABEL_TEXT}
};

/*
 * This keeps track of whether we are using addresses, or contacts
 * 0 is addresses, 1 is contacts
 */
long address_version=0;

//undo not needed??
char *field_names[]={"Last", "First", "Title", "Company", "Phone1",
     "Phone2", "Phone3", "Phone4", "Phone5", "Address", "City", "State",
     "ZipCode", "Country", "Custom1", "Custom2", "Custom3", "Custom4",
     "Note", "phoneLabel1", "phoneLabel2", "phoneLabel3", "phoneLabel4",
     "phoneLabel5", "showPhone", NULL};
char *field_names_ja[]={"kana(Last)", "Last",  "kana(First)", "First",
     "Title", "kana(Company)","Company", "Phone1",
     "Phone2", "Phone3", "Phone4", "Phone5", "Address", "City", "State",
     "ZipCode", "Country", "Custom1", "Custom2", "Custom3", "Custom4",
     "Note", "phoneLabel1", "phoneLabel2", "phoneLabel3", "phoneLabel4",
     "phoneLabel5", "showPhone", NULL};

#define ADDRESS_NAME_COLUMN  0
#define ADDRESS_NOTE_COLUMN  1
#define ADDRESS_PHONE_COLUMN 2

#define ADDRESS_MAX_CLIST_NAME 30
#define ADDRESS_MAX_COLUMN_LEN 80

/* Many RFCs require that the line termination be CRLF rather than just \n.
 * For conformance to standards this requires adding the two-byte string to
 * the end of strings destined for export */
#define CRLF "\x0D\x0A"

GtkWidget *clist;
#define MAX_NUM_TEXTS contNote+1
GtkWidget *address_text[MAX_NUM_TEXTS];
#ifdef ENABLE_GTK2
static GObject *gtk_txt_buf_address_text[MAX_NUM_TEXTS];
#endif
GtkWidget *text;
#ifdef ENABLE_GTK2
static GObject *gtk_txt_buf_text;
#endif
#ifndef ENABLE_GTK2
GtkWidget *vscrollbar;
#endif
//defines??
//GtkWidget *menu;
GtkWidget *notebook_label[6];
GtkWidget *phone_type_list_menu[NUM_PHONE_ENTRIES];
GtkWidget *phone_type_menu_item[7][8]; /* 7 menus with 8 possible entries */
GtkWidget *address_type_list_menu[3];
GtkWidget *address_type_menu_item[3][3]; /* 3 menus with 3 possible entries */
GtkWidget *IM_type_list_menu[2];
GtkWidget *IM_type_menu_item[2][5]; /* 2 menus with 5 possible entries */
int address_phone_label_selected[NUM_PHONE_ENTRIES];
int address_type_selected[3];
int IM_type_selected[2];

/* We need an extra one for the ALL category */
GtkWidget *address_cat_menu_item1[NUM_ADDRESS_CAT_ITEMS+1];
GtkWidget *address_cat_menu_item2[NUM_ADDRESS_CAT_ITEMS];
static GtkWidget *category_menu1;
static GtkWidget *category_menu2;
GtkWidget *address_quickfind_entry;
static GtkWidget *notebook;
static GtkWidget *pane;
static GtkWidget *radio_button[NUM_PHONE_ENTRIES];
static GtkWidget *dial_button[NUM_PHONE_ENTRIES];

static struct AddressAppInfo address_app_info;
static struct ContactAppInfo contact_app_info;
static struct sorted_cats sort_l[NUM_ADDRESS_CAT_ITEMS];
int address_category=CATEGORY_ALL;
static int clist_row_selected;
extern GtkTooltips *glob_tooltips;

static ContactList *glob_contact_list=NULL;
static ContactList *export_contact_list=NULL;

static GtkWidget *new_record_button;
static GtkWidget *apply_record_button;
static GtkWidget *add_record_button;
static GtkWidget *delete_record_button;
static GtkWidget *undelete_record_button;
static GtkWidget *copy_record_button;
static GtkWidget *cancel_record_button;
static int record_changed;

static GtkWidget *private_checkbox;
//static GtkWidget *picture_box;
static GtkWidget *picture_button;
static GtkWidget *birthday_checkbox;
static GtkWidget *birthday_button;
static GtkWidget *birthday_box;
static GtkWidget *reminder_checkbox;
static GtkWidget *reminder_entry;
static GtkWidget *reminder_box;
struct tm birthday;
static GtkWidget *image=NULL;
struct ContactPicture contact_picture;

GList *changed_list=NULL;

static void connect_changed_signals(int con_or_dis);
static void address_update_clist(GtkWidget *clist, GtkWidget *tooltip_widget,
				 ContactList **cont_list, int category, int main);
int address_clist_redraw();
static int address_find();
static void get_address_attrib(unsigned char *attrib);
static void cb_clist_selection(GtkWidget      *clist,
			       gint           row,
			       gint           column,
			       GdkEventButton *event,
			       gpointer       data);

static void init()
{
   int i, j;
   time_t ltime;
   struct tm *now;

   if (address_version) {
      jp_logf(JP_LOG_DEBUG, "setting schema to contacts\n");
      schema = contact_schema;
      schema_size = NUM_CONTACT_FIELDS;
   } else {
      jp_logf(JP_LOG_DEBUG, "setting schema to addresses\n");
      schema = address_schema;
      schema_size = 19;
   }

   time(&ltime);
   now = localtime(&ltime);
   memcpy(&birthday, now, sizeof(struct tm));

   contact_picture.dirty=0;
   contact_picture.length=0;
   contact_picture.data=NULL;

   changed_list=NULL;
   record_changed=CLEAR_FLAG;
   for (i=0; i<NUM_MENU_ITEM1; i++) {
      for (j=0; j<NUM_MENU_ITEM2; j++) {
	 phone_type_menu_item[i][j] = NULL;
      }
   }
   for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
      address_cat_menu_item2[i] = NULL;
   }
}

static void set_new_button_to(int new_state)
{
   jp_logf(JP_LOG_DEBUG, "set_new_button_to new %d old %d\n", new_state, record_changed);

   if (record_changed==new_state) {
      return;
   }

   switch (new_state) {
    case MODIFY_FLAG:
      gtk_widget_show(cancel_record_button);
      gtk_widget_show(copy_record_button);
      gtk_widget_show(apply_record_button);

      gtk_widget_hide(add_record_button);
      gtk_widget_hide(delete_record_button);
      gtk_widget_hide(new_record_button);
      gtk_widget_hide(undelete_record_button);

      break;
    case NEW_FLAG:
      gtk_widget_show(cancel_record_button);
      gtk_widget_show(add_record_button);

      gtk_widget_hide(apply_record_button);
      gtk_widget_hide(copy_record_button);
      gtk_widget_hide(delete_record_button);
      gtk_widget_hide(new_record_button);
      gtk_widget_hide(undelete_record_button);

      break;
    case CLEAR_FLAG:
      gtk_widget_show(delete_record_button);
      gtk_widget_show(copy_record_button);
      gtk_widget_show(new_record_button);

      gtk_widget_hide(add_record_button);
      gtk_widget_hide(apply_record_button);
      gtk_widget_hide(cancel_record_button);
      gtk_widget_hide(undelete_record_button);

      break;
    case UNDELETE_FLAG:
      gtk_widget_show(undelete_record_button);
      gtk_widget_show(copy_record_button);
      gtk_widget_show(new_record_button);

      gtk_widget_hide(add_record_button);
      gtk_widget_hide(apply_record_button);
      gtk_widget_hide(cancel_record_button);
      gtk_widget_hide(delete_record_button);
      break;

    default:
      return;
   }

   record_changed=new_state;
}

static void
cb_record_changed(GtkWidget *widget,
		  gpointer   data)
{
   jp_logf(JP_LOG_DEBUG, "cb_record_changed\n");
   if (record_changed==CLEAR_FLAG) {
      connect_changed_signals(DISCONNECT_SIGNALS);
      if (GTK_CLIST(clist)->rows > 0) {
	 set_new_button_to(MODIFY_FLAG);
      } else {
	 set_new_button_to(NEW_FLAG);
      }
   }
   else if (record_changed==UNDELETE_FLAG)
   {
      jp_logf(JP_LOG_INFO|JP_LOG_GUI,
	      _("This record is deleted.\n"
	      "Undelete it or copy it to make changes.\n"));
   }
}

static void connect_changed_signals(int con_or_dis)
{
   GtkWidget *w;
   GList *temp_list;
   static int connected=0;

   /* Connect signals */
   if ((con_or_dis==CONNECT_SIGNALS)) {
      if (connected) return;
      connected=1;
      for (temp_list = changed_list; temp_list; temp_list = temp_list->next) {
	 if (!(w=temp_list->data)) {
	    continue;
	 }
	 if (GTK_IS_TEXT_BUFFER(w) ||
	     GTK_IS_ENTRY(w) ||
#ifdef ENABLE_GTK2
	     GTK_IS_TEXT_VIEW(w)
#else
	     GTK_IS_TEXT(w)
#endif
	     ) {
#ifdef ENABLE_GTK2
	    g_signal_connect(w, "changed", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#else
	    gtk_signal_connect(GTK_OBJECT(w), "changed", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#endif
	    continue;
	 }
	 if (GTK_IS_CHECK_MENU_ITEM(w) ||
	     GTK_IS_RADIO_BUTTON(w) ||
	     GTK_IS_CHECK_BUTTON(w)
	     ) {
#ifdef ENABLE_GTK2
	    g_signal_connect(w, "toggled", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#else
	    gtk_signal_connect(GTK_OBJECT(w), "toggled", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#endif
	    continue;
	 }
	 if (GTK_IS_BUTTON(w)) {
#ifdef ENABLE_GTK2
	    g_signal_connect(w, "pressed", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#else
	    gtk_signal_connect(GTK_OBJECT(w), "pressed", GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#endif
	    continue;
	 }
      }
      return;
   }

   /* Disconnect signals */
   if ((con_or_dis==DISCONNECT_SIGNALS)) {
      if (!connected) return;
      connected=0;
      for (temp_list = changed_list; temp_list; temp_list = temp_list->next) {
	 if (!(temp_list->data)) {
	    continue;
	 }
	 w=temp_list->data;
#ifdef ENABLE_GTK2
	 g_signal_handlers_disconnect_by_func(w, GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#else
	 gtk_signal_disconnect_by_func(w, GTK_SIGNAL_FUNC(cb_record_changed), NULL);
#endif
      }
   }
}

//undo convert
int address_print()
{
   long this_many;
   MyAddress *maddr;
   AddressList *address_list;
   AddressList address_list1;

   get_pref(PREF_PRINT_THIS_MANY, &this_many, NULL);

   address_list=NULL;
   if (this_many==1) {
      maddr = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
      if (maddr < (MyAddress *)CLIST_MIN_DATA) {
	 return EXIT_FAILURE;
      }
      memcpy(&(address_list1.maddr), maddr, sizeof(MyAddress));
      address_list1.next=NULL;
      address_list = &address_list1;
   }
   if (this_many==2) {
      get_addresses2(&address_list, SORT_ASCENDING, 2, 2, 2, address_category);
   }
   if (this_many==3) {
      get_addresses2(&address_list, SORT_ASCENDING, 2, 2, 2, CATEGORY_ALL);
   }

   print_addresses(address_list);

   if ((this_many==2) || (this_many==3)) {
      free_AddressList(&address_list);
   }

   return EXIT_SUCCESS;
}

GString *contact_to_gstring(struct Contact *cont)
{
   GString *s;
   int i;
   int address_i, IM_i, phone_i;
   char birthday_str[255];
   const char *pref_date;
   char NL[2];

   s = g_string_sized_new(4096);
   NL[0]='\0'; NL[1]='\0';

   address_i = IM_i = phone_i = 0;
   for (i=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_LABEL_TEXT:
       case ADDRESS_GUI_WEBSITE:
	 if (cont->entry[schema[i].record_field]==NULL) continue;
	 g_string_sprintfa(s, "%s%s:%s",
			    NL, contact_app_info.labels[schema[i].record_field],
			    cont->entry[schema[i].record_field]);
	 NL[0]='\n';
	 break;
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	 if (cont->entry[schema[i].record_field]==NULL) {
	    phone_i++; continue;
	 }
	 g_string_sprintfa(s, "%s%s:%s",
			   NL, contact_app_info.phoneLabels[cont->phoneLabel[phone_i]],
			   cont->entry[schema[i].record_field]);
	 NL[0]='\n';
	 phone_i++;
	 break;
       case ADDRESS_GUI_IM_MENU_TEXT:
	 if (cont->entry[schema[i].record_field]==NULL) {
	    IM_i++; continue;
	 }
	 g_string_sprintfa(s, "%s%s:%s",
			   NL, contact_app_info.IMLabels[cont->IMLabel[IM_i]],
			   cont->entry[schema[i].record_field]);
	 NL[0]='\n';
	 IM_i++;
	 break;
       case ADDRESS_GUI_ADDR_MENU_TEXT:
	 if (cont->entry[schema[i].record_field]==NULL) {
	    address_i++; continue;
	 }
	 g_string_sprintfa(s, "%s%s:%s",
			   NL, contact_app_info.addrLabels[cont->addressLabel[address_i]],
			   cont->entry[schema[i].record_field]);
	 NL[0]='\n';
	 address_i++;
	 break;
       case ADDRESS_GUI_BIRTHDAY:
	 if (cont->birthdayFlag==0) continue;
	 get_pref(PREF_LONGDATE, NULL, &pref_date);
	 strftime(birthday_str, sizeof(birthday_str), pref_date, &cont->birthday);

	 g_string_sprintfa(s, "%s%s:%s",
			   NL, contact_app_info.labels[schema[i].record_field],
			   birthday_str);
	 NL[0]='\n';
	 break;
      }
   }
   return s;
}

//undo use above function
int address_to_text(struct Address *addr, char *text, int len)//old
{
   char *Pstr;
   int left;

   Pstr = text;
   left = len;
   //undo use a gstring?
   g_snprintf(text, len,
	      "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n"
	      "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n"
	      "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n"
	      "%s: %s\n%s: %s\n%s: %s\n%s: %s\n"
	      "%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n"
	      "%s: %d\n",
	      contact_app_info.labels[entryLastname], addr->entry[entryLastname],
	      field_names[1], addr->entry[entryFirstname],
	      field_names[2], addr->entry[entryTitle],
	      field_names[3], addr->entry[entryCompany],
	      field_names[4], addr->entry[entryPhone1],
	      field_names[5], addr->entry[entryPhone2],
	      field_names[6], addr->entry[entryPhone3],
	      field_names[7], addr->entry[entryPhone4],
	      field_names[8], addr->entry[entryPhone5],
	      field_names[9], addr->entry[entryAddress],
	      field_names[10], addr->entry[entryCity],
	      field_names[11], addr->entry[entryState],
	      field_names[12], addr->entry[entryZip],
	      field_names[13], addr->entry[entryCountry],
	      field_names[14], addr->entry[entryCustom1],
	      field_names[15], addr->entry[entryCustom2],
	      field_names[16], addr->entry[entryCustom3],
	      field_names[17], addr->entry[entryCustom4],
	      field_names[18], addr->entry[entryNote],
	      field_names[19], addr->phoneLabel[0],
	      field_names[20], addr->phoneLabel[1],
	      field_names[21], addr->phoneLabel[2],
	      field_names[22], addr->phoneLabel[3],
	      field_names[23], addr->phoneLabel[4],
	      field_names[24], addr->showPhone
	      );
   return EXIT_SUCCESS;
}

/*
 * Start Import Code
 */
int address_import_callback(GtkWidget *parent_window, const char *file_path, int type)
{
   FILE *in;
   char text[65536];
   struct Address new_addr;
   unsigned char attrib;
   int i, ret, index;
   int import_all;
   AddressList *addrlist;
   AddressList *temp_addrlist;
   struct CategoryAppInfo cai;
   char old_cat_name[32];
   int suggested_cat_num;
   int new_cat_num;
   int priv;

   get_address_attrib(&attrib);

   in=fopen(file_path, "r");
   if (!in) {
      jp_logf(JP_LOG_WARN, _("Unable to open file: %s\n"), file_path);
      return EXIT_FAILURE;
   }

   /* CSV */
   if (type==IMPORT_TYPE_CSV) {
      jp_logf(JP_LOG_DEBUG, "Address import CSV [%s]\n", file_path);
      /* The first line is format, so we don't need it */
      fgets(text, sizeof(text), in);
      import_all=FALSE;
      while (1) {
	 /* Read the category field */
	 ret = read_csv_field(in, text, sizeof(text));
	 if (feof(in)) break;
#ifdef JPILOT_DEBUG
	 printf("category is [%s]\n", text);
#endif
	 g_strlcpy(old_cat_name, text, 17);
	 attrib=0;
	 /* Figure out what the best category number is */
	 suggested_cat_num=0;
	 for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
	    if (address_app_info.category.name[i][0]=='\0') continue;
	    if (!strcmp(address_app_info.category.name[i], old_cat_name)) {
	       suggested_cat_num=i;
	       i=1000;
	       break;
	    }
	 }

	 /* Read the private field */
	 ret = read_csv_field(in, text, sizeof(text));
#ifdef JPILOT_DEBUG
	 printf("private is [%s]\n", text);
#endif
	 sscanf(text, "%d", &priv);

//undo rewrite
//	 for (i=0; i<19; i++) {
//	    new_addr.entry[order[i]]=NULL;
//	    ret = read_csv_field(in, text, sizeof(text));
//	    new_addr.entry[order[i]]=strdup(text);
//	 }
	 for (i=0; i<5; i++) {
	    ret = read_csv_field(in, text, sizeof(text));
	    sscanf(text, "%d", &(new_addr.phoneLabel[i]));
	 }
	 ret = read_csv_field(in, text, sizeof(text));
	 sscanf(text, "%d", &(new_addr.showPhone));

	 address_to_text(&new_addr, text, sizeof(text));
	 if (!import_all) {
	    ret=import_record_ask(parent_window, pane,
				  text,
				  &(address_app_info.category),
				  old_cat_name,
				  priv,
				  suggested_cat_num,
				  &new_cat_num);
	 } else {
	    new_cat_num=suggested_cat_num;
	 }
	 if (ret==DIALOG_SAID_IMPORT_QUIT) break;
	 if (ret==DIALOG_SAID_IMPORT_SKIP) continue;
	 if (ret==DIALOG_SAID_IMPORT_ALL) {
	    import_all=TRUE;
	 }
	 attrib = (new_cat_num & 0x0F) |
	   (priv ? dlpRecAttrSecret : 0);
	 if ((ret==DIALOG_SAID_IMPORT_YES) || (import_all)) {
	    pc_address_write(&new_addr, NEW_PC_REC, attrib, NULL);
	 }
      }
   }

   /* Palm Desktop DAT format */
   if (type==IMPORT_TYPE_DAT) {
      jp_logf(JP_LOG_DEBUG, "Address import DAT [%s]\n", file_path);
      if (dat_check_if_dat_file(in)!=DAT_ADDRESS_FILE) {
	 jp_logf(JP_LOG_WARN, _("File doesn't appear to be address.dat format\n"));
	 fclose(in);
	 return EXIT_FAILURE;
      }
      addrlist=NULL;
      dat_get_addresses(in, &addrlist, &cai);
      import_all=FALSE;
      for (temp_addrlist=addrlist; temp_addrlist; temp_addrlist=temp_addrlist->next) {
	 index=temp_addrlist->maddr.unique_id-1;
	 if (index<0) {
	    g_strlcpy(old_cat_name, _("Unfiled"), 17);
	    index=0;
	 } else {
	    g_strlcpy(old_cat_name, cai.name[index], 17);
	 }
	 attrib=0;
	 /* Figure out what category it was in the dat file */
	 index=temp_addrlist->maddr.unique_id-1;
	 suggested_cat_num=0;
	 if (index>-1) {
	    for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
	       if (address_app_info.category.name[i][0]=='\0') continue;
	       if (!strcmp(address_app_info.category.name[i], old_cat_name)) {
		  suggested_cat_num=i;
		  i=1000;
		  break;
	       }
	    }
	 }

	 ret=0;
	 if (!import_all) {
	    address_to_text(&(temp_addrlist->maddr.addr), text, sizeof(text));
	    ret=import_record_ask(parent_window, pane,
				  text,
				  &(address_app_info.category),
				  old_cat_name,
				  (temp_addrlist->maddr.attrib & 0x10),
				  suggested_cat_num,
				  &new_cat_num);
	 } else {
	    new_cat_num=suggested_cat_num;
	 }
	 if (ret==DIALOG_SAID_IMPORT_QUIT) break;
	 if (ret==DIALOG_SAID_IMPORT_SKIP) continue;
	 if (ret==DIALOG_SAID_IMPORT_ALL) {
	    import_all=TRUE;
	 }
	 attrib = (new_cat_num & 0x0F) |
	   ((temp_addrlist->maddr.attrib & 0x10) ? dlpRecAttrSecret : 0);
	 if ((ret==DIALOG_SAID_IMPORT_YES) || (import_all)) {
	    pc_address_write(&(temp_addrlist->maddr.addr), NEW_PC_REC, attrib, NULL);
	 }
      }
      free_AddressList(&addrlist);
   }

   address_refresh();
   fclose(in);
   return EXIT_SUCCESS;
}

int address_import(GtkWidget *window)
{
   char *type_desc[] = {
      "CSV (Comma Separated Values)",
      "DAT/ABA (Palm Archive Formats)",
      NULL
   };
   int type_int[] = {
      IMPORT_TYPE_CSV,
      IMPORT_TYPE_DAT,
      0
   };

   import_gui(window, pane, type_desc, type_int, address_import_callback);
   return EXIT_SUCCESS;
}
/*
 * End Import Code
 */

/*
 * Start Export code
 */

static char *ldifMapType(int label)
{
   switch(label) {
    case 0:
      return "telephoneNumber";
    case 1:
      return "homePhone";
    case 2:
      return "facsimileTelephoneNumber";
    case 3:
      return "xotherTelephoneNumber";
    case 4:
      return "mail";
    case 5:
      return "xmainTelephoneNumber";
    case 6:
      return "pager";
    case 7:
      return "mobile";
    default:
      return "xunknownTelephoneNumber";
   }
}

static char *vCardMapType(int label)
{
   switch(label) {
    case 0:
      return "work";
    case 1:
      return "home";
    case 2:
      return "fax";
    case 3:
      return "x-other";
    case 4:
      return "email";
    case 5:
      return "x-main";
    case 6:
      return "pager";
    case 7:
      return "cell";
    default:
      return "x-unknown";
   }
}

void cb_addr_export_ok(GtkWidget *export_window, GtkWidget *clist,
		       int type, const char *filename)
{
   MyAddress *maddr;
   GList *list, *temp_list;
   FILE *out;
   struct stat statb;
   const char *short_date;
   time_t ltime;
   struct tm *now;
   char str1[256], str2[256];
   char pref_time[40];
   int i, r, n, len;
   char *button_text[]={N_("OK")};
   char *button_overwrite_text[]={N_("No"), N_("Yes")};
   char text[1024];
   char csv_text[65550];
   long char_set, use_jos;
   char username[256];
   char hostname[256];
   const char *svalue;
   long userid;

   /* this stuff is for vcard only. */
   /* todo: create a pre-export switch */
   get_pref(PREF_USER, NULL, &svalue);
   g_strlcpy(text, svalue, sizeof(text));
   str_to_ical_str(username, sizeof(username), text);
   get_pref(PREF_USER_ID, &userid, NULL);
   gethostname(text, sizeof(text));
   text[sizeof(text)-1]='\0';
   str_to_ical_str(hostname, sizeof(hostname), text);

   list=GTK_CLIST(clist)->selection;

   if (!stat(filename, &statb)) {
      if (S_ISDIR(statb.st_mode)) {
	 g_snprintf(text, sizeof(text), _("%s is a directory"), filename);
	 dialog_generic(GTK_WINDOW(export_window),
			_("Error Opening File"),
			DIALOG_ERROR, text, 1, button_text);
	 return;
      }
      g_snprintf(text, sizeof(text), _("Do you want to overwrite file %s?"), filename);
      r = dialog_generic(GTK_WINDOW(export_window),
			 _("Overwrite File?"),
			 DIALOG_QUESTION, text, 2, button_overwrite_text);
      if (r!=DIALOG_SAID_2) {
	 return;
      }
   }

   out = fopen(filename, "w");
   if (!out) {
      g_snprintf(text, sizeof(text), _("Error opening file: %s"), filename);
      dialog_generic(GTK_WINDOW(export_window),
		     _("Error Opening File"),
		     DIALOG_ERROR, text, 1, button_text);
      return;
   }

   for (i=0, temp_list=list; temp_list; temp_list = temp_list->next, i++) {
      maddr = gtk_clist_get_row_data(GTK_CLIST(clist), GPOINTER_TO_INT(temp_list->data));
      if (!maddr) {
	 continue;
	 jp_logf(JP_LOG_WARN, _("Can't export address %d\n"), (long) temp_list->data + 1);
      }
      switch (type) {
       case EXPORT_TYPE_TEXT:
	 get_pref(PREF_SHORTDATE, NULL, &short_date);
	 get_pref_time_no_secs(pref_time);
	 time(&ltime);
	 now = localtime(&ltime);
	 strftime(str1, sizeof(str1), short_date, now);
	 strftime(str2, sizeof(str2), pref_time, now);
	 g_snprintf(text, sizeof(text), "%s %s", str1, str2);

	 /* Todo Should I translate these? */
	 fprintf(out, "Address: exported from %s on %s\n", PN, text);
	 fprintf(out, "Category: %s\n", address_app_info.category.name[maddr->attrib & 0x0F]);
	 fprintf(out, "Private: %s\n",
		 (maddr->attrib & dlpRecAttrSecret) ? "Yes":"No");
//undo rewrite
//	 for (n=0; (field_names[n]) && (n < NUM_ADDRESS_ENTRIES); n++) {
//	    fprintf(out, "%s: ", field_names[n]);
//	    fprintf(out, "%s\n", maddr->addr.entry[order[n]] ?
//		    maddr->addr.entry[order[n]] : "");
//	 }
	 for (n = 0; (field_names[n + NUM_ADDRESS_ENTRIES]) && ( n < NUM_PHONE_ENTRIES); n++) {
	    fprintf(out, "%s: ", field_names[n + NUM_ADDRESS_ENTRIES]);
	    fprintf(out, "%d\n", maddr->addr.phoneLabel[n]);
	 }
	 fprintf(out, "Show Phone: %d\n\n", maddr->addr.showPhone);
	 break;
       case EXPORT_TYPE_CSV:
	 get_pref(PREF_CHAR_SET, &char_set, NULL);
	 get_pref(PREF_USE_JOS, &use_jos, NULL);
	 if (i==0) {
	    fprintf(out, "CSV address: Category, Private, ");
	    if (!use_jos && (char_set == CHAR_SET_JAPANESE || char_set == CHAR_SET_SJIS_UTF)) {
	       for (n=0; (field_names_ja[n])
		    && (n < NUM_ADDRESS_ENTRIES + (NUM_PHONE_ENTRIES * 2) + 1
			+ NUM_ADDRESS_EXT_ENTRIES); n++) {
		  fprintf(out, "%s", field_names_ja[n]);
		  if (field_names_ja[n+1]) {
		     fprintf(out, ", ");
		  }
	       }
	    } else {
	       for (n=0; (field_names[n])
		    && (n < NUM_ADDRESS_ENTRIES + (NUM_PHONE_ENTRIES*2) +1 ); n++) {
		  fprintf(out, "%s", field_names[n]);
		  if (field_names[n+1]) {
		     fprintf(out, ", ");
		  }
	       }
	    }
	    fprintf(out, "\n");
	 }
	 len=0;
	 str_to_csv_str(csv_text,
			address_app_info.category.name[maddr->attrib & 0x0F]);
	 fprintf(out, "\"%s\",", csv_text);
	 fprintf(out, "\"%s\",", (maddr->attrib & dlpRecAttrSecret) ? "1":"0");
	 if (!use_jos && (char_set == CHAR_SET_JAPANESE || char_set == CHAR_SET_SJIS_UTF)) {
	    //char *tmp_p;
	    for (n = 0; n < NUM_ADDRESS_ENTRIES + NUM_ADDRESS_EXT_ENTRIES; n++) {
	       csv_text[0] = '\0';
	       //undo rewrite
#if 0
	       if ((order_ja[n] < NUM_ADDRESS_EXT_ENTRIES)
		   && (tmp_p = strchr(maddr->addr.entry[order_ja[n]],'\1'))) {
		  if (strlen(maddr->addr.entry[order_ja[n]]) > 65535) {
		     jp_logf(JP_LOG_WARN, "%s > 65535\n", _("Field"));
		  } else {
		     *(tmp_p) = '\0';
		     str_to_csv_str(csv_text, maddr->addr.entry[order_ja[n]]);
		     *(tmp_p) = '\1';
		  }
	       } else if (order_ja[n] < NUM_ADDRESS_ENTRIES) {
		  if (strlen(maddr->addr.entry[i]) > 65535) {
		     jp_logf(JP_LOG_WARN, "%s > 65535\n", _("Field"));
		  } else {
		     str_to_csv_str(csv_text, maddr->addr.entry[order_ja[n]]);
		  }
	       } else if (maddr->addr.entry[order_ja[n] - NUM_ADDRESS_ENTRIES]
			  && (tmp_p = strchr(maddr->addr.entry[order_ja[n] - NUM_ADDRESS_ENTRIES], '\1'))) {
		  str_to_csv_str(csv_text, (tmp_p + 1));
	       } else {
		  str_to_csv_str(csv_text, maddr->addr.entry[order_ja[n]]);
	       }
#endif
	       fprintf(out, "\"%s\", ", csv_text);
	    }
	 } else {
	    for (n = 0; n < NUM_ADDRESS_ENTRIES; n++) {
	       csv_text[0]='\0';
//undo rewrite
#if 0
	       if (maddr->addr.entry[order[n]]) {
		  if (strlen(maddr->addr.entry[order[n]])>65535) {
		     jp_logf(JP_LOG_WARN, "%s > 65535\n", _("Field"));
		  } else {
		     str_to_csv_str(csv_text, maddr->addr.entry[order[n]]);
		  }
	       }
#endif
	       fprintf(out, "\"%s\",", csv_text);
	    }
	 }
	 for (n = 0; n < NUM_PHONE_ENTRIES; n++) {
	    fprintf(out, "\"%d\",", maddr->addr.phoneLabel[n]);
	 }
	 fprintf(out, "\"%d\"", maddr->addr.showPhone);
	 fprintf(out, "\n");
	 break;

       case EXPORT_TYPE_VCARD:
	 /* RFC 2426: vCard MIME Directory Profile */
	 fprintf(out, "BEGIN:VCARD%s", CRLF);
	 fprintf(out, "VERSION:3.0%s", CRLF);
	 fprintf(out, "PRODID:%s%s", FPI_STRING, CRLF);
	 if (maddr->attrib & dlpRecAttrSecret) {
	    fprintf(out, "CLASS:PRIVATE%s", CRLF);
	 }
	 fprintf(out, "UID:palm-addressbook-%08x-%08lx-%s@%s%s",
		 maddr->unique_id, userid, username, hostname, CRLF);
	 str_to_vcard_str(csv_text, sizeof(csv_text),
			  address_app_info.category.name[maddr->attrib & 0x0F]);
	 fprintf(out, "CATEGORIES:%s%s", csv_text, CRLF);
	 if (maddr->addr.entry[0] || maddr->addr.entry[1]) {
	    char *last = maddr->addr.entry[0];
	    char *first = maddr->addr.entry[1];
	    fprintf(out, "FN:");
	    if (first) {
	       str_to_vcard_str(csv_text, sizeof(csv_text), first);
	       fprintf(out, "%s", csv_text);
	    }
	    if (first && last) {
	       fprintf(out, " ");
	    }
	    if (last) {
	       str_to_vcard_str(csv_text, sizeof(csv_text), last);
	       fprintf(out, "%s", csv_text);
	    }
	    fprintf(out, CRLF);
	    fprintf(out, "N:");
	    if (last) {
	       str_to_vcard_str(csv_text, sizeof(csv_text), last);
	       fprintf(out, "%s", csv_text);
	    }
	    fprintf(out, ";");
	    /* split up first into first + middle and do first;middle,middle*/
	    if (first) {
	       str_to_vcard_str(csv_text, sizeof(csv_text), first);
	       fprintf(out, "%s", csv_text);
	    }
	    fprintf(out, CRLF);
	 } else if (maddr->addr.entry[2]) {
	    str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[2]);
	    fprintf(out, "FN:%s%sN:%s%s", csv_text, CRLF, csv_text, CRLF);
	 } else {
	    fprintf(out, "FN:-Unknown-%sN:known-;-Un%s", CRLF, CRLF);
	 }
	 if (maddr->addr.entry[13]) {
	    str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[13]);
	    fprintf(out, "TITLE:%s%s", csv_text, CRLF);
	 }
	 if (maddr->addr.entry[2]) {
	    str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[2]);
	    fprintf(out, "ORG:%s%s", csv_text, CRLF);
	 }
	 for (n = 3; n < 8; n++) {
	    if (maddr->addr.entry[n]) {
	       str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[n]);
	       if (maddr->addr.phoneLabel[n - 3] == 4) {
		  fprintf(out, "EMAIL:%s%s", csv_text, CRLF);
	       } else {
		  fprintf(out, "TEL;TYPE=%s", vCardMapType(maddr->addr.phoneLabel[n - 3]));
		  if (maddr->addr.showPhone == n - 3) {
		     fprintf(out, ",pref");
		  }
		  fprintf(out, ":%s%s", csv_text, CRLF);
	       }
	    }
	 }
	 if (maddr->addr.entry[8] || maddr->addr.entry[9] || maddr->addr.entry[10] || maddr->addr.entry[11] || maddr->addr.entry[12]) {
	    /* XXX wrap this line. */
	    fprintf(out, "ADR:;;");
	    for (n = 8; n < 13; n++) {
	       if (maddr->addr.entry[n]) {
		  str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[n]);
		  fprintf(out, "%s", csv_text);
	       }
	       if (n < 12) {
		  fprintf(out, ";");
	       }
	    }
	    fprintf(out, CRLF);
	 }
	 if (maddr->addr.entry[14] || maddr->addr.entry[15] || maddr->addr.entry[16] ||
	     maddr->addr.entry[17] || maddr->addr.entry[18]) {
	    char *labels[]={"Custom1","Custom2","Custom3","Custom4","Note"};
	    int firstnote=1;
	    fprintf(out, "NOTE:");
	    for (n=14;n<=18;n++) {
	       if (maddr->addr.entry[n]) {
		  str_to_vcard_str(csv_text, sizeof(csv_text), maddr->addr.entry[n]);
		  if (firstnote == 0) {
		     fprintf(out, " ");
		  }
		  if (n==18 && firstnote) {
		     fprintf(out, "%s\\n%s", csv_text, CRLF);
		  } else {
		     fprintf(out, "%s:\\n%s %s\\n%s", labels[n-14], CRLF, csv_text, CRLF);
		  }
		  firstnote=0;
	       }
	    }
	 }
	 fprintf(out, "END:VCARD%s", CRLF);
	 break;
       case EXPORT_TYPE_LDIF:
	 /* RFC 2256 - organizationalPerson */
	 /* RFC 2798 - inetOrgPerson */
	 /* RFC 2849 - LDIF file format */
	 if (i == 0) {
	    fprintf(out, "version: 1\n");
	 }
	 {
	    char *cn;
	    char *email = NULL;
	    char *last = maddr->addr.entry[0];
	    char *first = maddr->addr.entry[1];
	    for (n = 3; n < 8; n++) {
	       if (maddr->addr.entry[n] && maddr->addr.phoneLabel[n - 3] == 4) {
		  email = maddr->addr.entry[n];
		  break;
	       }
	    }
	    if (first || last) {
	       cn = csv_text;
	       snprintf(csv_text, sizeof(csv_text), "%s%s%s", first ? first : "",
			first && last ? " " : "", last ? last : "");
	       if (!last) {
		  last = first;
		  first = NULL;
	       }
	    } else if (maddr->addr.entry[2]) {
	       last = maddr->addr.entry[2];
	       cn = last;
	    } else {
	       last = "Unknown";
	       cn = last;
	    }
	    /* maybe add dc=%s for each part of the email address? */
	    /* Mozilla just does mail=%s */
	    ldif_out(out, "dn", "cn=%s%s%s", cn, email ? ",mail=" : "",
		     email ? email : "");
	    fprintf(out, "dnQualifier: %s\n", PN);
	    fprintf(out, "objectClass: top\nobjectClass: person\n");
	    fprintf(out, "objectClass: organizationalPerson\n");
	    fprintf(out, "objectClass: inetOrgPerson\n");
	    ldif_out(out, "cn", "%s", cn);
	    ldif_out(out, "sn", "%s", last);
	    if (first)
	      ldif_out(out, "givenName", "%s", first);
	    if (maddr->addr.entry[2])
	      ldif_out(out, "o", "%s", maddr->addr.entry[2]);
	    for (n = 3; n < 8; n++) {
	       if (maddr->addr.entry[n]) {
		  ldif_out(out, ldifMapType(maddr->addr.phoneLabel[n - 3]), "%s", maddr->addr.entry[n]);
	       }
	    }
	    if (maddr->addr.entry[8])
	      ldif_out(out, "postalAddress", "%s", maddr->addr.entry[8]);
	    if (maddr->addr.entry[9])
	      ldif_out(out, "l", "%s", maddr->addr.entry[9]);
	    if (maddr->addr.entry[10])
	      ldif_out(out, "st", "%s", maddr->addr.entry[10]);
	    if (maddr->addr.entry[11])
	      ldif_out(out, "postalCode", "%s", maddr->addr.entry[11]);
	    if (maddr->addr.entry[12])
	      ldif_out(out, "c", "%s", maddr->addr.entry[12]);
	    if (maddr->addr.entry[13])
	      ldif_out(out, "title", "%s", maddr->addr.entry[13]);
	    if (maddr->addr.entry[14])
	      ldif_out(out, "custom1", "%s", maddr->addr.entry[14]);
	    if (maddr->addr.entry[15])
	      ldif_out(out, "custom2", "%s", maddr->addr.entry[15]);
	    if (maddr->addr.entry[16])
	      ldif_out(out, "custom3", "%s", maddr->addr.entry[16]);
	    if (maddr->addr.entry[17])
	      ldif_out(out, "custom4", "%s", maddr->addr.entry[17]);
	    if (maddr->addr.entry[18])
	      ldif_out(out, "description", "%s", maddr->addr.entry[18]);
/*	    if (maddr->addr.entry[19])
	      ldif_out(out, "seeAlso", "%s", maddr->addr.entry[19]);*/
	    fprintf(out, "\n");
	    break;
	 }
       default:
	 jp_logf(JP_LOG_WARN, _("Unknown export type\n"));
      }
   }

   if (out) {
      fclose(out);
   }
}


static void cb_addr_update_clist(GtkWidget *clist, int category)
{
   address_update_clist(clist, NULL, &export_contact_list, category, FALSE);
}


static void cb_addr_export_done(GtkWidget *widget, const char *filename)
{
   //free_AddressList(&export_contact_list);
   free_ContactList(&export_contact_list);

   set_pref(PREF_ADDRESS_EXPORT_FILENAME, 0, filename, TRUE);
}

int address_export(GtkWidget *window)
{
   int w, h, x, y;
   char *type_text[]={"Text", "CSV", "vCard", "ldif", NULL};
   int type_int[]={EXPORT_TYPE_TEXT, EXPORT_TYPE_CSV, EXPORT_TYPE_VCARD, EXPORT_TYPE_LDIF};

   gdk_window_get_size(window->window, &w, &h);
   gdk_window_get_root_origin(window->window, &x, &y);

#ifdef ENABLE_GTK2
   w = gtk_paned_get_position(GTK_PANED(pane));
#else
   w = GTK_PANED(pane)->handle_xpos;
#endif
   x+=40;

   export_gui(window,
              w, h, x, y, 3, sort_l,
	      PREF_ADDRESS_EXPORT_FILENAME,
	      type_text,
	      type_int,
	      cb_addr_update_clist,
	      cb_addr_export_done,
	      cb_addr_export_ok
	      );

   return EXIT_SUCCESS;
}

/*
 * End Export Code
 */

static int find_sorted_cat(int cat)
{
   int i;
   for (i=0; i< NUM_ADDRESS_CAT_ITEMS; i++) {
      if (sort_l[i].cat_num==cat) {
	 return i;
      }
   }
   return EXIT_FAILURE;
}


void cb_delete_address(GtkWidget *widget, gpointer data)
{
   MyAddress maddr;
   MyContact *mcont;
   int flag;
   int show_priv;
   long char_set; /* JPA */
   int i; /* JPA */

   mcont = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
   
   if (mcont < (MyContact *)CLIST_MIN_DATA) {
      return;
   }

   copy_contact_to_address(&(mcont->cont), &(maddr.addr));
   maddr.rt = mcont->rt;
   maddr.unique_id = mcont->unique_id;
   maddr.attrib = mcont->attrib;

   /* JPA convert to Palm character set */
   get_pref(PREF_CHAR_SET, &char_set, NULL);
   if (char_set != CHAR_SET_LATIN1) {
      for (i=0; i<19; i++) {
	 if (maddr.addr.entry[i]) {
	    charset_j2p(maddr.addr.entry[i],
			strlen(maddr.addr.entry[i])+1, char_set);
	 }
      }
   }

   /* Do masking like Palm OS 3.5 */
   show_priv = show_privates(GET_PRIVATES);
   if ((show_priv != SHOW_PRIVATES) &&
       (maddr.attrib & dlpRecAttrSecret)) {
      free_Address(&(maddr.addr));
      return;
   }
   /* End Masking */
   flag = GPOINTER_TO_INT(data);
   if ((flag==MODIFY_FLAG) || (flag==DELETE_FLAG)) {
      delete_pc_record(ADDRESS, &maddr, flag);
      if (flag==DELETE_FLAG) {
	 /* when we redraw we want to go to the line above the deleted one */
	 if (clist_row_selected>0) {
	    clist_row_selected--;
	 }
      }
   }

   free_Address(&(maddr.addr));

   if (flag == DELETE_FLAG) {
      address_clist_redraw();
   }
}

void cb_delete_contact(GtkWidget *widget, gpointer data)
{
   MyContact *mcont;
   int flag;
   int show_priv;
   long char_set;
   int i;

   mcont = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
   if (mcont < (MyContact *)CLIST_MIN_DATA) {
      return;
   }
   /* JPA convert to Palm character set */
   get_pref(PREF_CHAR_SET, &char_set, NULL);
   if (char_set != CHAR_SET_LATIN1) {
      for (i=0; i<NUM_CONTACT_FIELDS; i++) {
	 if (mcont->cont.entry[i]) {
	    charset_j2p(mcont->cont.entry[i],
			strlen(mcont->cont.entry[i])+1, char_set);
	 }
      }
   }

   /* Do masking like Palm OS 3.5 */
   show_priv = show_privates(GET_PRIVATES);
   if ((show_priv != SHOW_PRIVATES) &&
       (mcont->attrib & dlpRecAttrSecret)) {
      return;
   }
   /* End Masking */
   flag = GPOINTER_TO_INT(data);
   if ((flag==MODIFY_FLAG) || (flag==DELETE_FLAG)) {
      delete_pc_record(CONTACTS, mcont, flag);
      if (flag==DELETE_FLAG) {
	 /* when we redraw we want to go to the line above the deleted one */
	 if (clist_row_selected>0) {
	    clist_row_selected--;
	 }
      }
   }

   if (flag == DELETE_FLAG) {
      address_clist_redraw();
   }
}

void cb_delete_address_or_contact(GtkWidget *widget, gpointer data)
{
   if (address_version==0) {
      cb_delete_address(widget, data);
   } else {
      cb_delete_contact(widget, data);
   }
}


void cb_undelete_address(GtkWidget *widget,
		         gpointer   data)
{
   MyAddress *maddr;
   int flag;
   int show_priv;

   maddr = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
   if (maddr < (MyAddress *)CLIST_MIN_DATA) {
      return;
   }

   /* Do masking like Palm OS 3.5 */
   show_priv = show_privates(GET_PRIVATES);
   if ((show_priv != SHOW_PRIVATES) &&
       (maddr->attrib & dlpRecAttrSecret)) {
      return;
   }
   /* End Masking */

   jp_logf(JP_LOG_DEBUG, "maddr->unique_id = %d\n",maddr->unique_id);
   jp_logf(JP_LOG_DEBUG, "maddr->rt = %d\n",maddr->rt);

   flag = GPOINTER_TO_INT(data);
   if (flag==UNDELETE_FLAG) {
      if (maddr->rt == DELETED_PALM_REC ||
	 (maddr->rt == DELETED_PC_REC)) {
	 undelete_pc_record(ADDRESS, maddr, flag);
      }
      /* Possible later addition of undelete for modified records
      else if (maddr->rt == MODIFIED_PALM_REC) {
         cb_add_new_record(widget, GINT_TO_POINTER(COPY_FLAG));
      }
      */
   }

   address_clist_redraw();
}

static void cb_cancel(GtkWidget *widget, gpointer data)
{
   set_new_button_to(CLEAR_FLAG);
   address_refresh();
}

void cb_resort(GtkWidget *widget,
	       gpointer   data)
{
   MyAddress *maddr;

   sort_by_company = !(sort_by_company & 1);

   /* Return to this record after resorting */
   maddr = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
   if (maddr < (MyAddress *)CLIST_MIN_DATA) {
      glob_find_id = 0;
   }
   else {
      glob_find_id = maddr->unique_id;
   }

   address_clist_redraw();
   address_find();

   /* Update labels after redrawing clist to work around GTK bug */
   if (sort_by_company) {
      gtk_clist_set_column_title(GTK_CLIST(clist), ADDRESS_NAME_COLUMN, _("Company/Name"));
   } else {
      gtk_clist_set_column_title(GTK_CLIST(clist), ADDRESS_NAME_COLUMN, _("Name/Company"));
   }
}

void cb_phone_menu(GtkWidget *item, unsigned int value)
{
   if (!item)
     return;
   if ((GTK_CHECK_MENU_ITEM(item))->active) {
      jp_logf(JP_LOG_DEBUG, "phone_menu = %d\n", (value & 0xFF00) >> 8);
      jp_logf(JP_LOG_DEBUG, "selection = %d\n", value & 0xFF);
      address_phone_label_selected[(value & 0xFF00) >> 8] = value & 0xFF;
   }
}

void cb_IM_type_menu(GtkWidget *item, unsigned int value)
{
   if (!item)
     return;
   if ((GTK_CHECK_MENU_ITEM(item))->active) {
      jp_logf(JP_LOG_DEBUG, "IM_type_menu = %d\n", (value & 0xFF00) >> 8);
      jp_logf(JP_LOG_DEBUG, "selection = %d\n", value & 0xFF);
      IM_type_selected[(value & 0xFF00) >> 8] = value & 0xFF;
   }
}

void cb_address_type_menu(GtkWidget *item, unsigned int value)
{
   int menu, selection;

   if (!item)
     return;
   if ((GTK_CHECK_MENU_ITEM(item))->active) {
      menu = (value & 0xFF00) >> 8;
      selection = value & 0xFF;
      jp_logf(JP_LOG_DEBUG, "addr_type_menu = %d\n", menu);
      jp_logf(JP_LOG_DEBUG, "selection = %d\n", selection);
      address_type_selected[menu] = selection;

      if (GTK_IS_LABEL(notebook_label[menu+1])) {
	 gtk_label_set_text(GTK_LABEL(notebook_label[menu+1]), contact_app_info.addrLabels[selection]);
      }
   }
}

void cb_notebook_changed(GtkWidget *widget,
			 GtkWidget *widget2,
			 int        page,
			 gpointer   data)
{
   int prev_page;

   /* GTK calls this function while it is destroying the notebook
    * I use this function to tell if it is being destroyed */
   prev_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget));
   if (prev_page<0) {
      return;
   }
   jp_logf(JP_LOG_DEBUG, "cb_notebook_changed(), prev_page=%d, page=%d\n", prev_page, page);
   set_pref(PREF_ADDRESS_NOTEBOOK_PAGE, page, NULL, TRUE);
}

static void get_address_attrib(unsigned char *attrib)
{
   int i;
   /*Get the category that is set from the menu */
   *attrib = 0;
   for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
      if (GTK_IS_WIDGET(address_cat_menu_item2[i])) {
	 if (GTK_CHECK_MENU_ITEM(address_cat_menu_item2[i])->active) {
	    *attrib = sort_l[i].cat_num;
	    break;
	 }
      }
   }
   /* Get private flag */
   if (GTK_TOGGLE_BUTTON(private_checkbox)->active) {
      *attrib |= dlpRecAttrSecret;
   }
}

static void cb_add_new_record(GtkWidget *widget, gpointer data)
{
   int i;
   struct Contact cont;
   MyContact *mcont;
   struct Address addr;
   unsigned char attrib;
   int address_i, IM_i, phone_i;
   int flag, type;
   unsigned int unique_id;
   int show_priv;
   //long use_jos, char_set;
#ifdef ENABLE_GTK2
   GtkTextIter start_iter;
   GtkTextIter end_iter;
#endif

   memset(&cont, 0, sizeof(cont));
   flag=GPOINTER_TO_INT(data);
   unique_id=0;
   mcont=NULL;

   /* Do masking like Palm OS 3.5 */
   if ((flag==COPY_FLAG) || (flag==MODIFY_FLAG)) {
      show_priv = show_privates(GET_PRIVATES);
      mcont = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
      if (mcont < (MyContact *)CLIST_MIN_DATA) {
	 return;
      }
      if ((show_priv != SHOW_PRIVATES) &&
	  (mcont->attrib & dlpRecAttrSecret)) {
	 return;
      }
   }
   /* End Masking */
   if ((flag==NEW_FLAG) || (flag==COPY_FLAG) || (flag==MODIFY_FLAG)) {
      /*These rec_types are both the same for now */
      if (flag==MODIFY_FLAG) {
	 mcont = gtk_clist_get_row_data(GTK_CLIST(clist), clist_row_selected);
	 unique_id=mcont->unique_id;
	 if (mcont < (MyContact *)CLIST_MIN_DATA) {
	    return;
	 }
	 if ((mcont->rt==DELETED_PALM_REC) ||
	     (mcont->rt==DELETED_PC_REC)   ||
	     (mcont->rt==MODIFIED_PALM_REC)) {
	    jp_logf(JP_LOG_INFO, _("You can't modify a record that is deleted\n"));
	    return;
	 }
      }

      cont.showPhone=0;

      address_i = IM_i = phone_i = 0;
      for (i=0; i<schema_size; i++) {
	 switch (schema[i].type) {
	  case ADDRESS_GUI_LABEL_TEXT:
	    break;
	  case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	    if (GTK_TOGGLE_BUTTON(radio_button[phone_i])->active) {
	       cont.showPhone=phone_i;
	    }
	    cont.phoneLabel[phone_i]=address_phone_label_selected[phone_i];
	    phone_i++;
	    break;
	  case ADDRESS_GUI_IM_MENU_TEXT:
	    cont.IMLabel[IM_i]=IM_type_selected[IM_i];
	    IM_i++;
	    break;
	  case ADDRESS_GUI_ADDR_MENU_TEXT:
	    cont.addressLabel[address_i]=address_type_selected[address_i];
	    address_i++;
	    break;
	  case ADDRESS_GUI_WEBSITE:
	    break;
	  case ADDRESS_GUI_BIRTHDAY:
	    if (GTK_TOGGLE_BUTTON(birthday_checkbox)->active) {
	       cont.birthdayFlag = 1;
	       memcpy(&cont.birthday, &birthday, sizeof(struct tm));
	    }
	    if (GTK_TOGGLE_BUTTON(reminder_checkbox)->active) {
	       cont.reminder = 1;
	       cont.advance=atoi(gtk_entry_get_text(GTK_ENTRY(reminder_entry)));
	       cont.advanceUnits = 1; /* Days */
	    }
	    break;
	 }
      }
      
      for (i=0; i<schema_size; i++) {
	 /* Get the entry texts */
	 switch (schema[i].type) {
	  case ADDRESS_GUI_LABEL_TEXT:
	  case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	  case ADDRESS_GUI_IM_MENU_TEXT:
	  case ADDRESS_GUI_ADDR_MENU_TEXT:
	  case ADDRESS_GUI_WEBSITE:
#ifdef ENABLE_GTK2
	    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(gtk_txt_buf_address_text[schema[i].record_field]),&start_iter,&end_iter);
	    cont.entry[schema[i].record_field] =
	      gtk_text_buffer_get_text(GTK_TEXT_BUFFER(gtk_txt_buf_address_text[schema[i].record_field]),&start_iter,&end_iter,TRUE);
#else
	    cont.entry[schema[i].record_field] =
	      gtk_editable_get_chars(GTK_EDITABLE(address_text[schema[i].record_field]), 0, -1);
#endif
	    break;
	  case ADDRESS_GUI_BIRTHDAY:
	    if (contact_picture.data) {
	       Contact_add_picture(&cont, &contact_picture);
	    }
	    break;
	 }
      }

      /*Get the attributes */
      get_address_attrib(&attrib);

      set_new_button_to(CLEAR_FLAG);

      if (flag==MODIFY_FLAG) {
	 cb_delete_address_or_contact(NULL, data);
	 if ((mcont->rt==PALM_REC) || (mcont->rt==REPLACEMENT_PALM_REC)) {
	    type = REPLACEMENT_PALM_REC;
	 } else {
	    unique_id = 0;
	    type = NEW_PC_REC;
	 }
      } else {
	 unique_id=0;
	 type = NEW_PC_REC;
      }

      if (address_version==0) {
	 copy_contact_to_address(&cont, &addr);
	 pc_address_write(&addr, type, attrib, &unique_id);
	 free_Address(&addr);
      } else {
	 pc_contact_write(&cont, type, attrib, &unique_id);
	 free_Contact(&cont);
      }

      address_clist_redraw();
      glob_find_id = unique_id;
      address_find();
   }
}

void addr_clear_details()
{
   int i;
   int new_cat;
   int sorted_position;
   //long use_jos, char_set;
   int address_i, IM_i, phone_i;

   /* Need to disconnect these signals first */
   connect_changed_signals(DISCONNECT_SIGNALS);

   /* Clear the quickview */
#ifdef ENABLE_GTK2
   gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_txt_buf_text), "", -1);
#else
   gtk_text_set_point(GTK_TEXT(text), 0);
   gtk_text_forward_delete(GTK_TEXT(text),
			   gtk_text_get_length(GTK_TEXT(text)));
#endif

   //START NEW CODE

   /* Clear all of the text fields */
   for (i=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_LABEL_TEXT:
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
       case ADDRESS_GUI_ADDR_MENU_TEXT:
       case ADDRESS_GUI_IM_MENU_TEXT:
       case ADDRESS_GUI_WEBSITE:
#ifdef ENABLE_GTK2
	 gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_txt_buf_address_text[schema[i].record_field]), "", -1);
#else
	 gtk_text_set_point(GTK_TEXT(address_text[schema[i].record_field]), 0);
	 gtk_text_forward_delete(GTK_TEXT(address_text[schema[i].record_field]),
				 gtk_text_get_length(GTK_TEXT(address_text[schema[i].record_field])));
#endif
      }
   }
   
   address_i=IM_i=phone_i=0;
   for (i=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	 if (phone_i==0) {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[0]), TRUE);
	 }
	 gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					(phone_type_menu_item[phone_i][phone_i]), TRUE);
	 gtk_option_menu_set_history(GTK_OPTION_MENU(phone_type_list_menu[phone_i]), phone_i);
	 phone_i++;
	 break;
       case ADDRESS_GUI_IM_MENU_TEXT:
	 gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					(IM_type_menu_item[IM_i][IM_i]), TRUE);
	 gtk_option_menu_set_history(GTK_OPTION_MENU(IM_type_list_menu[IM_i]), IM_i);
	 IM_i++;
	 break;
       case ADDRESS_GUI_ADDR_MENU_TEXT:
	 gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					(address_type_menu_item[address_i][address_i]), TRUE);
	 gtk_option_menu_set_history(GTK_OPTION_MENU(address_type_list_menu[address_i]), address_i);
	 address_i++;
	 break;
       case ADDRESS_GUI_WEBSITE:
	 break;
       case ADDRESS_GUI_BIRTHDAY:
	 gtk_entry_set_text(GTK_ENTRY(reminder_entry), "");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(birthday_checkbox), 0);
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reminder_checkbox), 0);
	 break;
      }
   }

   if (image) {
      gtk_widget_destroy(image);
      image = NULL;
   }

   if (address_category==CATEGORY_ALL) {
      new_cat = 0;
   } else {
      new_cat = address_category;
   }
   sorted_position = find_sorted_cat(new_cat);
   if (sorted_position<0) {
      jp_logf(JP_LOG_WARN, _("Category is not legal\n"));
   } else {
      gtk_check_menu_item_set_active
	(GTK_CHECK_MENU_ITEM(address_cat_menu_item2[sorted_position]), TRUE);
      gtk_option_menu_set_history(GTK_OPTION_MENU(category_menu2), sorted_position);
   }

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private_checkbox), FALSE);
   set_new_button_to(CLEAR_FLAG);

   connect_changed_signals(CONNECT_SIGNALS);
}

void cb_address_clear(GtkWidget *widget,
		      gpointer   data)
{
   addr_clear_details();
   gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 0);
   connect_changed_signals(DISCONNECT_SIGNALS);
   set_new_button_to(NEW_FLAG);
   gtk_widget_grab_focus(GTK_WIDGET(address_text[0]));
}

/* Attempt to make the best possible string out of whatever garbage we find
 * Remove illegal characters, stop at carriage return and at least 1 digit
 */
void parse_phone_str(char *dest, char *src, int max_len)
{
   int i1, i2;

   for (i1=0, i2=0; (i1<max_len) && src[i1]; i1++) {
      if (isdigit(src[i1]) || (src[i1]==',')
	  || (src[i1]=='A') || (src[i1]=='B') || (src[i1]=='C')
	  || (src[i1]=='D') || (src[i1]=='*') || (src[i1]=='#')
	  ) {
	 dest[i2]=src[i1];
	 i2++;
      } else if (((src[i1] =='\n') || (src[i1] =='\r') ||
		  (src[i1] =='x')) && i2) {
	 break;
      }
   }
   dest[i2]='\0';
}

void email_contact(GtkWidget *widget, gchar *str)
{
   char command[1024];
   const char *pref_command;

   get_pref(PREF_MAIL_COMMAND, NULL, &pref_command);
   if (!pref_command) {
      jp_logf(JP_LOG_DEBUG, _("email command empty\n"));
      return;
   }

   /* Make a system call command string */
   g_snprintf(command, sizeof(command), pref_command, str);
   command[1023]='\0';

   jp_logf(JP_LOG_STDOUT|JP_LOG_FILE, _("executing command = [%s]\n"), command);
   system(command);
}

void dial_contact(GtkWidget *widget, gchar *str)
{
   char *Px;
   char number[100];
   char ext[100];

   number[0]=ext[0]='\0';

   parse_phone_str(number, str, sizeof(number));

   Px = strstr(str, "x");
   if (Px) {
      parse_phone_str(ext, Px, sizeof(ext));
   }

   dialog_dial(GTK_WINDOW(gtk_widget_get_toplevel(widget)), number, ext);
}

void cb_dial_or_mail(GtkWidget *widget, gpointer data)
{
   GtkWidget *text;
   gchar *str;
   int is_mail;
#ifdef ENABLE_GTK2
   GtkTextIter    start_iter;
   GtkTextIter    end_iter;
   GtkTextBuffer *text_buffer;
#endif
   text=data;

#ifdef ENABLE_GTK2
   text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
   gtk_text_buffer_get_bounds(text_buffer,&start_iter,&end_iter);
   str = gtk_text_buffer_get_text(text_buffer,&start_iter,&end_iter,TRUE);
#else
   str=gtk_editable_get_chars(GTK_EDITABLE(text), 0, -1);
#endif

   if (!str) return;
   printf("[%s]\n", str);

   is_mail = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "mail"));
   if (is_mail) {
      email_contact(widget, str);
   } else {
      dial_contact(widget, str);
   }

   g_free(str);
}

void cb_address_quickfind(GtkWidget *widget,
			  gpointer   data)
{
   const char *entry_text;
   char *clist_text;
   int i, r;

   jp_logf(JP_LOG_DEBUG, "cb_address_quickfind\n");

   entry_text = gtk_entry_get_text(GTK_ENTRY(widget));
   if (!strlen(entry_text)) {
      return;
   }

   for (i = 0; i<GTK_CLIST(clist)->rows; i++) {
      r = gtk_clist_get_text(GTK_CLIST(clist), i, ADDRESS_NAME_COLUMN, &clist_text);
      if (!r) {
         break;
      }
      if (!strncasecmp(clist_text, entry_text, strlen(entry_text))) {
         clist_select_row(GTK_CLIST(clist), i, ADDRESS_NAME_COLUMN);
         gtk_clist_moveto(GTK_CLIST(clist), i, 0, 0.5, 0.0);
         break;
      }
   }
}

static void cb_category(GtkWidget *item, int selection)
{
   int b;

   if (!item) return;
   if ((GTK_CHECK_MENU_ITEM(item))->active) {
      b=dialog_save_changed_record(pane, record_changed);
      if (b==DIALOG_SAID_2) {
	 cb_add_new_record(NULL, GINT_TO_POINTER(record_changed));
      }

      address_category = selection;
      clist_row_selected = 0;
      jp_logf(JP_LOG_DEBUG, "address_category = %d\n",address_category);
      address_update_clist(clist, category_menu1, &glob_contact_list,
			   address_category, TRUE);
   }
}


/* Do masking like Palm OS 3.5 */
static void clear_mycontact(MyContact *mcont)
{
   int i;

   mcont->unique_id=0;
   mcont->attrib=mcont->attrib & 0xF8;
   for (i=0; i<8; i++) {//undo need a define for this
      mcont->cont.phoneLabel[i]=0;
   }
   //undo need to clear addrLabels, and IMLabels
   mcont->cont.showPhone=0;
   for (i=0; i<NUM_CONTACT_ENTRIES; i++) {
      if (mcont->cont.entry) {
	 free(mcont->cont.entry[i]);
	 mcont->cont.entry[i]=NULL;
      }
   }
   return;
}
/* End Masking */

static void cb_edit_cats(GtkWidget *widget, gpointer data)
{
   struct AddressAppInfo ai;
   char full_name[FILENAME_MAX];
   char buffer[65536];
   int num;
#ifdef PILOT_LINK_0_12
   size_t size;
#else
   int size;
#endif
   void *buf;
   struct pi_file *pf;

   jp_logf(JP_LOG_DEBUG, "cb_edit_cats\n");

   get_home_file_name("AddressDB.pdb", full_name, sizeof(full_name));

   buf=NULL;
   memset(&ai, 0, sizeof(ai));

   pf = pi_file_open(full_name);
   pi_file_get_app_info(pf, &buf, &size);

   num = unpack_AddressAppInfo(&ai, buf, size);
   if (num <= 0) {
      jp_logf(JP_LOG_WARN, _("Error reading file: %s\n"), "AddressDB.pdb");
      return;
   }

   pi_file_close(pf);

   edit_cats(widget, "AddressDB", &(ai.category));

   size = pack_AddressAppInfo(&ai, (unsigned char*)buffer, sizeof(buffer));

   pdb_file_write_app_block("AddressDB", buffer, size);

   cb_app_button(NULL, GINT_TO_POINTER(REDRAW));
}

GtkWidget *image_from_data(void *buf, size_t size)
{
    GdkPixbufLoader *loader;
    GError *error;
    GdkPixbuf *pb;
    GtkWidget *image;

    error=NULL;
    loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_write(loader, buf, size, &error);
    pb = gdk_pixbuf_loader_get_pixbuf(loader);
    image = g_object_ref(gtk_image_new_from_pixbuf(pb));
    if (loader) {
	gdk_pixbuf_loader_close(loader, &error);
	g_object_unref(loader);
    }

    return image;
}

static void set_button_label_to_date(GtkWidget *button, struct tm *date)
{
   char birthday_str[255];
   const char *pref_date;

   birthday_str[0]='\0';
   get_pref(PREF_SHORTDATE, NULL, &pref_date);
   strftime(birthday_str, sizeof(birthday_str), pref_date, date);
#ifdef ENABLE_GTK2
   gtk_button_set_label(GTK_BUTTON(button), birthday_str);
#else
   gtk_object_set(GTK_OBJECT(button), "label", birthday_str, NULL);
#endif
}

static void cb_button_birthday(GtkWidget *widget, gpointer data)
{
   long fdow;
   int r;

   get_pref(PREF_FDOW, &fdow, NULL);
   r = cal_dialog(GTK_WINDOW(gtk_widget_get_toplevel(widget)), _("Birthday"), fdow,
		  &(birthday.tm_mon),
		  &(birthday.tm_mday),
		  &(birthday.tm_year));
   if (r==CAL_DONE) {
      set_button_label_to_date(birthday_button, &birthday);
   }
}

static void cb_check_button_birthday(GtkWidget *widget, gpointer data)
{
   time_t ltime;
   struct tm *now;

   if (GTK_TOGGLE_BUTTON(widget)->active) {
      gtk_widget_show(birthday_box);
      set_button_label_to_date(birthday_button, &birthday);
   } else {
      gtk_widget_hide(birthday_box);
      gtk_widget_hide(reminder_box);
      time(&ltime);
      now = localtime(&ltime);
      memcpy(&birthday, now, sizeof(struct tm));
   }
}

static void cb_check_button_reminder(GtkWidget *widget, gpointer data)
{
   if (GTK_TOGGLE_BUTTON(widget)->active) {
      gtk_widget_show(reminder_box);
   } else {
      gtk_widget_hide(reminder_box);
   }
}

/*
 * Photo Code
 */

int change_photo(char *filename)
{
   FILE *in;
   char command[FILENAME_MAX + 256];
   char buf[0xFFFF];
   int total_read, count, r;

/*   get_home_file_name("photoXXXXXX", full_out, FILENAME_MAX - 5);
   full_out[FILENAME_MAX - 5]='\0';

   out = mkstemp(full_out);
   printf("full_out %s\n", full_out);
   if (out<0) {
      //undo report error
      return -1;
   }
   printf("close = %d\n", close(out));
*/
   sprintf(command, "convert -resize %dx%d %s jpg:-", PHOTO_X, PHOTO_Y, filename);
   printf("calling %s\n", command);
   in = popen(command, "r");
   //#include <errno.h>
   //   printf("in=%d errno=%d ECHILD=%d UNDO\n", in, errno, ECHILD);
   //printf("feof = %d", feof(in));
   //fread(buf, 1, 1, in);
   //printf("feof = %d", feof(in));

   if (!in) {
      return -1;
   }

   total_read = count = 0;
   while (!feof(in)) {
      count = fread(buf + total_read, 1, 0xFFFF - total_read, in);
      //printf("count = %d\n", count);
      total_read+=count;
      //fixme possible buffer overflow
      if ((count==0) || (total_read>=0xFFFF)) break;
   }
   r = pclose(in);

   if (r) {
      dialog_generic_ok(gtk_widget_get_toplevel(notebook),
			_("External program not found, or other error"), DIALOG_ERROR,
			_("J-Pilot can not find an external program \"convert\"\nor an error occurred while executing convert.\nYou may need to install package ImageMagick"));
      jp_logf(JP_LOG_WARN, _("Command executed was \"%s\"\n"), command);
      jp_logf(JP_LOG_WARN, _("return code was %d\n"), r);
      return -1;
   }

   if (image) {
      gtk_widget_destroy(image);
      image=NULL;
   }
   if (contact_picture.data) {
      free(contact_picture.data);
      contact_picture.dirty=0;
      contact_picture.length=0;
      contact_picture.data=NULL;
   }
   contact_picture.data=malloc(total_read);
   memcpy(contact_picture.data, buf, total_read);
   contact_picture.length = total_read;
   contact_picture.dirty = 0;
   image = image_from_data(contact_picture.data, contact_picture.length);
   gtk_container_add(GTK_CONTAINER(picture_button), image);
   gtk_widget_show(image);   

   return 0;
}

//undo make a common filesel function
static void
cb_photo_browse_cancel(GtkWidget *widget, gpointer data)
{
   gtk_widget_destroy(data);
}

static void
cb_photo_browse_ok(GtkWidget *widget, gpointer data)
{
   const char *sel;
   char **Pselection;

   sel = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   set_pref(PREF_CONTACTS_PHOTO_FILENAME, 0, sel, TRUE);
   
   Pselection = gtk_object_get_data(GTK_OBJECT(GTK_FILE_SELECTION(data)), "selection");
   if (Pselection) {
      jp_logf(JP_LOG_DEBUG, "setting selection to %s\n", sel);
      *Pselection = strdup(sel);
   }

   gtk_widget_destroy(data);
}

static gboolean cb_photo_browse_destroy(GtkWidget *widget)
{
   gtk_main_quit();
   return FALSE;
}

int browse_photo(GtkWidget *main_window)
{
   GtkWidget *filesel;
   const char *svalue;
   char dir[MAX_PREF_VALUE+2];
   int i;
   char *selection;

   get_pref(PREF_CONTACTS_PHOTO_FILENAME, NULL, &svalue);
   g_strlcpy(dir, svalue, sizeof(dir));
   i=strlen(dir)-1;
   if (i<0) i=0;
   if (dir[i]!='/') {
      for (i=strlen(dir); i>=0; i--) {
	 if (dir[i]=='/') {
	    dir[i+1]='\0';
	    break;
	 }
      }
   }

   chdir(dir);

   filesel = gtk_file_selection_new(_("Add Photo"));

   gtk_window_set_modal(GTK_WINDOW(filesel), TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(filesel), GTK_WINDOW(main_window));

   gtk_signal_connect(GTK_OBJECT(filesel), "destroy",
		      GTK_SIGNAL_FUNC(cb_photo_browse_destroy), filesel);

   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(cb_photo_browse_ok), filesel);
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC(cb_photo_browse_cancel), filesel);

   gtk_widget_show(filesel);

   gtk_object_set_data(GTK_OBJECT(filesel), "selection", &selection);

   selection = NULL;

   gtk_main();

   if (selection) {
      jp_logf(JP_LOG_DEBUG, "browse_photo(): selection = %s\n", selection);
      change_photo(selection);
   }

   return 0;
}

void cb_photo_menu_select(GtkWidget       *item,
			  GtkPositionType  selected)
{
//undo   printf("selected %d\n", selected);
   if (selected == 1) {
      browse_photo(gtk_widget_get_toplevel(clist));
      return;
   }
   if (selected==2) {
      if (image) {
	 gtk_widget_destroy(image);
	 image=NULL;
      }
      if (contact_picture.data) {
	 free(contact_picture.data);
	 contact_picture.dirty=0;
	 contact_picture.length=0;
	 contact_picture.data=NULL;
      }
      cb_record_changed(NULL, NULL);
   }
}

static gint cb_photo_menu_popup(GtkWidget *widget, GdkEvent *event)
{
   GtkMenu *menu;
   GdkEventButton *event_button;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_MENU(widget), FALSE);
   g_return_val_if_fail(event != NULL, FALSE);

   menu = GTK_MENU (widget);

   if (event->type == GDK_BUTTON_PRESS) {
      event_button = (GdkEventButton *) event;
      if (event_button->button == 1) {
	 gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 
			event_button->button, event_button->time);
	 return TRUE;
      }
   }

   return FALSE;
}
/*
 * End Photo code
 */

static void cb_clist_selection(GtkWidget      *clist,
			       gint           row,
			       gint           column,
			       GdkEventButton *event,
			       gpointer       data)
{
   /* The rename-able phone entries are indexes 3,4,5,6,7 */
   struct Contact *cont;
   MyContact *mcont;
   int cat, count, sorted_position;
   unsigned int unique_id = 0;
   int i;
   int b;
   //char *tmp_p;
   char *clist_text;
   const char *entry_text;
   //long use_jos, char_set;
   int address_i, IM_i, phone_i;
   char birthday_str[255];
   GString *s;

   if ((record_changed==MODIFY_FLAG) || (record_changed==NEW_FLAG)) {
      mcont = gtk_clist_get_row_data(GTK_CLIST(clist), row);
      if (mcont!=NULL) {
	 unique_id = mcont->unique_id;
      }

      b=dialog_save_changed_record(pane, record_changed);
      if (b==DIALOG_SAID_2) {
	 cb_add_new_record(NULL, GINT_TO_POINTER(record_changed));
      }
      set_new_button_to(CLEAR_FLAG);

      if (unique_id)
      {
	 glob_find_id = unique_id;
         address_find();
      } else {
	 clist_select_row(GTK_CLIST(clist), row, column);
      }
      return;
   }

   clist_row_selected=row;

   mcont = gtk_clist_get_row_data(GTK_CLIST(clist), row);
   if (mcont==NULL) {
      return;
   }

   if (mcont->rt == DELETED_PALM_REC ||
      (mcont->rt == DELETED_PC_REC))
      /* Possible later addition of undelete code for modified deleted records
         || mcont->rt == MODIFIED_PALM_REC
      */
   {
      set_new_button_to(UNDELETE_FLAG);
   } else {
      set_new_button_to(CLEAR_FLAG);
   }

   connect_changed_signals(DISCONNECT_SIGNALS);

   if (mcont->cont.picture && mcont->cont.picture->data) {
      /* Set global variables to keep the picture data */
      contact_picture.data=malloc(mcont->cont.picture->length);
      memcpy(contact_picture.data, mcont->cont.picture->data, mcont->cont.picture->length);
      contact_picture.length = mcont->cont.picture->length;
      contact_picture.dirty = 0;
      if (image) gtk_widget_destroy(image);
      image = image_from_data(mcont->cont.picture->data, mcont->cont.picture->length);
      //gtk_box_pack_start(GTK_BOX(picture_box), image, FALSE, FALSE, 0);
      gtk_container_add(GTK_CONTAINER(picture_button), image);
      gtk_widget_show(image);   
   } else {
      if (image) {
	 gtk_widget_destroy(image);
	 image=NULL;
      }
      if (contact_picture.data) {
	 free(contact_picture.data);
	 contact_picture.dirty=0;
	 contact_picture.length=0;
	 contact_picture.data=NULL;
      }
   }

   cont=&(mcont->cont);
   clist_text = NULL;
   gtk_clist_get_text(GTK_CLIST(clist), row, ADDRESS_NAME_COLUMN, &clist_text);
   entry_text = gtk_entry_get_text(GTK_ENTRY(address_quickfind_entry));
   if (strncasecmp(clist_text, entry_text, strlen(entry_text))) {
      gtk_entry_set_text(GTK_ENTRY(address_quickfind_entry), "");
   }

   /* category menu */
   cat = mcont->attrib & 0x0F;
   sorted_position = find_sorted_cat(cat);
   if (address_cat_menu_item2[sorted_position]==NULL) {
      /* Illegal category, Assume that category 0 is Unfiled and valid*/
      jp_logf(JP_LOG_DEBUG, "Category is not legal\n");
      cat = sorted_position = 0;
      sorted_position = find_sorted_cat(cat);
   }
   /* We need to count how many items down in the list this is */
   for (i=sorted_position, count=0; i>=0; i--) {
      if (address_cat_menu_item2[i]) {
	 count++;
      }
   }
   count--;

   if (sorted_position<0) {
      jp_logf(JP_LOG_WARN, _("Category is not legal\n"));
   } else {
      if (address_cat_menu_item2[sorted_position]) {
	 gtk_check_menu_item_set_active
	   (GTK_CHECK_MENU_ITEM(address_cat_menu_item2[sorted_position]), TRUE);
      }
      gtk_option_menu_set_history(GTK_OPTION_MENU(category_menu2), count);
   }
   /* End category menu */

   //undo what is text?
#ifdef ENABLE_GTK2
   gtk_widget_freeze_child_notify(text);

   gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_txt_buf_text), "", -1);
#else
   gtk_text_freeze(GTK_TEXT(text));

   gtk_text_set_point(GTK_TEXT(text), 0);
   gtk_text_forward_delete(GTK_TEXT(text),
			   gtk_text_get_length(GTK_TEXT(text)));
#endif

// NEW CODE
   /* Fill out the "All" text buffer */
   gtk_text_insert(GTK_TEXT(text), NULL,NULL,NULL, _("Category: "), -1);
   gtk_text_insert(GTK_TEXT(text), NULL,NULL,NULL,
		   contact_app_info.category.name[mcont->attrib & 0x0F], -1);
   gtk_text_insert(GTK_TEXT(text), NULL,NULL,NULL, "\n", -1);
   
   s = contact_to_gstring(cont);
   gtk_text_insert(GTK_TEXT(text), NULL,NULL,NULL, s->str, -1);
   // printf("[%s]\n",s->str);
   g_string_free(s, TRUE);

   address_i=phone_i=IM_i=0;
   for (i=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_LABEL_TEXT:
	 goto set_text;
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	 /* Set dial/email button text and callback data */
	 if (!strcmp(contact_app_info.phoneLabels[cont->phoneLabel[phone_i]], _("E-mail"))) {
	    gtk_object_set_data(GTK_OBJECT(dial_button[phone_i]), "mail", GINT_TO_POINTER(1));
#ifdef ENABLE_GTK2
	    gtk_button_set_label(GTK_BUTTON(dial_button[phone_i]), _("Mail"));
#else
	    gtk_object_set(GTK_OBJECT(dial_button[phone_i]), "label", _("Mail"), NULL);
#endif
	 } else {
	    gtk_object_set_data(GTK_OBJECT(dial_button[phone_i]), "mail", 0);
#ifdef ENABLE_GTK2
	    gtk_button_set_label(GTK_BUTTON(dial_button[phone_i]), _("Dial"));
#else
	    gtk_object_set(GTK_OBJECT(dial_button[phone_i]), "label", _("Dial"), NULL);
#endif
	 }
	 //undo #defines?
	 if ((phone_i<7) && (cont->phoneLabel[phone_i] < 8)) {
	    if (GTK_IS_WIDGET(phone_type_menu_item[phone_i][cont->phoneLabel[phone_i]])) {
	       gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					      (phone_type_menu_item[phone_i][cont->phoneLabel[phone_i]]), TRUE);
	       gtk_option_menu_set_history(GTK_OPTION_MENU(phone_type_list_menu[phone_i]),
					   cont->phoneLabel[phone_i]);
	    }
	 }
	 phone_i++;
	 goto set_text;
       case ADDRESS_GUI_IM_MENU_TEXT:
	 if (GTK_IS_WIDGET(IM_type_menu_item[IM_i][cont->IMLabel[IM_i]])) {
	    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					   (IM_type_menu_item[IM_i][cont->IMLabel[IM_i]]), TRUE);
	    gtk_option_menu_set_history(GTK_OPTION_MENU(IM_type_list_menu[IM_i]),
					cont->IMLabel[IM_i]);
	 }
	 IM_i++;
	 goto set_text;
       case ADDRESS_GUI_ADDR_MENU_TEXT:
	 if (GTK_IS_WIDGET(address_type_menu_item[address_i][cont->addressLabel[address_i]])) {
	    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					   (address_type_menu_item[address_i][cont->addressLabel[address_i]]), TRUE);
	    gtk_option_menu_set_history(GTK_OPTION_MENU(address_type_list_menu[address_i]),
					cont->addressLabel[address_i]);
	    /* Set the label on the notebook to match the type of address */
	    if (GTK_IS_LABEL(notebook_label[address_i+1])) {
	       gtk_label_set_text(GTK_LABEL(notebook_label[address_i+1]), contact_app_info.addrLabels[cont->addressLabel[address_i]]);
	    }
	 }
	 address_i++;
	 goto set_text;
       case ADDRESS_GUI_WEBSITE:
	 set_text:
#ifdef ENABLE_GTK2
	 gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_txt_buf_address_text[schema[i].record_field]), "", -1);
#else
	 gtk_text_set_point(GTK_TEXT(address_text[schema[i].record_field]), 0);
	 gtk_text_forward_delete(GTK_TEXT(address_text[schema[i].record_field]),
				 gtk_text_get_length(GTK_TEXT(address_text[schema[i].record_field])));
#endif
	 if (cont->entry[schema[i].record_field]) {
	    gtk_text_insert(GTK_TEXT(address_text[schema[i].record_field]),
			    NULL,NULL,NULL, cont->entry[schema[i].record_field], -1);
	 }
	 break;
       case ADDRESS_GUI_BIRTHDAY:
	 if (cont->birthdayFlag) {
	    memcpy(&birthday, &cont->birthday, sizeof(struct tm));
	    set_button_label_to_date(birthday_button, &birthday);

	    /* Birthday checkbox */
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(birthday_checkbox),
					 TRUE);

	    if (cont->reminder) {
	       sprintf(birthday_str, "%d", cont->advance);
	       gtk_entry_set_text(GTK_ENTRY(reminder_entry), birthday_str);

	       /* Reminder checkbox */
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reminder_checkbox),
					    cont->reminder);
	    }
	 }
	 break;
      }
   }

   /* Set phone grouped radio buttons */
   if ((cont->showPhone > -1) && (cont->showPhone < NUM_PHONE_ENTRIES)) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[cont->showPhone]),
				   TRUE);
   }

   /* Private checkbox */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private_checkbox),
				mcont->attrib & dlpRecAttrSecret);

#ifdef ENABLE_GTK2
   gtk_widget_thaw_child_notify(text);
#else
   gtk_text_thaw(GTK_TEXT(text));
#endif
   connect_changed_signals(CONNECT_SIGNALS);
}

static gboolean cb_key_pressed_left_side(GtkWidget   *widget, 
                                         GdkEventKey *event)
{
   GtkWidget *entry_widget;
#ifdef ENABLE_GTK2
   GtkTextBuffer *text_buffer;
   GtkTextIter    iter;
#endif

   if (event->keyval == GDK_Return) {
      gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");

      switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))) {
        case 0 :
	   entry_widget = address_text[contLastname];
           break;
        case 1 :
	   entry_widget = address_text[contAddress1];
           break;
        case 2 :
	   entry_widget = address_text[contAddress2];
           break;
        case 3 :
	   entry_widget = address_text[contAddress3];
           break;
        case 4 :
	   entry_widget = address_text[contCustom1];
           break;
        default:
	   entry_widget = address_text[0];
      }
      gtk_widget_grab_focus(entry_widget);

#ifdef ENABLE_GTK2
      /* Position cursor at start of text */
      text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry_widget));
      gtk_text_buffer_get_start_iter(text_buffer, &iter);
      gtk_text_buffer_place_cursor(text_buffer, &iter);
#endif

      return TRUE;
   }

   return FALSE;
}

static gboolean cb_key_pressed_right_side(GtkWidget   *widget, 
                                          GdkEventKey *event)
{
   if ((event->keyval == GDK_Return) && (event->state & GDK_SHIFT_MASK)) {
      gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");
      /* Call clist_selection to handle any cleanup such as a modified record */
      cb_clist_selection(clist, clist_row_selected, ADDRESS_PHONE_COLUMN, 
	                 GINT_TO_POINTER(1), NULL);
      gtk_widget_grab_focus(GTK_WIDGET(clist));
      return TRUE;
   }

   return FALSE;
}

static void address_update_clist(GtkWidget *clist, GtkWidget *tooltip_widget,
				 ContactList **cont_list, int category, int main)
{
   int num_entries, entries_shown, i;
   int show1, show2, show3;
   gchar *empty_line[] = { "","","" };
   GdkPixmap *pixmap_note;
   GdkBitmap *mask_note;
   ContactList *temp_cl;
   char str[ADDRESS_MAX_CLIST_NAME+8];
   char str2[ADDRESS_MAX_COLUMN_LEN+2];
   int show_priv;
   long use_jos, char_set;
   char *tmp_p1, *tmp_p2, *tmp_p3;
   char blank[]="";
   char slash[]=" / ";
   char comma_space[]=", ";
   char *field1, *field2, *field3;
   char *delim1, *delim2;
   char *tmp_delim1, *tmp_delim2;
   AddressList *addr_list;

   free_ContactList(cont_list);

   if (address_version==0) {
      addr_list = NULL;
      num_entries = get_addresses2(&addr_list, SORT_ASCENDING, 2, 2, 1, CATEGORY_ALL);
      copy_addresses_to_contacts(addr_list, cont_list);
      free_AddressList(&addr_list);
   } else {
      /* Need to get all records including private ones for the tooltips calculation */
      num_entries = get_contacts2(cont_list, SORT_ASCENDING, 2, 2, 1, CATEGORY_ALL);
   }

   /* Start by clearing existing entry if in main window */
   if (main) {
      addr_clear_details();
   }


   /* Clear the text box to make things look nice */
   if (main) {
#ifdef ENABLE_GTK2
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_txt_buf_text), "", -1);
#else
      gtk_text_set_point(GTK_TEXT(text), 0);
      gtk_text_forward_delete(GTK_TEXT(text),
			      gtk_text_get_length(GTK_TEXT(text)));
#endif
   }

   /* Freeze clist to prevent flicker during updating */
   gtk_clist_freeze(GTK_CLIST(clist));
   if (main)
	   gtk_signal_disconnect_by_func(GTK_OBJECT(clist),
				 GTK_SIGNAL_FUNC(cb_clist_selection), NULL);
   gtk_clist_clear(GTK_CLIST(clist));

   /* Collect preferences and pixmaps before loop */
   get_pref(PREF_CHAR_SET, &char_set, NULL);
   get_pref(PREF_USE_JOS, &use_jos, NULL);
   show_priv = show_privates(GET_PRIVATES);
   get_pixmaps(clist, PIXMAP_NOTE, &pixmap_note, &mask_note);

   if (sort_by_company) {
      show1=contCompany;
      show2=contLastname;
      show3=contFirstname;
      delim1 = slash;
      delim2 = comma_space;
   } else {
      show1=contLastname;
      show2=contFirstname;
      show3=contCompany;
      delim1 = comma_space;
      delim2 = slash;
   }

   entries_shown=0;

   for (temp_cl = *cont_list, i=0; temp_cl; temp_cl=temp_cl->next) {
      if ( ((temp_cl->mcont.attrib & 0x0F) != category) &&
	   category != CATEGORY_ALL) {
	 continue;
      }
      
      /* Do masking like Palm OS 3.5 */
      if ((show_priv == MASK_PRIVATES) &&
	  (temp_cl->mcont.attrib & dlpRecAttrSecret)) {
	 gtk_clist_append(GTK_CLIST(clist), empty_line);
	 gtk_clist_set_text(GTK_CLIST(clist), entries_shown, ADDRESS_NAME_COLUMN, "---------------");
	 gtk_clist_set_text(GTK_CLIST(clist), entries_shown, ADDRESS_PHONE_COLUMN, "---------------");
	 clear_mycontact(&temp_cl->mcont);
	 gtk_clist_set_row_data(GTK_CLIST(clist), entries_shown, &(temp_cl->mcont));
	 gtk_clist_set_row_style(GTK_CLIST(clist), entries_shown, NULL);
	 entries_shown++;
	 continue;
      }
      /* End Masking */

      /* Hide the private records if need be */
      if ((show_priv != SHOW_PRIVATES) &&
	  (temp_cl->mcont.attrib & dlpRecAttrSecret)) {
	 continue;
      }

      if (!use_jos && (char_set == CHAR_SET_JAPANESE || char_set == CHAR_SET_SJIS_UTF)) {
	 str[0]='\0';
	 if (temp_cl->mcont.cont.entry[show1] || temp_cl->mcont.cont.entry[show2]) {
	    if (temp_cl->mcont.cont.entry[show1] && temp_cl->mcont.cont.entry[show2]) {
	       if ((tmp_p1 = strchr(temp_cl->mcont.cont.entry[show1],'\1'))) *tmp_p1='\0';
	       if ((tmp_p2 = strchr(temp_cl->mcont.cont.entry[show2],'\1'))) *tmp_p2='\0';
	       g_snprintf(str, ADDRESS_MAX_CLIST_NAME, "%s, %s", temp_cl->mcont.cont.entry[show1], temp_cl->mcont.cont.entry[show2]);
	       if (tmp_p1) *tmp_p1='\1';
	       if (tmp_p2) *tmp_p2='\1';
	    }
	    if (temp_cl->mcont.cont.entry[show1] && ! temp_cl->mcont.cont.entry[show2]) {
	       if ((tmp_p1 = strchr(temp_cl->mcont.cont.entry[show1],'\1'))) *tmp_p1='\0';
	       if (temp_cl->mcont.cont.entry[show3]) {
		  if ((tmp_p3 = strchr(temp_cl->mcont.cont.entry[show3],'\1'))) *tmp_p3='\0';
		  g_snprintf(str, ADDRESS_MAX_CLIST_NAME, "%s, %s", temp_cl->mcont.cont.entry[show1], temp_cl->mcont.cont.entry[show3]);
		  if (tmp_p3) *tmp_p3='\1';
	       } else {
		  multibyte_safe_strncpy(str, temp_cl->mcont.cont.entry[show1], ADDRESS_MAX_CLIST_NAME);
	       }
	       if (tmp_p1) *tmp_p1='\1';
	    }
	    if (! temp_cl->mcont.cont.entry[show1] && temp_cl->mcont.cont.entry[show2]) {
	       if ((tmp_p2 = strchr(temp_cl->mcont.cont.entry[show2],'\1'))) *tmp_p2='\0';
	       multibyte_safe_strncpy(str, temp_cl->mcont.cont.entry[show2], ADDRESS_MAX_CLIST_NAME);
	       if (tmp_p2) *tmp_p2='\1';
	    }
	 } else if (temp_cl->mcont.cont.entry[show3]) {
	    if ((tmp_p3 = strchr(temp_cl->mcont.cont.entry[show3],'\1'))) *tmp_p3='\0';
	    multibyte_safe_strncpy(str, temp_cl->mcont.cont.entry[show3], ADDRESS_MAX_CLIST_NAME);
	    if (tmp_p3) *tmp_p3='\1';
	 } else {
	    strcpy(str, _("-Unnamed-"));
	 }
	 gtk_clist_append(GTK_CLIST(clist), empty_line);
      } else {
	 str[0]='\0';
	 field1=field2=field3=blank;
	 tmp_delim1=delim1;
	 tmp_delim2=delim2;
	 if (temp_cl->mcont.cont.entry[show1]) field1=temp_cl->mcont.cont.entry[show1];
	 if (temp_cl->mcont.cont.entry[show2]) field2=temp_cl->mcont.cont.entry[show2];
	 if (temp_cl->mcont.cont.entry[show3]) field3=temp_cl->mcont.cont.entry[show3];
	 if (sort_by_company) {
	    /* Company / Last, First */
	    if (!(field1[0])) tmp_delim1=blank;
	    if ((!field2[0]) || (!field3[0])) tmp_delim2=blank;
	    if ((!field2[0]) && (!field3[0])) tmp_delim1=blank;
	 } else {
	    /* Last, First / Company */
	    if ((!field1[0]) || (!field2[0])) tmp_delim1=blank;
	    if (!(field3[0])) tmp_delim2=blank;
	    if ((!field1[0]) && (!field2[0])) tmp_delim2=blank;
	 }
	 g_snprintf(str, ADDRESS_MAX_CLIST_NAME, "%s%s%s%s%s",
		    field1, tmp_delim1, field2, tmp_delim2, field3);
	 if (strlen(str)<1) strcpy(str, _("-Unnamed-"));
	 str[ADDRESS_MAX_CLIST_NAME]='\0';

	 gtk_clist_append(GTK_CLIST(clist), empty_line);
      }

      lstrncpy_remove_cr_lfs(str2, str, ADDRESS_MAX_COLUMN_LEN);
      gtk_clist_set_text(GTK_CLIST(clist), entries_shown, ADDRESS_NAME_COLUMN, str2);
      /* Clear string so previous data won't be used inadvertently in next set_text */
      str2[0] = '\0';
      lstrncpy_remove_cr_lfs(str2, temp_cl->mcont.cont.entry[temp_cl->mcont.cont.showPhone + 4], ADDRESS_MAX_COLUMN_LEN);
      gtk_clist_set_text(GTK_CLIST(clist), entries_shown, ADDRESS_PHONE_COLUMN, str2);
      gtk_clist_set_row_data(GTK_CLIST(clist), entries_shown, &(temp_cl->mcont));

      /* Highlight row background depending on status */
      switch (temp_cl->mcont.rt) {
       case NEW_PC_REC:
       case REPLACEMENT_PALM_REC:
	 set_bg_rgb_clist_row(clist, entries_shown,
			      CLIST_NEW_RED, CLIST_NEW_GREEN, CLIST_NEW_BLUE);
	 break;
       case DELETED_PALM_REC:
       case DELETED_PC_REC:
	 set_bg_rgb_clist_row(clist, entries_shown,
			      CLIST_DEL_RED, CLIST_DEL_GREEN, CLIST_DEL_BLUE);
	 break;
       case MODIFIED_PALM_REC:
	 set_bg_rgb_clist_row(clist, entries_shown,
			      CLIST_MOD_RED, CLIST_MOD_GREEN, CLIST_MOD_BLUE);
	 break;
       default:
	 if (temp_cl->mcont.attrib & dlpRecAttrSecret) {
	    set_bg_rgb_clist_row(clist, entries_shown,
				 CLIST_PRIVATE_RED, CLIST_PRIVATE_GREEN, CLIST_PRIVATE_BLUE);
	 } else {
	    gtk_clist_set_row_style(GTK_CLIST(clist), entries_shown, NULL);
	 }
      }

      /* Put a note pixmap up */
      if (temp_cl->mcont.cont.entry[contNote]) {
	 gtk_clist_set_pixmap(GTK_CLIST(clist), entries_shown, ADDRESS_NOTE_COLUMN, pixmap_note, mask_note);
      } else {
	 gtk_clist_set_text(GTK_CLIST(clist), entries_shown, ADDRESS_NOTE_COLUMN, "");
      }

      entries_shown++;
   }

   gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		      GTK_SIGNAL_FUNC(cb_clist_selection), NULL);

   /* If there are items in the list, highlight the selected row */
   if ((main) && (entries_shown>0)) {
      /* Select the existing requested row, or row 0 if that is impossible */
      if (clist_row_selected < entries_shown)
      {
	 clist_select_row(GTK_CLIST(clist), clist_row_selected, ADDRESS_PHONE_COLUMN);
	 if (!gtk_clist_row_is_visible(GTK_CLIST(clist), clist_row_selected)) {
	    gtk_clist_moveto(GTK_CLIST(clist), clist_row_selected, 0, 0.5, 0.0);
	 }
      }
      else
      {
	 clist_select_row(GTK_CLIST(clist), 0, ADDRESS_PHONE_COLUMN);
      }
   }

   /* Unfreeze clist after all changes */
   gtk_clist_thaw(GTK_CLIST(clist));

   if (tooltip_widget) {
      if (cont_list==NULL) {
	 gtk_tooltips_set_tip(glob_tooltips, category_menu1, _("0 records"), NULL);
      }
      else {
	 sprintf(str, _("%d of %d records"), entries_shown, num_entries);
	 gtk_tooltips_set_tip(glob_tooltips, category_menu1, str, NULL);
      }
   }

   /* return focus to clist after any big operation which requires a redraw */
   gtk_widget_grab_focus(GTK_WIDGET(clist));

}

/* default set is which menu item is to be set on by default */
/* set is which set in the phone_type_menu_item array to use */
static int make_IM_type_menu(int default_set, unsigned int callback_id, int set)
{
   int i;
   GSList *group;
   GtkWidget *menu;

   IM_type_list_menu[set] = gtk_option_menu_new();

   menu = gtk_menu_new();
   group = NULL;

   //undo define for 5
   for (i=0; i<5; i++) {
      if (contact_app_info.IMLabels[i][0]) {
	 IM_type_menu_item[set][i] = gtk_radio_menu_item_new_with_label(
			group, contact_app_info.IMLabels[i]);
	 gtk_signal_connect(GTK_OBJECT(IM_type_menu_item[set][i]), "activate",
			    GTK_SIGNAL_FUNC(cb_IM_type_menu),
			    GINT_TO_POINTER(callback_id<<8 |i));
	 group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(IM_type_menu_item[set][i]));
	 gtk_menu_append(GTK_MENU(menu), IM_type_menu_item[set][i]);
	 gtk_widget_show(IM_type_menu_item[set][i]);

	 changed_list = g_list_prepend(changed_list, IM_type_menu_item[set][i]);
      }
   }
   /*Set this one to active */
   if (GTK_IS_WIDGET(IM_type_menu_item[set][default_set])) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				     IM_type_menu_item[set][default_set]), TRUE);
   }

   gtk_option_menu_set_menu(GTK_OPTION_MENU(IM_type_list_menu[set]), menu);
   /*Make this one show up by default */
   gtk_option_menu_set_history(GTK_OPTION_MENU(IM_type_list_menu[set]),
			       default_set);

   gtk_widget_show(IM_type_list_menu[set]);

   return EXIT_SUCCESS;
}


//undo rewrite this crappy function
/* default set is which menu item is to be set on by default */
/* set is which set in the menu_item array to use */
static int make_address_type_menu(int default_set, int set)
{
   int i;
   GSList *group;
   GtkWidget *menu;

   address_type_list_menu[set] = gtk_option_menu_new();

   menu = gtk_menu_new();
   group = NULL;

   //undo define for 3
   for (i=0; i<3; i++) {
      if (contact_app_info.addrLabels[i][0]) {
	 address_type_menu_item[set][i] = gtk_radio_menu_item_new_with_label(
			group, contact_app_info.addrLabels[i]);
	 gtk_signal_connect(GTK_OBJECT(address_type_menu_item[set][i]), "activate",
			    GTK_SIGNAL_FUNC(cb_address_type_menu),
			    GINT_TO_POINTER(set<<8 | i));
	 group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(address_type_menu_item[set][i]));
	 gtk_menu_append(GTK_MENU(menu), address_type_menu_item[set][i]);
	 gtk_widget_show(address_type_menu_item[set][i]);

      	 changed_list = g_list_prepend(changed_list, address_type_menu_item[set][i]);
      }
   }
   /*Set this one to active */
   if (GTK_IS_WIDGET(address_type_menu_item[set][default_set])) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				     address_type_menu_item[set][default_set]), TRUE);
   }

   gtk_option_menu_set_menu(GTK_OPTION_MENU(address_type_list_menu[set]), menu);
   /*Make this one show up by default */
   gtk_option_menu_set_history(GTK_OPTION_MENU(address_type_list_menu[set]),
			       default_set);

   gtk_widget_show(address_type_list_menu[set]);

   return EXIT_SUCCESS;
}

/* default set is which menu item is to be set on by default */
/* set is which set in the phone_type_menu_item array to use */
static int make_phone_menu(int default_set, unsigned int callback_id, int set)
{
   int i;
   GSList *group;
   GtkWidget *menu;

   phone_type_list_menu[set] = gtk_option_menu_new();

   menu = gtk_menu_new();
   group = NULL;

   for (i=0; i<8; i++) {
      if (contact_app_info.phoneLabels[i][0]) {
	 phone_type_menu_item[set][i] = gtk_radio_menu_item_new_with_label(
			group, contact_app_info.phoneLabels[i]);
	 gtk_signal_connect(GTK_OBJECT(phone_type_menu_item[set][i]), "activate",
			    GTK_SIGNAL_FUNC(cb_phone_menu),
			    GINT_TO_POINTER(callback_id<<8 | i));
	 group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(phone_type_menu_item[set][i]));
	 gtk_menu_append(GTK_MENU(menu), phone_type_menu_item[set][i]);
	 gtk_widget_show(phone_type_menu_item[set][i]);

	 changed_list = g_list_prepend(changed_list, phone_type_menu_item[set][i]);
      }
   }
   /*Set this one to active */
   if (GTK_IS_WIDGET(phone_type_menu_item[set][default_set])) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				     phone_type_menu_item[set][default_set]), TRUE);
   }

   gtk_option_menu_set_menu(GTK_OPTION_MENU(phone_type_list_menu[set]), menu);
   /*Make this one show up by default */
   gtk_option_menu_set_history(GTK_OPTION_MENU(phone_type_list_menu[set]),
			       default_set);

   gtk_widget_show(phone_type_list_menu[set]);

   return EXIT_SUCCESS;
}

/* returns 1 if found, 0 if not */
static int address_find()
{
   int r, found_at;

   r = 0;
   if (glob_find_id) {
      r = clist_find_id(clist,
			glob_find_id,
			&found_at);
      if (r) {
	 clist_select_row(GTK_CLIST(clist), found_at, ADDRESS_PHONE_COLUMN);
	 if (!gtk_clist_row_is_visible(GTK_CLIST(clist), found_at)) {
	    gtk_clist_moveto(GTK_CLIST(clist), found_at, 0, 0.5, 0.0);
	 }
      }
      glob_find_id = 0;
   }
   return r;
}

/* This redraws the clist */
int address_clist_redraw()
{
   address_update_clist(clist, category_menu1, &glob_contact_list,
			address_category, TRUE);

   return EXIT_SUCCESS;
}

int address_cycle_cat()
{
   int b;
   int i, new_cat;

   b=dialog_save_changed_record(pane, record_changed);
   if (b==DIALOG_SAID_2) {
      cb_add_new_record(NULL, GINT_TO_POINTER(record_changed));
   }

   if (address_category == CATEGORY_ALL) {
      new_cat = -1;
   } else {
      new_cat = find_sorted_cat(address_category);
   }
   for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
      new_cat++;
      if (new_cat >= NUM_ADDRESS_CAT_ITEMS) {
	 address_category = CATEGORY_ALL;
	 break;
      }
      if ((sort_l[new_cat].Pcat) && (sort_l[new_cat].Pcat[0])) {
	 address_category = sort_l[new_cat].cat_num;
	 break;
      }
   }
   clist_row_selected = 0;

   return EXIT_SUCCESS;
}

int address_refresh()
{
   int index;

   if (glob_find_id) {
      address_category = CATEGORY_ALL;
   }
   if (address_category==CATEGORY_ALL) {
      index=0;
   } else {
      index=find_sorted_cat(address_category)+1;
   }
   address_update_clist(clist, category_menu1, &glob_contact_list,
			address_category, TRUE);
   if (index<0) {
      jp_logf(JP_LOG_WARN, _("Category is not legal\n"));
   } else {
      gtk_option_menu_set_history(GTK_OPTION_MENU(category_menu1), index);
      gtk_check_menu_item_set_active
	(GTK_CHECK_MENU_ITEM(address_cat_menu_item1[index]), TRUE);
   }
   address_find();

   /* gives the focus to the search field */
   gtk_widget_grab_focus(address_quickfind_entry);

   return EXIT_SUCCESS;
}


static gboolean
cb_key_pressed_quickfind(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
   int row_count;
   int select_row;
   int add;

   add=0;
   if ((event->keyval == GDK_KP_Down) || (event->keyval == GDK_Down)) {
      add=1;
   }
   if ((event->keyval == GDK_KP_Up) || (event->keyval == GDK_Up)) {
      add=-1;
   }
   if (!add) return FALSE;
   row_count =GTK_CLIST(clist)->rows;
   if (!row_count) return FALSE;

   gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");

   select_row=clist_row_selected+add;
   if (select_row>row_count-1) {
      select_row=0;
   }
   if (select_row<0) {
      select_row=row_count-1;
   }
   clist_select_row(GTK_CLIST(clist), select_row, ADDRESS_NAME_COLUMN);
   if (!gtk_clist_row_is_visible(GTK_CLIST(clist), select_row)) {
      gtk_clist_moveto(GTK_CLIST(clist), select_row, 0, 0.5, 0.0);
   }
   return TRUE;
}
   
static gboolean
cb_key_pressed(GtkWidget *widget, GdkEventKey *event)
{
#ifdef ENABLE_GTK2
   GtkTextIter    cursor_pos_iter;
   GtkTextBuffer *text_buffer;
#endif
   int page;
   int first, next;
   int i, j, found;

   if ((event->keyval != GDK_Tab) &&
       (event->keyval != GDK_ISO_Left_Tab)) {
      return FALSE;
   }
   /* See if they are at the end of the text */
#ifdef ENABLE_GTK2
   text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
   gtk_text_buffer_get_iter_at_mark(text_buffer,&cursor_pos_iter,gtk_text_buffer_get_insert(text_buffer));
   if (!(gtk_text_iter_is_end(&cursor_pos_iter)))
#else
   if (!(gtk_text_get_point(GTK_TEXT(widget)) ==
       gtk_text_get_length(GTK_TEXT(widget))))
#endif
       {
	  return FALSE;
       }
   gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");

   page = found = 0;
   next = first = -1;
   for (i=j=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_LABEL_TEXT:
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
       case ADDRESS_GUI_ADDR_MENU_TEXT:
       case ADDRESS_GUI_IM_MENU_TEXT:
       case ADDRESS_GUI_WEBSITE:
	 if (first < 0) {
	    page = schema[i].notebook_page;
	    first = next = schema[i].record_field;
	 }
	 if (found) {
	    page = schema[i].notebook_page;
	    next = schema[i].record_field;
	    i = 10000;
	    break;
	 }
	 if (address_text[schema[i].record_field]==widget) {
	    found = 1;
	 }
	 j++;
      }
   }
   gtk_notebook_set_page(GTK_NOTEBOOK(notebook), page);
   gtk_widget_grab_focus(GTK_WIDGET(address_text[next]));

   return TRUE;
}

int address_gui_cleanup()
{
   int b;

   b=dialog_save_changed_record(pane, record_changed);
   if (b==DIALOG_SAID_2) {
      cb_add_new_record(NULL, GINT_TO_POINTER(record_changed));
   }

   g_list_free(changed_list);
   changed_list=NULL;

   //undo free_AddressList(&glob_address_list);
   free_ContactList(&glob_contact_list);
   free_ContactList(&export_contact_list);
   connect_changed_signals(DISCONNECT_SIGNALS);
#ifdef ENABLE_GTK2
   set_pref(PREF_ADDRESS_PANE, gtk_paned_get_position(GTK_PANED(pane)), NULL, TRUE);
#else
   set_pref(PREF_ADDRESS_PANE, GTK_PANED(pane)->handle_xpos, NULL, TRUE);
#endif
   set_pref(PREF_LAST_ADDR_CATEGORY, address_category, NULL, TRUE);

   if (contact_picture.data) {
      free(contact_picture.data);
   }
   contact_picture.dirty=0;
   contact_picture.length=0;
   contact_picture.data=NULL;

   return EXIT_SUCCESS;
}

/*
 * Main function
 */
int address_gui(GtkWidget *vbox, GtkWidget *hbox)
{
   extern GtkWidget *glob_date_label;
   extern int glob_date_timer_tag;
   GtkWidget *scrolled_window;
   GtkWidget *pixmapwid;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkWidget *vbox1, *vbox2;
   GtkWidget *hbox_temp;
   GtkWidget *vbox_temp;
   GtkWidget *separator;
   GtkWidget *label;
   GtkWidget *button;
   GtkWidget *table;
   GtkWidget *notebook_tab;
   GSList *group;
   long ivalue, notebook_page;
   char *titles[]={"","",""};
   GtkAccelGroup *accel_group;
   int address_type_i, IM_type_i, page_i, table_y_i;
   int x, y;

   int i, phone_i, num_pages;
//   long use_jos, char_set;
   long char_set;
   char *cat_name;

   char *contact_page_names[]={
      N_("Name"),
	N_("Address"),
	N_("Address"),
	N_("Address"),
	N_("Other")
   };
   char *address_page_names[]={
      N_("Name"),
	N_("Address"),
	N_("Other")
   };
   char **page_names;

   clist_row_selected=0;

   get_pref(PREF_ADDRESS_VERSION, &address_version, NULL);

   init();

   if (address_version) {
      page_names = contact_page_names;
      num_pages=5;
      get_contact_app_info(&contact_app_info);
   } else {
      page_names = address_page_names;
      num_pages=3;
      get_address_app_info(&address_app_info);
      copy_address_ai_to_contact_ai(&address_app_info, &contact_app_info);
   }

   get_pref(PREF_CHAR_SET, &char_set, NULL);
   for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
      cat_name = charset_p2newj(contact_app_info.category.name[i], 31, char_set);
      strcpy(sort_l[i].Pcat, cat_name);
      free(cat_name);
      sort_l[i].cat_num=i;
   }

   qsort(sort_l, NUM_ADDRESS_CAT_ITEMS, sizeof(struct sorted_cats), cat_compare);
#ifdef JPILOT_DEBUG
   for (i=0; i<NUM_ADDRESS_CAT_ITEMS; i++) {
      printf("cat %d [%s]\n", sort_l[i].cat_num, sort_l[i].Pcat);
   }
#endif

   get_pref(PREF_LAST_ADDR_CATEGORY, &ivalue, NULL);
   address_category = ivalue;

   if (contact_app_info.category.name[address_category][0]=='\0') {
      address_category=CATEGORY_ALL;
   }

   accel_group = gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(gtk_widget_get_toplevel(vbox)),
			      accel_group);

   pane = gtk_hpaned_new();
   get_pref(PREF_ADDRESS_PANE, &ivalue, NULL);
   gtk_paned_set_position(GTK_PANED(pane), ivalue + PANE_CREEP);

   gtk_box_pack_start(GTK_BOX(hbox), pane, TRUE, TRUE, 5);

   vbox1 = gtk_vbox_new(FALSE, 0);
   vbox2 = gtk_vbox_new(FALSE, 0);
   gtk_paned_pack1(GTK_PANED(pane), vbox1, TRUE, FALSE);
   gtk_paned_pack2(GTK_PANED(pane), vbox2, TRUE, FALSE);

   /* Separator */
   separator = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox1), separator, FALSE, FALSE, 5);

   /* Make the Today is: label */
   glob_date_label = gtk_label_new(" ");
   gtk_box_pack_start(GTK_BOX(vbox1), glob_date_label, FALSE, FALSE, 0);
   timeout_date(NULL);
   glob_date_timer_tag = gtk_timeout_add(get_timeout_interval(), timeout_date, NULL);

   /* Separator */
   separator = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox1), separator, FALSE, FALSE, 5);

   /* Category Box */
   hbox_temp = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox1), hbox_temp, FALSE, FALSE, 0);

   /* Put the category menu up */
   make_category_menu(&category_menu1, address_cat_menu_item1,
		      sort_l, cb_category, TRUE);
   gtk_box_pack_start(GTK_BOX(hbox_temp), category_menu1, TRUE, TRUE, 0);

   /* Edit category button */
   button = gtk_button_new_with_label(_("Edit Categories"));
   gtk_signal_connect(GTK_OBJECT(button), "clicked",
		      GTK_SIGNAL_FUNC(cb_edit_cats), NULL);
   gtk_box_pack_start(GTK_BOX(hbox_temp), button, FALSE, FALSE, 0);


   /* Put the address list window up */
   scrolled_window = gtk_scrolled_window_new(NULL, NULL);
   /*gtk_widget_set_usize(GTK_WIDGET(scrolled_window), 150, 0); */
   gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start(GTK_BOX(vbox1), scrolled_window, TRUE, TRUE, 0);

   clist = gtk_clist_new_with_titles(3, titles);
   
   gtk_clist_column_title_passive(GTK_CLIST(clist), ADDRESS_PHONE_COLUMN);
   gtk_clist_column_title_passive(GTK_CLIST(clist), ADDRESS_NOTE_COLUMN);

   /* Initialize sort_by_company the first time program is called */ 
   if (sort_by_company == -1) {
      sort_by_company = contact_app_info.sortByCompany;
   }

   if (sort_by_company) {
      gtk_clist_set_column_title(GTK_CLIST(clist), ADDRESS_NAME_COLUMN, _("Company/Name"));
   } else {
      gtk_clist_set_column_title(GTK_CLIST(clist), ADDRESS_NAME_COLUMN, _("Name/Company"));
   }
   gtk_signal_connect(GTK_OBJECT(GTK_CLIST(clist)->column[ADDRESS_NAME_COLUMN].button),
		      "clicked", GTK_SIGNAL_FUNC(cb_resort), NULL);
   gtk_clist_set_column_title(GTK_CLIST(clist), ADDRESS_PHONE_COLUMN, _("Phone"));
   /* Put pretty pictures in the clist column headings */
   get_pixmaps(vbox, PIXMAP_NOTE, &pixmap, &mask);
   pixmapwid = gtk_pixmap_new(pixmap, mask);
   hack_clist_set_column_title_pixmap(clist, ADDRESS_NOTE_COLUMN, pixmapwid);

   gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		      GTK_SIGNAL_FUNC(cb_clist_selection), NULL);

   gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_ETCHED_OUT);
   gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);

   gtk_clist_set_column_auto_resize(GTK_CLIST(clist), ADDRESS_NAME_COLUMN, TRUE);
   gtk_clist_set_column_auto_resize(GTK_CLIST(clist), ADDRESS_NOTE_COLUMN, TRUE);
   gtk_clist_set_column_auto_resize(GTK_CLIST(clist), ADDRESS_PHONE_COLUMN, FALSE);

   gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(clist));

   /*gtk_clist_set_column_justification(GTK_CLIST(clist), 1, GTK_JUSTIFY_RIGHT); */

   hbox_temp = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox1), hbox_temp, FALSE, FALSE, 0);

   label = gtk_label_new(_("Quick Find: "));
   gtk_box_pack_start(GTK_BOX(hbox_temp), label, FALSE, FALSE, 0);

   address_quickfind_entry = gtk_entry_new();
   gtk_signal_connect(GTK_OBJECT(address_quickfind_entry), "key_press_event",
		      GTK_SIGNAL_FUNC(cb_key_pressed_quickfind), NULL);
   gtk_signal_connect(GTK_OBJECT(address_quickfind_entry), "changed",
		      GTK_SIGNAL_FUNC(cb_address_quickfind),
		      NULL);
   gtk_box_pack_start(GTK_BOX(hbox_temp), address_quickfind_entry, TRUE, TRUE, 0);

   /* The new entry gui */
   hbox_temp = gtk_hbox_new(FALSE, 3);
   gtk_box_pack_start(GTK_BOX(vbox2), hbox_temp, FALSE, FALSE, 0);

   /* Create Cancel button */
   CREATE_BUTTON(cancel_record_button, _("Cancel"), CANCEL, _("Cancel the modifications"), GDK_Escape, 0, "ESC")
   gtk_signal_connect(GTK_OBJECT(cancel_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_cancel), NULL);

   /* Delete Button */
   CREATE_BUTTON(delete_record_button, _("Delete"), DELETE, _("Delete the selected record"), GDK_d, GDK_CONTROL_MASK, "Ctrl+D")
   gtk_signal_connect(GTK_OBJECT(delete_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_delete_address_or_contact),
		      GINT_TO_POINTER(DELETE_FLAG));

   /* Undelete Button */
   CREATE_BUTTON(undelete_record_button, _("Undelete"), UNDELETE, _("Undelete the selected record"), 0, 0, "")
   gtk_signal_connect(GTK_OBJECT(undelete_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_undelete_address),
		      GINT_TO_POINTER(UNDELETE_FLAG));

   /* Create "Copy" button */
   CREATE_BUTTON(copy_record_button, _("Copy"), COPY, _("Copy the selected record"), GDK_o, GDK_CONTROL_MASK, "Ctrl+O")
   gtk_signal_connect(GTK_OBJECT(copy_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_add_new_record),
		      GINT_TO_POINTER(COPY_FLAG));

   /* Create "New" button */
   CREATE_BUTTON(new_record_button, _("New Record"), NEW, _("Add a new record"), GDK_n, GDK_CONTROL_MASK, "Ctrl+N")
   gtk_signal_connect(GTK_OBJECT(new_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_address_clear), NULL);

   /* Create "Add Record" button */
   CREATE_BUTTON(add_record_button, _("Add Record"), ADD, _("Add the new record"), GDK_Return, GDK_CONTROL_MASK, "Ctrl+Enter")
   gtk_signal_connect(GTK_OBJECT(add_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_add_new_record),
		      GINT_TO_POINTER(NEW_FLAG));
#ifndef ENABLE_STOCK_BUTTONS
   gtk_widget_set_name(GTK_WIDGET(GTK_LABEL(GTK_BIN(add_record_button)->child)),
		       "label_high");
#endif

   /* Create "apply changes" button */
   CREATE_BUTTON(apply_record_button, _("Apply Changes"), APPLY, _("Commit the modifications"), GDK_Return, GDK_CONTROL_MASK, "Ctrl+Enter")
   gtk_signal_connect(GTK_OBJECT(apply_record_button), "clicked",
		      GTK_SIGNAL_FUNC(cb_add_new_record),
		      GINT_TO_POINTER(MODIFY_FLAG));
#ifndef ENABLE_STOCK_BUTTONS
   gtk_widget_set_name(GTK_WIDGET(GTK_LABEL(GTK_BIN(apply_record_button)->child)),
		       "label_high");
#endif

   /*Separator */
   separator = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(vbox2), separator, FALSE, FALSE, 5);


   /*Private check box */
   hbox_temp = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox2), hbox_temp, FALSE, FALSE, 0);
   private_checkbox = gtk_check_button_new_with_label(_("Private"));
   gtk_box_pack_end(GTK_BOX(hbox_temp), private_checkbox, FALSE, FALSE, 0);

   changed_list = g_list_prepend(changed_list, private_checkbox);

   /*Add the new category menu */
   make_category_menu(&category_menu2, address_cat_menu_item2,
		      sort_l, NULL, FALSE);

   gtk_box_pack_start(GTK_BOX(hbox_temp), category_menu2, TRUE, TRUE, 0);


   /*Add the notebook for new entries */
   notebook = gtk_notebook_new();
   gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
   gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
   gtk_signal_connect(GTK_OBJECT(notebook), "switch-page",
		      GTK_SIGNAL_FUNC(cb_notebook_changed), NULL);

   gtk_box_pack_start(GTK_BOX(vbox2), notebook, TRUE, TRUE, 0);

   /* Add the notebook pages and their widgets */
   phone_i = address_type_i = IM_type_i = 0;
   for (page_i=0; page_i<num_pages; page_i++) {
      x=y=0;
      for (i=0; i<schema_size; i++) {
	 /* Figure out the table X and Y size */
	 if (schema[i].notebook_page!=page_i) continue;
	 switch (schema[i].type) {
	  case ADDRESS_GUI_LABEL_TEXT:
	    if (x<2) x=2;
	    y++;
	    break;
	  case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	    if (x<4) x=4;
	    y++;
	    break;
	  case ADDRESS_GUI_ADDR_MENU_TEXT:
	    if (x<2) x=2;
	    y++;
	    break;
	  case ADDRESS_GUI_IM_MENU_TEXT:
	    if (x<2) x=2;
	    y++;
	    break;
	  case ADDRESS_GUI_WEBSITE:
	    if (x<2) x=2;
	    y++;
	    break;
	  case ADDRESS_GUI_BIRTHDAY:
	    if (x<2) x=2;
	    y++;
	    break;
	 }	 
      }

      if ((x==0) || (y==0)) {
	 continue;
      }

      /* Add a page */
      table_y_i=0;
      notebook_label[page_i] = gtk_label_new(_(page_names[page_i]));
      vbox_temp = gtk_vbox_new(FALSE, 0);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_temp, notebook_label[page_i]);

      gtk_widget_show(vbox_temp);
      gtk_widget_show(label);

      table = gtk_table_new(y, x, FALSE);

      gtk_box_pack_start(GTK_BOX(vbox_temp), table, TRUE, TRUE, 0);

      //undo remove picture_box
      if ((page_i==0) && (table_y_i==0) && (address_version==1)) {
	 GtkWidget *menu, *menu_item;

	 //picture_box = gtk_hbox_new(FALSE, 0);
	 picture_button = gtk_button_new();
	 gtk_widget_set_usize(GTK_WIDGET(picture_button), PHOTO_X+10, PHOTO_Y+10);
//	 gtk_button_set_relief(GTK_BUTTON(picture_button), GTK_RELIEF_NONE);
	 gtk_container_set_border_width(GTK_CONTAINER(picture_button), 0);
	 gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(picture_button),
			  0, 2, 0, 4, GTK_SHRINK, GTK_SHRINK, 0, 0);
	 //undo define for default picture size x and y
	 //gtk_widget_set_usize(GTK_WIDGET(picture_box), 139, 144);
	 //gtk_box_pack_start(GTK_BOX(picture_box), picture_button, TRUE, TRUE, 0);
//	 gtk_signal_connect(GTK_OBJECT(picture_button), "enter",
//			    GTK_SIGNAL_FUNC(cb_test),
//			    GINT_TO_POINTER(MODIFY_FLAG));

	 /* Create a photo menu */
	 menu = gtk_menu_new();

	 menu_item = gtk_menu_item_new_with_label("Change Photo");
	 gtk_widget_show(menu_item);
	 gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			    G_CALLBACK(cb_photo_menu_select), GINT_TO_POINTER(1));
	 gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 0, 1);
	 menu_item = gtk_menu_item_new_with_label("Remove Photo");
	 gtk_widget_show(menu_item);
	 gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			    G_CALLBACK(cb_photo_menu_select), GINT_TO_POINTER(2));
	 gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 1, 2);

	 g_signal_connect_swapped(picture_button, "button_press_event",
				  G_CALLBACK(cb_photo_menu_popup), menu);

      }
      
      //printf("%s\n", g_find_program_in_path("convert"));
      
      /* Add widgets for each page */
      group=NULL;
      for (i=0; i<schema_size; i++) {

	 if (schema[i].notebook_page!=page_i) continue;
	 switch (schema[i].type) {
	  case ADDRESS_GUI_LABEL_TEXT:
	    /* Label */
	    label = gtk_label_new(contact_app_info.labels[schema[i].record_field]);
	    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	    //gtk_box_pack_start(GTK_BOX(vbox_left), label, TRUE, TRUE, 0);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(label),
			     x-2, x-1, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    /* Text */
#ifdef ENABLE_GTK2
	    address_text[schema[i].record_field] = gtk_text_view_new();
	    gtk_txt_buf_address_text[schema[i].record_field] = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address_text[schema[i].record_field])));
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(address_text[schema[i].record_field]), TRUE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(address_text[schema[i].record_field]), GTK_WRAP_CHAR);
	    gtk_container_set_border_width(GTK_CONTAINER(address_text[schema[i].record_field]), 1);
#else
	    address_text[schema[i].record_field] = gtk_text_new(NULL, NULL);
	    gtk_text_set_editable(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
	    gtk_text_set_word_wrap(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
#endif
	    gtk_widget_set_usize(GTK_WIDGET(address_text[schema[i].record_field]), 0, 25);
	    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(address_text[schema[i].record_field]),
				      x-1, x, table_y_i, table_y_i+1);

#ifdef ENABLE_GTK2
	    changed_list = g_list_prepend(changed_list, gtk_txt_buf_address_text[schema[i].record_field]);
#else
	    changed_list = g_list_prepend(changed_list, address_text[schema[i].record_field]);
#endif
	    break;
	  case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
	    if (!strcmp(contact_app_info.phoneLabels[phone_i], _("E-mail"))) {
	       dial_button[phone_i] = gtk_button_new_with_label(_("Mail"));
	       gtk_object_set_data(GTK_OBJECT(dial_button[phone_i]), "mail", GINT_TO_POINTER(1));
	    } else {
	       dial_button[phone_i] = gtk_button_new_with_label(_("Dial"));
	       gtk_object_set_data(GTK_OBJECT(dial_button[phone_i]), "mail", 0);
	    }
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(dial_button[phone_i]),
			     x-4, x-3, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    
	    radio_button[phone_i] = gtk_radio_button_new_with_label(group, _("Show In List"));
	    group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_button[phone_i]));
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(radio_button[phone_i]),
			     x-3, x-2, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);

	    changed_list = g_list_prepend(changed_list, radio_button[phone_i]);
	    
	    make_phone_menu(phone_i, phone_i, phone_i);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(phone_type_list_menu[phone_i]),
			     x-2, x-1, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    
	    /* Text */
#ifdef ENABLE_GTK2
	    address_text[schema[i].record_field] = gtk_text_view_new();
	    gtk_txt_buf_address_text[schema[i].record_field] = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address_text[schema[i].record_field])));
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(address_text[schema[i].record_field]), TRUE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(address_text[schema[i].record_field]), GTK_WRAP_CHAR);
	    gtk_container_set_border_width(GTK_CONTAINER(address_text[schema[i].record_field]), 1);
#else
	    address_text[schema[i].record_field] = gtk_text_new(NULL, NULL);
	    gtk_text_set_editable(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
	    gtk_text_set_word_wrap(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
#endif
	    gtk_widget_set_usize(GTK_WIDGET(address_text[schema[i].record_field]), 0, 25);
	    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(address_text[schema[i].record_field]),
			     x-1, x, table_y_i, table_y_i+1);
	    
	    gtk_signal_connect(GTK_OBJECT(dial_button[phone_i]), "clicked",
			       GTK_SIGNAL_FUNC(cb_dial_or_mail),
			       address_text[schema[i].record_field]);
#ifdef ENABLE_GTK2
	    changed_list = g_list_prepend(changed_list, gtk_txt_buf_address_text[schema[i].record_field]);
#else
	    changed_list = g_list_prepend(changed_list, address_text[schema[i].record_field]);
#endif
	    phone_i++;
	    break;
	  case ADDRESS_GUI_ADDR_MENU_TEXT:
	    make_address_type_menu(address_type_i, address_type_i);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(address_type_list_menu[address_type_i]),
			     x-2, x-1, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    address_type_i++;
	    
	    /* Text */
#ifdef ENABLE_GTK2
	    address_text[schema[i].record_field] = gtk_text_view_new();
	    gtk_txt_buf_address_text[schema[i].record_field] = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address_text[schema[i].record_field])));
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(address_text[schema[i].record_field]), TRUE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(address_text[schema[i].record_field]), GTK_WRAP_CHAR);
	    gtk_container_set_border_width(GTK_CONTAINER(address_text[schema[i].record_field]), 1);
#else
	    address_text[schema[i].record_field] = gtk_text_new(NULL, NULL);
	    gtk_text_set_editable(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
	    gtk_text_set_word_wrap(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
#endif
	    gtk_widget_set_usize(GTK_WIDGET(address_text[schema[i].record_field]), 0, 25);
	    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(address_text[schema[i].record_field]),
				      x-1, x, table_y_i, table_y_i+1);

#ifdef ENABLE_GTK2
	    changed_list = g_list_prepend(changed_list, gtk_txt_buf_address_text[schema[i].record_field]);
#else
	    changed_list = g_list_prepend(changed_list, address_text[schema[i].record_field]);
#endif
	    break;
	  case ADDRESS_GUI_IM_MENU_TEXT:
	    make_IM_type_menu(IM_type_i, IM_type_i, IM_type_i);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(IM_type_list_menu[IM_type_i]),
			     x-2, x-1, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    IM_type_i++;
	    
	    /* Text */
#ifdef ENABLE_GTK2
	    address_text[schema[i].record_field] = gtk_text_view_new();
	    gtk_txt_buf_address_text[schema[i].record_field] = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address_text[schema[i].record_field])));
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(address_text[schema[i].record_field]), TRUE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(address_text[schema[i].record_field]), GTK_WRAP_CHAR);
	    gtk_container_set_border_width(GTK_CONTAINER(address_text[schema[i].record_field]), 1);
#else
	    address_text[schema[i].record_field] = gtk_text_new(NULL, NULL);
	    gtk_text_set_editable(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
	    gtk_text_set_word_wrap(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
#endif
	    gtk_widget_set_usize(GTK_WIDGET(address_text[schema[i].record_field]), 0, 25);
	    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(address_text[schema[i].record_field]),
				      x-1, x, table_y_i, table_y_i+1);

#ifdef ENABLE_GTK2
	    changed_list = g_list_prepend(changed_list, gtk_txt_buf_address_text[schema[i].record_field]);
#else
	    changed_list = g_list_prepend(changed_list, address_text[schema[i].record_field]);
#endif
	    break;
	  case ADDRESS_GUI_WEBSITE:
	    /* Label */
	    button = gtk_button_new_with_label(contact_app_info.labels[schema[i].record_field]);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(button),
			     x-2, x-1, table_y_i, table_y_i+1, GTK_SHRINK, 0, 0, 0);
	    /* Text */
#ifdef ENABLE_GTK2
	    address_text[schema[i].record_field] = gtk_text_view_new();
	    gtk_txt_buf_address_text[schema[i].record_field] = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address_text[schema[i].record_field])));
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(address_text[schema[i].record_field]), TRUE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(address_text[schema[i].record_field]), GTK_WRAP_CHAR);
	    gtk_container_set_border_width(GTK_CONTAINER(address_text[schema[i].record_field]), 1);
#else
	    address_text[schema[i].record_field] = gtk_text_new(NULL, NULL);
	    gtk_text_set_editable(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
	    gtk_text_set_word_wrap(GTK_TEXT(address_text[schema[i].record_field]), TRUE);
#endif
	    gtk_widget_set_usize(GTK_WIDGET(address_text[schema[i].record_field]), 0, 25);
	    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(address_text[schema[i].record_field]),
				      x-1, x, table_y_i, table_y_i+1);

#ifdef ENABLE_GTK2
	    changed_list = g_list_prepend(changed_list, gtk_txt_buf_address_text[schema[i].record_field]);
#else
	    changed_list = g_list_prepend(changed_list, address_text[schema[i].record_field]);
#endif
	    break;
	  case ADDRESS_GUI_BIRTHDAY:
	    hbox_temp = gtk_hbox_new(FALSE, 0);
	    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(hbox_temp),
			     0, x, table_y_i, table_y_i+1,
			     GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);

	    birthday_checkbox = gtk_check_button_new_with_label(contact_app_info.labels[schema[i].record_field]);
	    gtk_box_pack_start(GTK_BOX(hbox_temp), birthday_checkbox, FALSE, FALSE, 0);
	    gtk_signal_connect(GTK_OBJECT(birthday_checkbox), "clicked",
			       GTK_SIGNAL_FUNC(cb_check_button_birthday), NULL);

	    changed_list = g_list_prepend(changed_list, birthday_checkbox);

	    birthday_box = gtk_hbox_new(FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(hbox_temp), birthday_box, FALSE, FALSE, 0);

	    birthday_button = gtk_button_new_with_label("");
	    gtk_box_pack_start(GTK_BOX(birthday_box), birthday_button, FALSE, FALSE, 0);
	    gtk_signal_connect(GTK_OBJECT(birthday_button), "clicked",
			       GTK_SIGNAL_FUNC(cb_button_birthday), NULL);

	    changed_list = g_list_prepend(changed_list, birthday_button);

	    reminder_checkbox = gtk_check_button_new_with_label(_("Reminder"));
	    gtk_box_pack_start(GTK_BOX(birthday_box), reminder_checkbox, FALSE, FALSE, 0);
	    gtk_signal_connect(GTK_OBJECT(reminder_checkbox), "clicked",
			       GTK_SIGNAL_FUNC(cb_check_button_reminder), NULL);

	    changed_list = g_list_prepend(changed_list, reminder_checkbox);

	    reminder_box = gtk_hbox_new(FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(hbox_temp), reminder_box, FALSE, FALSE, 0);

	    reminder_entry = gtk_entry_new_with_max_length(2);
	    gtk_widget_set_usize(reminder_entry, 30, 0);
	    gtk_box_pack_start(GTK_BOX(reminder_box), reminder_entry, FALSE, FALSE, 0);

	    changed_list = g_list_prepend(changed_list, reminder_entry);

	    label = gtk_label_new(_("Days"));
	    gtk_box_pack_start(GTK_BOX(reminder_box), label, FALSE, FALSE, 0);

	    break;
	 };
	 table_y_i++;
      }
   }
   
   /* Connect keypress signals to callbacks */

   /* Capture the Enter key to move to the left-hand side of the display */
   gtk_signal_connect(GTK_OBJECT(clist), "key_press_event",
		      GTK_SIGNAL_FUNC(cb_key_pressed_left_side),
		      NULL);

   for (i=0; i<schema_size; i++) {
      switch (schema[i].type) {
       case ADDRESS_GUI_LABEL_TEXT:
       case ADDRESS_GUI_DIAL_SHOW_MENU_TEXT:
       case ADDRESS_GUI_ADDR_MENU_TEXT:
       case ADDRESS_GUI_IM_MENU_TEXT:
       case ADDRESS_GUI_WEBSITE:
	 /* Capture the Shift-Enter key combination to move back to 
	  * the right-hand side of the display. */
	 gtk_signal_connect(GTK_OBJECT(address_text[schema[i].record_field]), "key_press_event",
			    GTK_SIGNAL_FUNC(cb_key_pressed_right_side), clist);

	 gtk_signal_connect(GTK_OBJECT(address_text[schema[i].record_field]), "key_press_event",
			    GTK_SIGNAL_FUNC(cb_key_pressed), 0);
	 break;
      }
   }

   /* All page */
   notebook_tab = gtk_label_new(_("All"));
   vbox_temp = gtk_vbox_new(FALSE, 0);
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_temp, notebook_tab);
   /* Notebook tabs have to be shown before the show_all */
   gtk_widget_show(vbox_temp);
   gtk_widget_show(notebook_tab);

   /*The Quickview (ALL) page */
   hbox_temp = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox_temp), hbox_temp, TRUE, TRUE, 0);

#ifdef ENABLE_GTK2
   text = gtk_text_view_new();
   gtk_txt_buf_text = G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)));
   gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
   gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);

   scrolled_window = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
   gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 1);
   gtk_container_add(GTK_CONTAINER(scrolled_window), text);
   gtk_box_pack_start(GTK_BOX(hbox_temp), scrolled_window, TRUE, TRUE, 0);
#else
   text = gtk_text_new(NULL, NULL);
   gtk_text_set_editable(GTK_TEXT(text), FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);
   vscrollbar = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
   gtk_box_pack_start(GTK_BOX(hbox_temp), text, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(hbox_temp), vscrollbar, FALSE, FALSE, 0);
#endif

   gtk_widget_show_all(vbox);
   gtk_widget_show_all(hbox);

   gtk_widget_hide(add_record_button);
   gtk_widget_hide(apply_record_button);
   gtk_widget_hide(undelete_record_button);
   gtk_widget_hide(cancel_record_button);
   if (address_version) {
      if (GTK_IS_WIDGET(reminder_box)) {
	 gtk_widget_hide(reminder_box);
      }
      if (GTK_IS_WIDGET(birthday_box)) {
	 gtk_widget_hide(birthday_box);
      }
   }

   get_pref(PREF_ADDRESS_NOTEBOOK_PAGE, &notebook_page, NULL);

   if ((notebook_page<6) && (notebook_page>-1)) {
      gtk_notebook_set_page(GTK_NOTEBOOK(notebook), notebook_page);
   }

   address_refresh();

   return EXIT_SUCCESS;
}
