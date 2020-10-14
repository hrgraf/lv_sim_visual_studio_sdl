// ------------------------------------------------------------------------
// My TTGO Watch GUI
// ------------------------------------------------------------------------

#include "my_watch.h"
#include "lvgl/lvgl.h"
#include "math.h"

// display size
#define WIDTH  240
#define HEIGHT 240

// logging
//#define MY_LOG(...)
//#define MY_LOG LV_LOG_USER
#ifdef _WIN32
#include "stdio.h"
#define MY_LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#else
#include "HardwareSerial.h"
#define MY_LOG(...) { Serial.printf(__VA_ARGS__); Serial.println(""); }
#endif


// ------------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------------

extern "C" LV_IMG_DECLARE(silver_number);
extern "C" LV_IMG_DECLARE(white_face);
extern "C" LV_IMG_DECLARE(mickey);
extern "C" LV_IMG_DECLARE(hand_hour);
extern "C" LV_IMG_DECLARE(hand_min);
extern "C" LV_IMG_DECLARE(hand_sec);

static const char * day_names[7] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };
static const char * month_names[12] = {
	"Januar", "Februar", "Marz",
	"April", "Mai", "Juni",
	"Juli", "August", "September",
	"Oktober", "November", "Dezember"
};
static const char * weekday_names[7] = { 
	"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" 
};

static const char * options_0_23 =
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";

static const char * options_0_59 =
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n"
	"10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
	"20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n"
	"30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n"
	"40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n"
	"50\n51\n52\n53\n54\n55\n56\n57\n58\n59";

enum {
	HOUR_NONE = 100,
	MIN_NONE  = 100,
	SEC_NONE  = 100
};

#define MAX_TILES 10
static lv_coord_t num_tiles;
static lv_point_t tile_pos[MAX_TILES];

static lv_obj_t * home;
static lv_obj_t * app;

// ------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------

inline uint32_t diff_time(uint32_t curr_ms, uint32_t start_ms)
{
	uint32_t elapsed;
	if (curr_ms >= start_ms)
	{
		elapsed = curr_ms - start_ms;
	}
	else
	{
		elapsed = UINT32_MAX - start_ms + 1;
		elapsed += curr_ms;
	}
	return elapsed;
}

// ------------------------------------------------------------------------
// List Demo
// ------------------------------------------------------------------------

static void list_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if (event == LV_EVENT_CLICKED) {
		MY_LOG("List button %s clicked", lv_list_get_btn_text(obj));

		MY_LOG("Switching back to home screen");
		lv_scr_load(home);

		if (app) {
			MY_LOG("Killing app");
			lv_obj_del(app);
			app = NULL;
		}
	}
}

static void demo_list(lv_obj_t *parent)
{
	// Create a list
	lv_obj_t *list = lv_list_create(parent, NULL);
	lv_obj_set_size(list, WIDTH-20, HEIGHT-20);
	//lv_obj_set_size(list, lv_obj_get_width(parent), lv_obj_get_height(parent));
	//lv_page_set_scrollable_fit2(list, LV_FIT_PARENT, LV_FIT_PARENT);
//	lv_cont_set_fit2(list, LV_FIT_PARENT, LV_FIT_PARENT);
	MY_LOG("Demo list %dx%d", lv_obj_get_width(list), lv_obj_get_height(list));
	lv_obj_align(list, NULL, LV_ALIGN_CENTER, 0, 0);
	//lv_list_set_edge_flash(list, true);

	// Add buttons to the list
	lv_obj_t *btn;

	btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Zeit");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_BELL, "Wecker");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_AUDIO, "Ton");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_IMAGE, "Anzeige");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WLAN");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_BATTERY_FULL, "Batterie");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_SD_CARD, "Speicherkarte");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_CALL, "Telefon");
	lv_obj_set_event_cb(btn, list_btn_cb);

	btn = lv_list_add_btn(list, LV_SYMBOL_GPS, "GPS");
	lv_obj_set_event_cb(btn, list_btn_cb);
}

// ------------------------------------------------------------------------
// Base Tile
// ------------------------------------------------------------------------

class BaseTile
{
public:
	//BaseTile() : tile(NULL), name("???") {}

	void create(lv_obj_t * parent, const char *name)
	{
		this->name = name;

		if (num_tiles >= MAX_TILES) {
			MY_LOG("No more tiles available");
			return;
		}

		if (! num_tiles) // init
		{
			lv_style_init(&style_time);
			lv_style_set_text_font(&style_time, LV_STATE_DEFAULT, &lv_font_montserrat_48);
		}

#if 1
		// container
		tile = lv_cont_create(parent, NULL);
		//lv_obj_reset_style_list(cont, LV_CONT_PART_MAIN); // remove border
		lv_obj_set_style_local_border_side(tile, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_obj_set_size(tile, WIDTH, HEIGHT);
		lv_cont_set_layout(tile, LV_LAYOUT_CENTER);
		//lv_obj_set_drag_parent(tile, true);
#else
		tile = lv_obj_create(parent, NULL);
		lv_obj_set_style_local_border_side(tile, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_obj_set_size(tile, WIDTH-20, HEIGHT-20);
#endif
		lv_obj_set_pos(tile, num_tiles*WIDTH, 0);
		lv_tileview_add_element(parent, tile);

		populate();

		tile_pos[num_tiles].x = num_tiles;
		tile_pos[num_tiles].y = 0;
		num_tiles++;
	}

	virtual void populate()
	{
		// dummy implementation
		populate_button(name);
	}

	// --------------------------------------------------------------------
	// label
	// --------------------------------------------------------------------

	void populate_label(const char *text)
	{
		// label
		lv_obj_t * label = lv_label_create(get_parent(), NULL);
		lv_label_set_static_text(label, text);
	}

	// --------------------------------------------------------------------
	// button
	// --------------------------------------------------------------------

	void populate_button(const char *text)
	{
		// button
		btn = lv_btn_create(get_parent(), NULL);
		//lv_obj_align(btn, NULL, LV_ALIGN_CENTER, 0, 0);
		//lv_obj_set_drag_parent(btn, true);
		lv_page_glue_obj(btn, true);
		lv_obj_set_user_data(btn, this);
		lv_obj_set_event_cb(btn, btn_cb);

		// button label
		lv_obj_t * label = lv_label_create(btn, NULL);
		lv_label_set_static_text(label, text);
	}

	void set_button_text(const char *text)
	{
		if (btn) {
			lv_obj_t * label = lv_obj_get_child(btn, NULL);
			if (label)
				lv_label_set_static_text(label, text);
		}
	}

	static void btn_cb(lv_obj_t * obj, lv_event_t event)
	{
		static uint16_t ignore_click = 0;
		BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);

		switch (event)
		{
		case LV_EVENT_PRESSED:
			ignore_click = 0; // reset at new touch
			if (inst)
				inst->button_start_press_cb();
			break;

		case LV_EVENT_LONG_PRESSED:
			ignore_click = 1; // avoid immediate restart on clicked
			if (inst)
				inst->button_long_press_cb();
			break;

		case LV_EVENT_CLICKED:
			if (inst && (!ignore_click))
				inst->button_clicked_cb();
			break;

		case LV_EVENT_PRESS_LOST:
		case LV_EVENT_RELEASED:
			if (inst)
				inst->button_stop_press_cb();
		}
	}

	virtual void button_start_press_cb() {}
	virtual void button_long_press_cb() {}
	virtual void button_stop_press_cb() {}

	virtual void button_clicked_cb() 
	{
		// dummy implementation
		if (app) {
			MY_LOG("Switching to existing app");
		}
		else {
			MY_LOG("Creating new app");
			app = lv_obj_create(NULL, NULL);
			demo_list(app);
		}
		lv_scr_load(app);
	}

	// --------------------------------------------------------------------
	// hour min sec
	// --------------------------------------------------------------------

	void populate_hour_min_sec(uint16_t hour, uint16_t min, uint16_t sec)
	{
		// container
		lv_obj_t * cont = lv_cont_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_cont_set_layout(cont, LV_LAYOUT_ROW_MID);
		lv_cont_set_fit(cont, LV_FIT_TIGHT);
		lv_page_glue_obj(cont, true);
		//lv_obj_set_drag_parent(cont, true);

		if (hour != HOUR_NONE)
		{
			// roller
			lv_obj_t * rol = lv_roller_create(cont, NULL);
			lv_roller_set_options(rol, options_0_23, LV_ROLLER_MODE_NORMAL);
			lv_roller_set_visible_row_count(rol, 3);
			lv_roller_set_selected(rol, hour, LV_ANIM_OFF);
			lv_obj_set_user_data(rol, this);
			lv_obj_set_event_cb(rol, rol_hour_cb);
			//lv_page_glue_obj(rol, true);
			//lv_obj_set_drag_parent(rol, true);
			rol_hour = rol;
		}

		if (min != MIN_NONE)
		{
			if (hour != HOUR_NONE)
			{
				// label
				lv_obj_t * label = lv_label_create(cont, NULL);
				lv_label_set_static_text(label, ":");
			}

			// roller
			lv_obj_t * rol = lv_roller_create(cont, NULL);
			lv_roller_set_options(rol, options_0_59, LV_ROLLER_MODE_NORMAL);
			lv_roller_set_visible_row_count(rol, 3);
			lv_roller_set_selected(rol, min, LV_ANIM_OFF);
			lv_obj_set_user_data(rol, this);
			lv_obj_set_event_cb(rol, rol_min_cb);
			//lv_page_glue_obj(rol, true);
			//lv_obj_set_drag_parent(rol, true);
			rol_min = rol;
		}

		if (sec != SEC_NONE)
		{
			if (min != MIN_NONE)
			{
				// label
				lv_obj_t * label = lv_label_create(cont, NULL);
				lv_label_set_static_text(label, ":");
			}

			// roller
			lv_obj_t * rol = lv_roller_create(cont, NULL);
//			lv_roller_set_options(rol, options_0_59, LV_ROLLER_MODE_INIFINITE); // shows strange animation
			lv_roller_set_options(rol, options_0_59, LV_ROLLER_MODE_NORMAL);
			lv_roller_set_visible_row_count(rol, 3);
			lv_roller_set_selected(rol, sec, LV_ANIM_OFF);
			lv_obj_set_user_data(rol, this);
			lv_obj_set_event_cb(rol, rol_sec_cb);
			//lv_page_glue_obj(rol, true);
			//lv_obj_set_drag_parent(rol, true);
			rol_sec = rol;
		}
	}

	void populate_hour_min(uint16_t hour, uint16_t min)
	{
		populate_hour_min_sec(hour, min, SEC_NONE);
	}

	void set_hour(uint16_t hour)
	{
		if (rol_hour)
			lv_roller_set_selected(rol_hour, hour, LV_ANIM_ON);
	}

	void set_min(uint16_t min)
	{
		if (rol_min)
			lv_roller_set_selected(rol_min, min, LV_ANIM_ON);
	}

	void set_sec(uint16_t sec)
	{
		if (rol_sec)
			lv_roller_set_selected(rol_sec, sec, LV_ANIM_ON);
	}

	static void rol_hour_cb(lv_obj_t * obj, lv_event_t event)
	{
		if (event == LV_EVENT_VALUE_CHANGED)
		{
			uint16_t pos = lv_roller_get_selected(obj);
			MY_LOG("Selected hour %d", pos);
			BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);
			if (inst)
				inst->hour_changed_cb(pos);
		}
	}

	static void rol_min_cb(lv_obj_t * obj, lv_event_t event)
	{
		if (event == LV_EVENT_VALUE_CHANGED)
		{
			uint16_t pos = lv_roller_get_selected(obj);
			MY_LOG("Selected min %d", pos);
			BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);
			if (inst)
				inst->min_changed_cb(pos);
		}
	}

	static void rol_sec_cb(lv_obj_t * obj, lv_event_t event)
	{
		if (event == LV_EVENT_VALUE_CHANGED)
		{
			uint16_t pos = lv_roller_get_selected(obj);
			MY_LOG("Selected sec %d", pos);
			BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);
			if (inst)
				inst->sec_changed_cb(pos);
		}
	}

	virtual void hour_changed_cb(uint16_t hour) {}
	virtual void min_changed_cb( uint16_t min ) {}
	virtual void sec_changed_cb( uint16_t sec ) {}

	// --------------------------------------------------------------------
	// switch
	// --------------------------------------------------------------------

	void populate_switch(uint16_t on, const char *text)
	{
		// container
		lv_obj_t * cont = lv_cont_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_cont_set_layout(cont, LV_LAYOUT_ROW_MID);
		lv_cont_set_fit(cont, LV_FIT_TIGHT);
		lv_page_glue_obj(cont, true);

		// switch
		lv_obj_t * sw = lv_switch_create(cont, NULL);
		if (on)
			lv_switch_on(sw, LV_ANIM_OFF);
		lv_obj_set_user_data(sw, this);
		lv_obj_set_event_cb(sw, sw_cb);

		// label
		lv_obj_t * label = lv_label_create(cont, NULL);
		lv_label_set_static_text(label, text);
	}

	static void sw_cb(lv_obj_t * obj, lv_event_t event)
	{
		if (event == LV_EVENT_VALUE_CHANGED)
		{
			uint16_t pos = lv_switch_get_state(obj);
			MY_LOG("Switched %d", pos);
			BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);
			if (inst)
				inst->switched_cb(pos);
		}
	}

	virtual void switched_cb(uint16_t on) {}

	// --------------------------------------------------------------------
	// arc
	// --------------------------------------------------------------------

	void populate_arc()
	{
		arc = lv_arc_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(arc, LV_CALENDAR_PART_BG, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_obj_set_size(arc, WIDTH * 2 / 3, HEIGHT * 2 / 3);
//		lv_obj_set_size(arc, WIDTH-40, HEIGHT-40);
		//lv_arc_set_rotation(arc, -90); // makes 0� at 12 o'clock, but does not work
		lv_arc_set_bg_angles(arc, 0, 360);
		lv_arc_set_angles(arc, 270, 270);
		lv_page_glue_obj(arc, true);
		lv_obj_set_user_data(arc, this);
		lv_obj_set_event_cb(arc, arc_cb);

		// label
		lv_obj_t * label = lv_label_create(arc, NULL);
		lv_label_set_static_text(label, "ARC");
		lv_obj_align(label, arc, LV_ALIGN_CENTER, 0, 0);
	}

	void set_arc(uint16_t angle)
	{
		if (arc)
			lv_arc_set_end_angle(arc, angle+270);
	}

	void set_arc_text(const char *text)
	{
		if (arc) {
			lv_obj_t * label = lv_obj_get_child(arc, NULL);
			if (label)
				lv_label_set_static_text(label, text);
		}
	}

	static void arc_cb(lv_obj_t * obj, lv_event_t event)
	{
		static uint16_t ignore_click = 0;
		//int16_t pos = 0;
		BaseTile *inst = (BaseTile *)lv_obj_get_user_data(obj);
		//MY_LOG("Arc event %d", event);

		switch (event)
		{
		case LV_EVENT_PRESSED:
			ignore_click = 0; // reset at new touch
			if (inst)
				inst->arc_start_press_cb();
			break;

		case LV_EVENT_LONG_PRESSED:
			ignore_click = 1; // avoid immediate restart on clicked
			if (inst)
				inst->arc_long_press_cb();
			break;

		case LV_EVENT_CLICKED:
			if (inst && (!ignore_click))
				inst->arc_clicked_cb();
			break;

//		case LV_EVENT_VALUE_CHANGED: // never happens??
//			pos = lv_arc_get_value(obj);
//			MY_LOG("Arc changed to %d", pos);
//			if (inst)
//				inst->arc_changed_cb(pos);
//			break;
		}
	}

	virtual void arc_start_press_cb() {}
	virtual void arc_long_press_cb() {}
	virtual void arc_clicked_cb() {}
	virtual void arc_changed_cb(int16_t pos) {}

	// --------------------------------------------------------------------

	inline lv_obj_t * get_parent() { return tile;  }

protected:
	const char *name;
	static lv_style_t style_time;

private:
	// GUI
	lv_obj_t *tile;
	lv_obj_t *rol_hour, *rol_min, *rol_sec;
	lv_obj_t *arc;
	lv_obj_t *btn;
};

lv_style_t BaseTile::style_time;


// ------------------------------------------------------------------------
// Calendar Tile
// ------------------------------------------------------------------------

class CalendarTile : public BaseTile
{
public:
	virtual void populate()
	{
		//static lv_style_t style1;
		//lv_style_set_color(&style1, LV_STATE_DISABLED, LV_COLOR_GRAY);

		cal = lv_calendar_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(cal, LV_CALENDAR_PART_BG, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_obj_set_style_local_text_color(cal, LV_CALENDAR_PART_DATE, LV_STATE_DISABLED, LV_COLOR_MAKE(0x40, 0x40, 0x40));
		lv_calendar_set_day_names(cal, day_names);
		lv_calendar_set_month_names(cal, month_names);
		lv_obj_set_size(cal, WIDTH, HEIGHT);
		lv_obj_set_user_data(cal, this);
		lv_obj_set_event_cb(cal, cal_cb);
		lv_page_glue_obj(cal, true);

		// Make the date number smaller to be sure they fit into their area
		//lv_obj_set_style_local_text_font(cal, LV_CALENDAR_PART_DATE, LV_STATE_DEFAULT, lv_theme_get_font_small());

		// Set date
		//update(2020, 9, 22);
	}

	void update(uint16_t year, uint16_t month, uint16_t day)
	{
		// Set today's date
		lv_calendar_date_t today;
		today.year = year;
		today.month = (int8_t)month;
		today.day = (int8_t)day;

		lv_calendar_set_today_date(cal, &today);
		lv_calendar_set_showed_date(cal, &today);
	}

	static void cal_cb(lv_obj_t * obj, lv_event_t event)
	{
		static uint16_t year = 0, month = 0, day = 0;

		if (event == LV_EVENT_VALUE_CHANGED)
		{
			lv_calendar_date_t * date = lv_calendar_get_pressed_date(obj);
			if (date)
			{
				year  = date->year;
				month = date->month;
				day   = date->day;
			}
			else
			{
				year = month = day = 0;
			}
		}

		if (event == LV_EVENT_RELEASED)
		{
			year = month = day = 0;
		}

		if (event == LV_EVENT_CLICKED)
		{
			if (year)
			{
				MY_LOG("Clicked %02d.%02d.%d", day, month, year);
			}
			else
			{
				MY_LOG("Clicked no date");
			}
		}
	}

private:
	// GUI
	lv_obj_t *cal;
};

// ------------------------------------------------------------------------
// Alarm Tile
// ------------------------------------------------------------------------

class AlarmTile : public BaseTile
{
public:
	AlarmTile() : alarm_hour(7), alarm_min(15), alarm_on(0) {}

	virtual void populate()
	{
		populate_label(name);
		populate_hour_min(alarm_hour, alarm_min);
		populate_switch(alarm_on, "Alarm");
	}

	virtual void hour_changed_cb(uint16_t hour)
	{
		alarm_hour = hour;
	}

	virtual void min_changed_cb(uint16_t min) 
	{
		alarm_min = min;
	}

	virtual void switched_cb(uint16_t on)
	{
		alarm_on = on;
	}

	void update(uint16_t weekday, uint16_t hour, uint16_t min)
	{
		if (alarm_on && (hour == alarm_hour) && (min == alarm_min))
		{
			MY_LOG("ALARM!!!");
		}
	}

private:
	uint16_t alarm_hour, alarm_min, alarm_on;
};

// ------------------------------------------------------------------------
// Stopwatch Tile
// ------------------------------------------------------------------------

class StopwatchTile : public BaseTile
{
public:
	StopwatchTile() : start_ms(0) {}

	virtual void populate()
	{
		populate_label(name);

		// time
		lv_obj_t * label = lv_label_create(get_parent(), NULL);
		lv_obj_add_style(label, LV_OBJ_PART_MAIN, &style_time);
		label_time = label;
		redraw(0);

		populate_button("");
		update_button();
	}

	void redraw(uint32_t elapsed) // ms
	{
		elapsed /= 10; // hund
		uint32_t hund = elapsed % 100;
		elapsed /= 100; // sec
		uint32_t sec = elapsed % 60;
		elapsed /= 60; // min
		lv_label_set_text_fmt(label_time, "%d:%02d.%02d", elapsed, sec, hund);
	}

	virtual void button_start_press_cb()
	{
		set_high_speed();
	}

	virtual void button_long_press_cb() 
	{
		// stop and reset
		start_ms = 0;
		set_normal_speed();
		redraw(0);
		update_button();
	}

	virtual void button_stop_press_cb() 
	{
		if (!start_ms)
			set_normal_speed();

	}

	virtual void button_clicked_cb()
	{
		if (start_ms) // running -> stop
		{
			uint32_t curr_ms = lv_tick_get();
			uint32_t elapsed = diff_time(curr_ms, start_ms);
			start_ms = 0; // off
			set_normal_speed();
			MY_LOG("Stopped after %d ms", elapsed);
		}
		else // start
		{
			start_ms = lv_tick_get();
			set_high_speed();
		}

		update_button();
	}

	void update_button()
	{
		set_button_text(is_running() ? "stopp" : "start");
	}

	inline bool is_running() { return (start_ms != 0); }

	void update_ms(uint32_t curr_ms)
	{
		if (start_ms)
		{
			uint32_t elapsed = diff_time(curr_ms, start_ms);
			redraw(elapsed);
		}
	}

private:
	uint32_t start_ms;

	// GUI
	lv_obj_t *label_time;
};


// ------------------------------------------------------------------------
// Metronome Tile
// ------------------------------------------------------------------------

class MetronomeTile : public BaseTile
{
public:
	MetronomeTile() : start_ms(0), delta_ms(0),
		min_rpm(60), max_rpm(180), def_rpm(120), 
		def_beat(4), curr_beat(0)
	{
		num_beat = def_beat;
	}

	#define NUM_METRO_LED 5

	virtual void populate()
	{
		populate_label(name);

		// LED container
		lv_obj_t * cont = lv_cont_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_cont_set_layout(cont, LV_LAYOUT_ROW_MID);
		lv_cont_set_fit(cont, LV_FIT_TIGHT);
		lv_page_glue_obj(cont, true);

		// LEDs
		for (int i = 0; i < NUM_METRO_LED; i++)
		{
			led[i] = lv_led_create(cont, NULL);
			lv_obj_set_size(led[i], 20, 20);
			lv_led_off(led[i]);
			lv_obj_set_drag_parent(led[i], true);
			//lv_page_glue_obj(led[i], true);
		}

		// button matrix
		static const char * btnm_map[NUM_METRO_LED+1] = { "off", "2", "3", "4", "5", "6" };
		btnm_map[NUM_METRO_LED] = "";

		btnm = lv_btnmatrix_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(btnm, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_obj_set_width_fit(btnm, WIDTH);
		lv_obj_set_height_fit(btnm, HEIGHT / 4);
		lv_page_glue_obj(btnm, true);
		lv_btnmatrix_set_map(btnm, btnm_map);
		for (int i = 0; i < NUM_METRO_LED; i++)
			lv_btnmatrix_set_btn_ctrl(btnm, i, 
				LV_BTNMATRIX_CTRL_NO_REPEAT | LV_BTNMATRIX_CTRL_CHECKABLE | (i==0 ? LV_BTNMATRIX_CTRL_CHECK_STATE : 0));
		lv_btnmatrix_set_btn_width(btnm, 0, 2);
		lv_btnmatrix_set_one_check(btnm, true);
		lv_obj_set_user_data(btnm, this);
		lv_obj_set_event_cb(btnm, btnm_cb);

		// slider container
		cont = lv_cont_create(get_parent(), NULL);
		lv_obj_set_style_local_border_side(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
		lv_cont_set_layout(cont, LV_LAYOUT_OFF);
		lv_cont_set_fit(cont, LV_FIT_TIGHT);
		lv_page_glue_obj(cont, true);

		// slider
		slider = lv_slider_create(cont, NULL);
		lv_obj_set_width(slider, WIDTH*2/3);
		lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_slider_set_range(slider, min_rpm, max_rpm);
		lv_slider_set_value(slider, def_rpm, LV_ANIM_OFF);
		lv_obj_set_user_data(slider, this);
		lv_obj_set_event_cb(slider, slider_cb);

		// slider label
#if 1
		slider_label = lv_label_create(cont, NULL);
		lv_label_set_text(slider_label, "");
		lv_obj_set_auto_realign(slider_label, true);
		lv_obj_align(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
#else
		// built-in
		lv_obj_set_style_local_margin_bottom(slider, LV_SLIDER_PART_BG, LV_STATE_DEFAULT, 25);
		lv_obj_set_style_local_value_ofs_y(slider, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT, 25);

#endif
		speed_changed_cb(def_rpm);
		redraw();
	}

	void redraw()
	{
		for (uint16_t i = 1; i <= NUM_METRO_LED; i++)
		{
			if (i == curr_beat)
				lv_led_on(led[i-1]);
			else
				lv_led_off(led[i-1]);
		}
	}

	static void btnm_cb(lv_obj_t * obj, lv_event_t event)
	{
		MetronomeTile *inst = (MetronomeTile *)lv_obj_get_user_data(obj);

		switch (event)
		{
		case LV_EVENT_LONG_PRESSED:
			if (inst)
				inst->reset_speed();
			break;

		case LV_EVENT_CLICKED:
			int16_t beats = lv_btnmatrix_get_active_btn(obj);
			if (beats == LV_BTNMATRIX_BTN_NONE)
				return;
			if (beats > 0)
				beats++;
			MY_LOG("Selected %d beats", beats);
			if (inst)
				inst->beats_changed_cb(beats);
		}
	}

	void beats_changed_cb(int16_t beats)
	{
		num_beat = beats;
		if (beats == 0) // running -> stop
		{
			start_ms = 0; // off
			curr_beat = 0;
		}
		else // start
		{
			start_ms = lv_tick_get();
			curr_beat = 1;
		}

		redraw();
	}

	static void slider_cb(lv_obj_t * obj, lv_event_t event)
	{
		if (event == LV_EVENT_VALUE_CHANGED)
		{
			int16_t rpm = lv_slider_get_value(obj);
			MY_LOG("Selected %d rpm", rpm);
			MetronomeTile *inst = (MetronomeTile *)lv_obj_get_user_data(obj);
			if (inst)
				inst->speed_changed_cb(rpm);
		}
	}

	void speed_changed_cb(int16_t rpm)
	{
		// speed limit
		if (rpm < min_rpm)
			rpm = min_rpm;
		if (rpm > max_rpm)
			rpm = max_rpm;

		delta_ms = 60 * 1000 / rpm;

		static char buf[8];
		lv_snprintf(buf, sizeof(buf), "%d rpm", rpm);
		if (slider_label)
			lv_label_set_static_text(slider_label, buf);
		else
			lv_obj_set_style_local_value_str(slider, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT, buf);
	}

	void reset_speed()
	{
		// reset speed
		MY_LOG("reset speed");
		lv_slider_set_value(slider, def_rpm, LV_ANIM_ON);
		speed_changed_cb(def_rpm);
	}

	inline bool is_running() { return (start_ms != 0); }

	void update_ms(uint32_t curr_ms)
	{
		if (start_ms)
		{
			uint32_t elapsed = diff_time(curr_ms, start_ms);
			if (elapsed > delta_ms)
			{
				start_ms += delta_ms;

				if (curr_beat == num_beat)
					curr_beat = 1;
				else
					curr_beat++;

				redraw();
			}
		}
	}

private:
	uint32_t start_ms, delta_ms;
	int16_t min_rpm, max_rpm, def_rpm;
	int16_t num_beat, def_beat, curr_beat;

	// GUI
	lv_obj_t *led[NUM_METRO_LED];
	lv_obj_t *btnm;
	lv_obj_t *slider, *slider_label;
};


// ------------------------------------------------------------------------
// Countdown Tile
// ------------------------------------------------------------------------

class CountdownTile : public BaseTile
{
public:
	virtual void populate()
	{
		populate_label(name);
		populate_hour_min_sec(0, 0, 0);
		populate_button("");
		reset();
	}

	void reset()
	{
		// stop and reset
		cnt_on = 0;
		cnt_hour = 0;
		cnt_min = 3; // cooking eggs
		cnt_sec = 0;
		set_sec(cnt_sec);
		set_min(cnt_min);
		set_hour(cnt_hour);
		update_button();
	}

	virtual void hour_changed_cb(uint16_t hour)
	{
		cnt_hour = hour;
	}

	virtual void min_changed_cb(uint16_t min)
	{
		cnt_min = min;
	}

	virtual void sec_changed_cb(uint16_t sec)
	{
		cnt_sec = sec;
	}

	virtual void button_long_press_cb()
	{
		reset();
	}

	virtual void button_clicked_cb()
	{
		cnt_on = !cnt_on;
		MY_LOG("Countdown %02d:%02d:%02d is %d", cnt_hour, cnt_min, cnt_sec, cnt_on);
		update_button();
	}

	void update_button()
	{
		set_button_text(cnt_on ? "stopp" : "start");
	}

	void update_sec(bool need_draw)
	{
		if (!cnt_on)
			return;

		if (cnt_sec)
		{
			cnt_sec--;

			set_sec(cnt_sec);
		}
		else
		{
			if (cnt_min)
			{
				cnt_sec = 59;
				cnt_min--;

				set_sec(cnt_sec);
				set_min(cnt_min);
			}
			else
			{
				if (cnt_hour)
				{
					cnt_sec = 59;
					cnt_min = 59;
					cnt_hour--;

					set_sec(cnt_sec);
					set_min(cnt_min);
					set_hour(cnt_hour);
				}
				else
				{
					MY_LOG("COUNTDOWN DONE!!");
					cnt_on = 0;
					update_button();
				}
			}
		}
	}

private:
	uint16_t cnt_hour, cnt_min, cnt_sec, cnt_on;
};

// ------------------------------------------------------------------------
// Toothbrushing Tile
// ------------------------------------------------------------------------

class ToothbrushingTile : public BaseTile
{
public:
	ToothbrushingTile() : num_sec(4*30), cnt_sec(0), cnt_on(0) {}

	virtual void populate()
	{
		populate_label(name);
		populate_arc();
		update_angle();
		update_text();
	}

	virtual void arc_clicked_cb()
	{
		// toggle
		cnt_on = !cnt_on;
		update_angle();
		if (cnt_on && (cnt_sec == 0))
		{
			MY_LOG("TOOTHBRUSH START!!");
		}
		update_text();
	}

	virtual void arc_long_press_cb()
	{
		// reset
		cnt_on  = 0;
		cnt_sec = 0;
		update_angle();
		update_text();
	}

	void update_angle()
	{
		if (num_sec)
			set_arc(360 * cnt_sec / num_sec);
	}

	void update_text()
	{
		MY_LOG("Arc at %d is %d", cnt_sec, cnt_on);
		set_arc_text(cnt_on ? "stopp" : "start");
	}

	void update_sec(bool need_draw)
	{
		if (! cnt_on)
			return;

		if (cnt_sec < num_sec)
		{
			cnt_sec++;
			update_angle();

		}

		if (cnt_sec < num_sec)
		{
			uint16_t quarter = num_sec / 4;
			if ((cnt_sec == quarter) || (cnt_sec == 2*quarter) || (cnt_sec == 3*quarter))
			{
				MY_LOG("TOOTHBRUSH CHANGE!!");
			}
		}
		else
		{
			MY_LOG("TOOTHBRUSH DONE!!");
			cnt_on = 0;
			update_text();
			cnt_sec = 0;
		}
	}

private:
	uint16_t num_sec, cnt_sec, cnt_on;
};

// ------------------------------------------------------------------------
// Lamp Tile
// ------------------------------------------------------------------------

class LampTile : public BaseTile
{
public:
	LampTile() {}

	virtual void populate()
	{
		// label
		lv_obj_t * label = lv_label_create(get_parent(), NULL);
		lv_obj_set_style_local_text_color(get_parent(), LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
		lv_label_set_static_text(label, name);

		// color picker
		lv_color_t col = LV_COLOR_WHITE;
		lv_obj_t * cpicker = lv_cpicker_create(get_parent(), NULL);
		lv_obj_set_style_local_pad_inner(cpicker, LV_CPICKER_PART_MAIN, LV_STATE_DEFAULT, 0);
		lv_obj_set_style_local_border_color(cpicker, LV_CPICKER_PART_KNOB, LV_STATE_DEFAULT, LV_COLOR_BLACK);
		lv_obj_set_style_local_border_color(cpicker, LV_CPICKER_PART_KNOB, LV_STATE_FOCUSED, LV_COLOR_BLACK);
		set_bg(col);
		lv_obj_set_size(cpicker, WIDTH*2/3, HEIGHT*2/3);
		//lv_cpicker_set_knob_colored(cpicker, false);
		lv_cpicker_set_color_mode_fixed(cpicker, true);
		lv_cpicker_set_color(cpicker, col);
		lv_page_glue_obj(cpicker, true);
		lv_obj_set_user_data(cpicker, this);
		lv_obj_set_event_cb(cpicker, cpicker_cb);
	}

	inline void set_bg(lv_color_t col)
	{
		lv_obj_set_style_local_bg_color(get_parent(), LV_CONT_PART_MAIN, LV_STATE_DEFAULT, col);
	}

	static void cpicker_cb(lv_obj_t * obj, lv_event_t event)
	{
		static uint16_t white_mode = 1;
		lv_color_t col = lv_cpicker_get_color(obj);
		LampTile *inst = (LampTile *)lv_obj_get_user_data(obj);
		//MY_LOG("Color event %d", event);

		switch (event)
		{
		case LV_EVENT_LONG_PRESSED:
			white_mode = !white_mode;
			col = (white_mode ? LV_COLOR_WHITE : LV_COLOR_RED);
			lv_cpicker_set_color(obj, col);
			MY_LOG("Color toggled");
			if (inst) inst->set_bg(col);
			break;

		case LV_EVENT_VALUE_CHANGED:
			if (white_mode)
			{
				col = LV_COLOR_RED;
				lv_cpicker_set_color(obj, col);
				white_mode = 0;
			}
			MY_LOG("Color changed to %02X/%02X/%02X", 
				LV_COLOR_GET_R(col), LV_COLOR_GET_G(col), LV_COLOR_GET_B(col));
			if (inst) inst->set_bg(col);
			break;
		}
	}

};

// ------------------------------------------------------------------------
// Level Tile
// ------------------------------------------------------------------------

class LevelTile : public BaseTile
{
public:
	LevelTile() {}

	virtual void populate()
	{
		lv_cont_set_layout(get_parent(), LV_LAYOUT_OFF);

		// label
		lv_obj_t * label = lv_label_create(get_parent(), NULL);
		lv_label_set_static_text(label, name);
		lv_obj_align_y(label, NULL, LV_ALIGN_IN_TOP_MID, 0);


		// crosshair
		static lv_point_t horiz[2] = { {-WIDTH / 2, 0 }, {WIDTH / 2, 0} };
		static lv_point_t vert[2]  = { {0, -HEIGHT / 2}, {0, HEIGHT / 2} };
		
		static lv_style_t style_line;
		lv_style_init(&style_line);
		lv_style_set_line_width(&style_line, LV_STATE_DEFAULT, 1);
		lv_style_set_line_color(&style_line, LV_STATE_DEFAULT, LV_COLOR_GRAY);

		lv_obj_t * line = lv_line_create(get_parent(), NULL);
		lv_obj_add_style(line, LV_LINE_PART_MAIN, &style_line);
		lv_line_set_points(line, horiz, 2);
		lv_obj_align(line, NULL, LV_ALIGN_CENTER, 0, 0);

		line = lv_line_create(get_parent(), NULL);
		lv_obj_add_style(line, LV_LINE_PART_MAIN, &style_line);
		lv_line_set_points(line, vert, 2);
		lv_obj_align(line, NULL, LV_ALIGN_CENTER, 0, 0);

		// bubble
		lv_obj_t * led = lv_led_create(get_parent(), NULL);
		//lv_obj_set_style_local_outline_width(led, LV_LED_PART_MAIN, LV_STATE_DEFAULT, 0);
		//lv_obj_set_style_local_shadow_width(led, LV_LED_PART_MAIN, LV_STATE_DEFAULT, 0);
		lv_obj_set_style_local_shadow_spread(led, LV_LED_PART_MAIN, LV_STATE_DEFAULT, 3);
		lv_obj_set_size(led, 15, 15);
		lv_obj_align(led, NULL, LV_ALIGN_CENTER, 0, 0);
		bubble = led;
	}

	void update(lv_coord_t x, lv_coord_t y)
	{
		if (bubble)
			lv_obj_align(bubble, NULL, LV_ALIGN_CENTER, x, y);
	}

private:
	lv_obj_t * bubble;
};


// ------------------------------------------------------------------------
// Home Tile
// ------------------------------------------------------------------------

class HomeTile : public BaseTile
{
public:
	HomeTile() :
//		hour(23), min(59), sec(48),
		hour(8), min(19), sec(48),
		weekday(2),
		year(2020), month(9), day(22),
		day_changed(true),
		cal(NULL), alarm(NULL)
	{}

	void setup(CalendarTile *my_cal, AlarmTile *my_alarm)
	{
		cal   = my_cal;
		alarm = my_alarm;

		if (cal)
			cal->update(year, month, day);
		if (alarm)
			alarm->update(weekday, hour, min);
	}

	virtual void populate() = 0;

	virtual void redraw() = 0;

	uint16_t days_in_month()
	{
		uint16_t m = month;

		if (m == 2)
			return ((year % 4) == 0) ? 29 : 28;

		if (m > 6)
			m -= 1;

		return ((m % 2) == 0) ? 30 : 31;
	}

	void update_sec(bool need_draw)
	{
		sec += 1;
		if (sec == 60)
		{
			sec = 0;
			min += 1;
			if (min == 60)
			{
				min = 0;
				hour += 1;
				if (hour == 24)
				{
					hour = 0;
					day_changed = true;

					weekday += 1;
					if (weekday == 7)
						weekday = 0;

					if (day < days_in_month()) // vs. get_month_length() in lv_calendar
					{
						day += 1;
					}
					else
					{
						day = 1;
						if (month < 12)
						{
							month += 1;
						}
						else
						{
							month = 1;
							year += 1;
						}
					}

					if (cal)
						cal->update(year, month, day);
				}
			}

			if (alarm)
				alarm->update(weekday, hour, min);
		}

		if (need_draw)
			redraw();
	}

protected:
	uint16_t hour, min, sec;
	uint16_t weekday; // 0..6, 0:Sunday, 1:Monday, ...
	uint16_t year, month, day; // 0: not used
	bool day_changed;

	// listeners
	CalendarTile *cal;
	AlarmTile *alarm;
};

class DigitalHomeTile : public HomeTile
{
public:
	virtual void populate()
	{
		lv_obj_t * label;

		// time
		label = lv_label_create(get_parent(), NULL);
		lv_obj_add_style(label, LV_OBJ_PART_MAIN, &style_time);
		//lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
		//lv_label_set_static_text(label, "23:59:48");
		label_time = label;

		// weekday
		label = lv_label_create(get_parent(), NULL);
		lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_color_primary());
		//lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
		//lv_label_set_static_text(label, "Dienstag");
		label_weekday = label;

		// date
		label = lv_label_create(get_parent(), NULL);
		//lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_color_secondary());
		//lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
		//lv_label_set_static_text(label, "22.09.2020");
		label_date = label;

		redraw();
	}

	virtual void redraw()
	{
		lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", hour, min, sec);

		if (day_changed)
		{
			lv_label_set_static_text(label_weekday, (weekday < 7) ? weekday_names[weekday] : "???");
			lv_label_set_text_fmt(label_date, "%02d.%02d.%04d", day, month, year);
			day_changed = false;
		}
	}

private:
	// GUI
	lv_obj_t *label_time, *label_weekday, *label_date;
};

class AnalogHomeTile : public HomeTile
{
public:
	virtual void populate()
	{
		lv_cont_set_layout(get_parent(), LV_LAYOUT_OFF);

		img_bg = lv_img_create(get_parent(), NULL);
//		lv_img_set_src(img_bg, &silver_number);
		lv_img_set_src(img_bg, &white_face);
		lv_obj_align(img_bg, NULL, LV_ALIGN_CENTER, 0, 0);

		img_fig = lv_img_create(get_parent(), NULL);
		lv_img_set_src(img_fig, &mickey);
		lv_obj_align(img_fig, NULL, LV_ALIGN_CENTER, 0, 10);

		img_hour = lv_img_create(get_parent(), NULL);
		lv_img_set_src(img_hour, &hand_hour);
		lv_obj_align(img_hour, NULL, LV_ALIGN_CENTER, 0, 0);

		img_min = lv_img_create(get_parent(), NULL);
		lv_img_set_src(img_min, &hand_min);
		lv_obj_align(img_min, NULL, LV_ALIGN_CENTER, 0, 0);

		img_sec = lv_img_create(get_parent(), NULL);
		lv_img_set_src(img_sec, &hand_sec);
		lv_obj_align(img_sec, NULL, LV_ALIGN_CENTER, 0, 0);

		redraw();
	}

	virtual void redraw()
	{
		lv_img_set_angle(img_fig, (sec & 1) ? -25 : 25);
		lv_img_set_angle(img_hour, (hour%12)*300+min*5);
		lv_img_set_angle(img_min, min*60+sec);
		lv_img_set_angle(img_sec, sec*60);
	}

private:
	// GUI
	lv_obj_t *img_bg, *img_fig, *img_hour, *img_min, *img_sec;
};

// ------------------------------------------------------------------------
// Main Tile View
// ------------------------------------------------------------------------

class MainTileView
{
public:
	void create(lv_obj_t * parent)
	{
		lv_coord_t width  = lv_obj_get_width(parent);
		lv_coord_t height = lv_obj_get_height(parent);
		MY_LOG("Create main screen %dx%d", width, height);

		lv_obj_t * tv = lv_tileview_create(parent, NULL);
		lv_obj_set_user_data(tv, this);
		lv_obj_set_event_cb(tv, tv_cb);

		num_tiles = 0;

		lamp.create(tv, "Taschenlampe");
		level.create(tv, "Wasserwaage");
		metronome.create(tv, "Metronom");
		calendar.create(tv, "Kalender");
		pos_time_day = num_tiles;
		time_day.create(tv, "Zeit/Datum");
		alarm.create(tv, "Wecker");
		pos_stopwatch = num_tiles;
		stopwatch.create(tv, "Stoppuhr");
		countdown.create(tv, "Countdown");
		toothbrushing.create(tv, "Zahnputzen");
		MY_LOG("Created %d tiles", num_tiles);

		lv_tileview_set_valid_positions(tv, tile_pos, num_tiles);
		lv_tileview_set_edge_flash(tv, true);
		lv_tileview_set_tile_act(tv, pos_time_day, 0, LV_ANIM_OFF);
//		lv_tileview_set_tile_act(tv, 2, 0, LV_ANIM_OFF); // fixme

		time_day.setup(&calendar, &alarm);

		lv_task_create(task_cb, 0, LV_TASK_PRIO_HIGH, this);
	}

	static void tv_cb(lv_obj_t * obj, lv_event_t event)
	{
		static lv_coord_t last_tile = -1;
		MainTileView *inst = (MainTileView *)lv_obj_get_user_data(obj);

		if (event == LV_EVENT_VALUE_CHANGED)
		{
			lv_coord_t x = 0, y = 0;
			lv_tileview_get_tile_act(obj, &x, &y);
			//MY_LOG("Changed to tile at %d / %d", x, y);
			if ((y == 0) && (x != last_tile))
			{
				if (inst)
					inst->tile_changed_cb(last_tile, x);
				last_tile = x;
			}
		}
	}

	void tile_changed_cb(lv_coord_t old_pos, lv_coord_t new_pos)
	{
		MY_LOG("Tile changed from %d to %d", old_pos, new_pos);

		// run highspeed for stopwatch only when present
		if (old_pos == pos_stopwatch)
			set_normal_speed();
		if ((new_pos == pos_stopwatch) && (stopwatch.is_running()))
			set_high_speed();
	}

	void check_sec(uint32_t curr_ms)
	{
		static uint32_t last_ms = 0;
		static uint32_t num = 0;

		num++;
		//MY_LOG("my_task called at %d ms", curr_ms);

		if (stopwatch.is_running() && lv_obj_is_visible(stopwatch.get_parent()))
		{
			stopwatch.update_ms(curr_ms);
		} 
		else if (lv_obj_is_visible(level.get_parent()))
		{ 
			// simulation
			level.update(50 * sin(0.003*curr_ms), 50 * cos(0.005*curr_ms));
		}

		if (metronome.is_running())
			metronome.update_ms(curr_ms);

		uint32_t elapsed = diff_time(curr_ms, last_ms);
		if (elapsed >= 1000)
		{
			MY_LOG("my_task called %d times during 1s", num);
			time_day.update_sec(num > 1);
			countdown.update_sec(num > 1);
			toothbrushing.update_sec(num > 1);
			last_ms += 1000;
			num = 0;
		}
	}

	static void task_cb(lv_task_t * task)
	{
		uint32_t curr_ms = task->last_run;
		MainTileView *inst = (MainTileView *)(task->user_data);
		if (inst)
			inst->check_sec(curr_ms);
	}

private:
	// Tile positions
	lv_coord_t pos_time_day, pos_stopwatch;

	// Tiles
	CalendarTile calendar;
	AlarmTile alarm;
	StopwatchTile stopwatch;
	CountdownTile countdown;
	ToothbrushingTile toothbrushing;
	MetronomeTile metronome;
	LevelTile level;
	LampTile lamp;
//	DigitalHomeTile time_day;
	AnalogHomeTile time_day;
};

// ------------------------------------------------------------------------
// MyWatch
// ------------------------------------------------------------------------

extern "C" void my_watch(void)
{
	static MainTileView mtv;

	set_normal_speed();

	home = lv_scr_act();
	app  = NULL;

	mtv.create(home);
}

// ------------------------------------------------------------------------
