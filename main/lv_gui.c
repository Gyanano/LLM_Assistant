#include "lvgl.h"
#include "main.h"

// the whole variables
lv_obj_t *main_obj;
lv_obj_t *sub_obj;
int is_in_app = 0; // 1: clock, 2: chat, 3: weather, 4: setting

lv_obj_t *label1;
lv_obj_t *label2;
lv_obj_t *label_wifi_info;
int is_wifi_connected = 0;

// test count
static int count = 0;

LV_IMG_DECLARE(img_bilibili120);
LV_FONT_DECLARE(font_alipuhui20);

void back_event_handler(lv_event_t *e)
{
    lv_obj_del(sub_obj);
    is_in_app = 0;
}

static void default_style_init(lv_style_t *style)
{
    lv_style_init(style);
    lv_style_set_radius(style, 10); // 设置圆角半径
    lv_style_set_bg_opa(style, LV_OPA_COVER);
    lv_style_set_bg_color(style, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(style, 0);
    lv_style_set_pad_all(style, 10);
    lv_style_set_width(style, 320);  // 设置宽
    lv_style_set_height(style, 240); // 设置高
}

static void create_back_btn(lv_obj_t *parent)
{
    // create a back btn on the top left corner
    static lv_style_t back_style;
    lv_style_init(&back_style);
    lv_style_set_radius(&back_style, 3);
    lv_style_set_bg_opa(&back_style, LV_OPA_COVER);
    lv_style_set_text_color(&back_style, lv_color_hex(0xffffff));
    lv_style_set_border_width(&back_style, 0);
    lv_style_set_width(&back_style, 50);
    lv_style_set_height(&back_style, 32);
    // 创建图标 clock
    lv_obj_t *icon_back = lv_btn_create(parent);
    lv_obj_add_style(icon_back, &back_style, 0);
    lv_obj_set_style_bg_color(icon_back, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(icon_back, 0, 0);
    lv_obj_add_event_cb(icon_back, back_event_handler, LV_EVENT_CLICKED, 1);
    lv_obj_t *img_back = lv_img_create(icon_back);
    LV_IMG_DECLARE(_back_black_alpha_16x16);
    lv_img_set_src(img_back, &_back_black_alpha_16x16);
    lv_obj_align(img_back, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(icon_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(icon_back, back_event_handler, LV_EVENT_CLICKED, NULL);
}

void lv_gui_start(void)
{
    // 显示开机GIF图片
    lv_obj_t *gif_start = lv_gif_create(lv_scr_act());
    lv_gif_set_src(gif_start, &img_bilibili120);
    lv_obj_align(gif_start, LV_ALIGN_CENTER, 0, -20);

    // 显示wifi连接信息
    label_wifi_info = lv_label_create(lv_scr_act());
    lv_obj_align(label_wifi_info, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_set_style_text_font(label_wifi_info, &font_alipuhui20, LV_STATE_DEFAULT);
    // lv_label_set_text(label_wifi_info, "正在连接wifi...");
    lv_label_set_text(label_wifi_info, "初始化系统...");
}

/*
void lv_gui_load(const char* text)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xffffff), 0);

    static lv_style_t style;
    default_style_init(&style);

    lv_obj_t *gif_start = lv_gif_create(lv_scr_act());
    lv_gif_set_src(gif_start, &img_bilibili120);
    lv_obj_align(gif_start, LV_ALIGN_CENTER, 0, -20);

    label_wifi_info = lv_label_create(lv_scr_act());
    lv_obj_align(label_wifi_info, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_set_style_text_font(label_wifi_info, &font_alipuhui20, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(label_wifi_info, "%s...", text);
}*/

void clock_event_handler(lv_event_t *e)
{
    int selected_icon = lv_event_get_user_data(e);
    is_in_app = selected_icon;
    switch (is_in_app)
    {
    case 1:
        // clock app
        lv_clock_page();
        break;
    case 2:
        // chat app
        lv_chat_page();
        break;
    case 3:
        // weather app
        lv_weather_page();
        break;
    case 4:
        // setting app
        lv_setting_page();
        break;
    default:
        break;
    }
    return;
}

void lv_main_page(void)
{
    // 这里显示主界面，即多个应用图标
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0); // 修改背景为黑色

    // 创建主页面
    static lv_style_t style;
    default_style_init(&style);
    main_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(main_obj, &style, 0);

    // 创建图标 style
    static lv_style_t icon_style;
    lv_style_init(&icon_style);
    lv_style_set_radius(&icon_style, 10);
    lv_style_set_bg_opa(&icon_style, LV_OPA_COVER);
    lv_style_set_text_color(&icon_style, lv_color_hex(0xffffff));
    lv_style_set_border_width(&icon_style, 0);
    lv_style_set_pad_all(&icon_style, 5);
    lv_style_set_width(&icon_style, 100);
    lv_style_set_height(&icon_style, 100);

    // 创建图标 clock
    lv_obj_t *icon_clock = lv_btn_create(main_obj);
    lv_obj_add_style(icon_clock, &icon_style, 0);
    lv_obj_set_style_bg_color(icon_clock, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(icon_clock, 40, 10);
    lv_obj_add_event_cb(icon_clock, clock_event_handler, LV_EVENT_CLICKED, 1);
    lv_obj_t *img_clock = lv_img_create(icon_clock);
    LV_IMG_DECLARE(_clock_alpha_60x60);
    lv_img_set_src(img_clock, &_clock_alpha_60x60);
    lv_obj_align(img_clock, LV_ALIGN_CENTER, 0, 0);

    // 创建图标 chat
    lv_obj_t *icon_chat = lv_btn_create(main_obj);
    lv_obj_add_style(icon_chat, &icon_style, 0);
    lv_obj_set_style_bg_color(icon_chat, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(icon_chat, 160, 10);
    lv_obj_add_event_cb(icon_chat, clock_event_handler, LV_EVENT_CLICKED, 2);
    lv_obj_t *img_chat = lv_img_create(icon_chat);
    LV_IMG_DECLARE(_message_alpha_80x80);
    lv_img_set_src(img_chat, &_message_alpha_80x80);
    lv_obj_align(img_chat, LV_ALIGN_CENTER, 0, 0);

    // 创建图标 weather
    lv_obj_t *icon_weather = lv_btn_create(main_obj);
    lv_obj_add_style(icon_weather, &icon_style, 0);
    lv_obj_set_style_bg_color(icon_weather, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(icon_weather, 40, 120);
    lv_obj_add_event_cb(icon_weather, clock_event_handler, LV_EVENT_CLICKED, 3);
    lv_obj_t *img_weather = lv_img_create(icon_weather);
    LV_IMG_DECLARE(_weather_alpha_80x80);
    lv_img_set_src(img_weather, &_weather_alpha_80x80);
    lv_obj_align(img_weather, LV_ALIGN_CENTER, 0, 0);

    // 创建图标 setting
    lv_obj_t *icon_setting = lv_btn_create(main_obj);
    lv_obj_add_style(icon_setting, &icon_style, 0);
    lv_obj_set_style_bg_color(icon_setting, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(icon_setting, 160, 120);
    lv_obj_add_event_cb(icon_setting, clock_event_handler, LV_EVENT_CLICKED, 4);
    lv_obj_t *img_setting = lv_img_create(icon_setting);
    LV_IMG_DECLARE(_setting_alpha_80x80);
    lv_img_set_src(img_setting, &_setting_alpha_80x80);
    lv_obj_align(img_setting, LV_ALIGN_CENTER, 0, 0);
}

void lv_chat_page(void)
{
    static lv_style_t style;
    default_style_init(&style);

    /*Create an object with the new style*/
    sub_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(sub_obj, &style, 0);

    create_back_btn(sub_obj);

    label1 = lv_label_create(sub_obj);
    lv_obj_set_width(label1, 300);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    lv_obj_align(label1, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_obj_set_style_text_font(label1, &font_alipuhui20, LV_STATE_DEFAULT);
    lv_label_set_text(label1, "我：");

    label2 = lv_label_create(sub_obj);
    lv_obj_set_width(label2, 300);
    lv_obj_align(label2, LV_ALIGN_TOP_LEFT, 0, 75);
    lv_obj_set_style_text_font(label2, &font_alipuhui20, LV_STATE_DEFAULT);
    lv_label_set_text(label2, "AI：");
}

lv_obj_t *time_label;

void lv_clock_page(void)
{
    static lv_style_t style;
    default_style_init(&style);

    // maybe need to create a timer to update the time

    // Get the current time
    time(&now);
    localtime_r(&now, &timeinfo);

    sub_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(sub_obj, &style, 0);

    create_back_btn(sub_obj);

    time_label = lv_label_create(sub_obj);
    lv_obj_set_style_text_font(time_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_obj_center(time_label);
    // lv_obj_set_pos(time_label, 142, 42);

    // lv_obj_t *temp_img = lv_img_create(sub_obj);
    // lv_obj_add_flag(temp_img, LV_OBJ_FLAG_CLICKABLE);
    // LV_IMG_DECLARE(_clock_alpha_60x60);
    // lv_img_set_src(temp_img, &_clock_alpha_60x60);
    // lv_img_set_pivot(temp_img, 40, 40);
    // lv_img_set_angle(temp_img, 0);
    // lv_obj_set_pos(temp_img, 120 + 10, 80 + 10);
    // Write style for temp_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    // lv_obj_set_style_img_opa(temp_img, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void lv_weather_page(void)
{
    static lv_style_t style;
    default_style_init(&style);

    sub_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(sub_obj, &style, 0);

    create_back_btn(sub_obj);

    lv_obj_t *temp_label = lv_label_create(sub_obj);
    lv_obj_set_style_text_font(temp_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(temp_label, "%s℃，%s%%", g_temp, g_humidity);
    lv_obj_center(temp_label);

    // lv_obj_t *temp_img = lv_img_create(sub_obj);
    // lv_obj_add_flag(temp_img, LV_OBJ_FLAG_CLICKABLE);
    // LV_IMG_DECLARE(_weather_alpha_80x80);
    // lv_img_set_src(temp_img, &_weather_alpha_80x80);
    // lv_img_set_pivot(temp_img, 40, 40);
    // lv_img_set_angle(temp_img, 0);
    // lv_obj_set_pos(temp_img, 120, 80);
    // lv_obj_set_size(temp_img, 80, 80);

    // Write style for temp_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    // lv_obj_set_style_img_opa(temp_img, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void slider_event_cb(lv_event_t *e)
{
    int x;

    lv_obj_t *text_label = lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    lv_slider_set_range(slider, 10, 90);
    x = lv_slider_get_value(slider);
    lv_label_set_text_fmt(text_label, "屏幕亮度：%3d", x);
    bg_duty = (float)x / 100; // 根据滑动条的值计算占空比
    // 设置占空比
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191 * (1 - bg_duty));
    // 更新背光
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void lv_setting_page(void)
{
    static lv_style_t style;
    default_style_init(&style);

    sub_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(sub_obj, &style, 0);

    create_back_btn(sub_obj);

    lv_obj_t *text_label = lv_label_create(sub_obj);
    lv_obj_set_style_text_color(text_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(text_label, &font_alipuhui20, 0);
    lv_obj_align(text_label, LV_ALIGN_CENTER, 0, 60);
    lv_label_set_text_fmt(text_label, "屏幕亮度：%3d", (int)(bg_duty * 100));

    // 创建一个滑动条
    lv_obj_t *slider = lv_slider_create(sub_obj);
    lv_slider_set_value(slider, bg_duty * 100, 0);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_height(slider, 25);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, text_label);
}
