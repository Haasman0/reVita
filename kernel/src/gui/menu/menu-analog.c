#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_analog(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_START: profile_resetAnalog(); break;
		default: onButton_genericEntries(btn);
	}
}

void onDraw_analog(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {			
		gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

#define MENU_ANALOG_NUM 5
static struct MenuEntry menu_analog_entries[MENU_ANALOG_NUM] = {
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "$U Left  Analog - X Axis", .dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_X]},
	(MenuEntry){.name = "$L Left  Analog - Y Axis", .dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_Y]},
	(MenuEntry){.name = "$u Right Analog - X Axis", .dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_X]},
	(MenuEntry){.name = "$l Right Analog - Y Axis", .dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_Y]}};
static struct Menu menu_analog = (Menu){
	.id = MENU_ANALOG_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_ANALOG_NUM, 
	.name = "$u ANALOG STICKS", 
	.footer = "$SRESET  $:RESET ALL",
	.onButton = onButton_analog,
	.onDraw = onDraw_analog,
	.entries = menu_analog_entries};

void menu_initAnalog(){
	gui_registerMenu(&menu_analog);
}