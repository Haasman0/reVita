#include <vitasdkkern.h>
#include <psp2/touch.h> 
#include <stdlib.h>

#include "vitasdkext.h"
#include "main.h"
#include "ui.h"
#include "profile.h"
#include "common.h"

static uint32_t curr_buttons;
static uint32_t old_buttons;
static uint64_t tick;
static uint64_t pressedTicks[PHYS_BUTTONS_NUM];

#define LONG_PRESS_TIME   350000	//0.35sec

uint8_t isBtnActive(uint8_t btnNum){
	return ((curr_buttons & HW_BUTTONS[btnNum]) && !(old_buttons & HW_BUTTONS[btnNum])) 
		|| (pressedTicks[btnNum] != 0 && tick - pressedTicks[btnNum] > LONG_PRESS_TIME);
}

//Set custom touch point xy using RS
void analogTouchPicker(TouchPoint* tp, SceCtrlData *ctrl, int isFront, int isLeftAnalog){
	int shiftX = ((float)((isLeftAnalog ? ctrl->lx : ctrl->rx) - 127)) / 8;
	int shiftY = ((float)((isLeftAnalog ? ctrl->ly : ctrl->ry) - 127)) / 8;
	if (abs(shiftX) > 30 / 8)
		tp->x = clamp(tp->x + shiftX, 0, isFront ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
	if (abs(shiftY) > 30 / 8)
		tp->y = clamp(tp->y + shiftY, 0, isFront ? TOUCH_SIZE[1] : TOUCH_SIZE[3]);
}
//Set custom touch point xy using touch
void touchPicker(TouchPoint* tp, int port){
	SceTouchData std;
	internal_touch_call = 1;
	int ret = ksceTouchPeek(port, &std, 1);
	internal_touch_call = 0;
	if (ret && std.reportNum){
		tp->x = std.report[0].x;
		tp->y =std.report[0].y;
	}
}
void onInput_touchPicker(SceCtrlData *ctrl){
	RemapAction* ra = (RemapAction*)ui_menu->data;
	int port = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ?
		SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	if (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_BACK_TOUCH_POINT){
		touchPicker(&ra->param.touch, SCE_TOUCH_PORT_FRONT);
		analogTouchPicker(&ra->param.touch, ctrl, port == SCE_TOUCH_PORT_FRONT, 0);
	} else if (ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE){
		TouchPoint* tpA = &ra->param.zone.a;
		TouchPoint* tpB = &ra->param.zone.b;
		touchPicker((ui_entry->id < 2) ? tpA : tpB, SCE_TOUCH_PORT_FRONT);
		analogTouchPicker(tpA, ctrl, port == SCE_TOUCH_PORT_FRONT, 0);
		analogTouchPicker(tpB, ctrl, port == SCE_TOUCH_PORT_FRONT, 1);
	}
}

void onButton_generic(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN: ui_nextEntry(); break;
		case SCE_CTRL_UP: ui_prevEntry(); break;
		case SCE_CTRL_CIRCLE: ui_openMenuParent(); break;
	}
}

void onButton_main(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_openMenu(ui_menu->entries[ui_menu->idx].id);
			ui_setIdx(0);
			break;
		case SCE_CTRL_CIRCLE:
			ui_opened = 0;
			profile_saveSettings();
			if (profile_settings[0])
				profile_save(titleid);
			delayedStart();
			break;
		default: onButton_generic(btn);
	}
}
void onButton_analog(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			if (id == HEADER_IDX) break;
			if (id < 4) profile.analog[id] = (profile.analog[id] + 1) % 128;
			else profile.analog[id] = !profile.analog[id];
			break;
		case SCE_CTRL_LEFT:
			if (id == HEADER_IDX) break;
			if (profile.analog[id]) 	
				profile.analog[id]--;
			else
				profile.analog[id] = id < 4 ? 127 : 1;
			break;
		case SCE_CTRL_SQUARE:
			profile.analog[id] = profile_def.analog[id];
			break;
		case SCE_CTRL_START: profile_resetAnalog(); break;
		default: onButton_generic(btn);
	}
}
void onButton_touch(uint32_t btn){
	int8_t idx = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			if (idx == HEADER_IDX) break;
			else if (idx < 8)//Front Points xy
				profile.touch[idx] = (profile.touch[idx] + 1) 
					% ((idx % 2) ? TOUCH_SIZE[1] : TOUCH_SIZE[0]);
			else if (idx < 16)//Rear Points xy
				profile.touch[idx] = (profile.touch[idx] + 1)
					% ((idx % 2) ? TOUCH_SIZE[3] : TOUCH_SIZE[2]);
			else 			//yes/no otion
				profile.touch[idx] = !profile.touch[idx];
			break;
		case SCE_CTRL_LEFT:
			if (idx == HEADER_IDX) break;
			else if (profile.touch[idx]) 	
				profile.touch[idx]--;
			else {
				if (idx < 8)//front points xy
					profile.touch[idx] = ((idx % 2) ? TOUCH_SIZE[1] - 1 : TOUCH_SIZE[0] - 1);
				if (idx < 16)//rear points xy
					profile.touch[idx] = ((idx % 2) ? TOUCH_SIZE[3] - 1 : TOUCH_SIZE[2] - 1);
				else //yes/no options
					profile.touch[idx] = !profile.touch[idx];
			}
			break;
		case SCE_CTRL_SQUARE:
			profile.touch[idx] = profile_def.touch[idx];
			break;
		case SCE_CTRL_START: profile_resetTouch(); break;
		default: onButton_generic(btn);
	}
}
void onButton_gyro(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			if (id < 6) //Sens & deadzone
				profile.gyro[id] = (profile.gyro[id] + 1) % 200;
			else if (id == 6) // Deadband
				profile.gyro[id] = min(2, profile.gyro[id] + 1);
			else if (id == 7) // Wheel mode
				profile.gyro[id] = (profile.gyro[id] + 1) % 2;
			/*else if (cfg_i < 10) // Reset wheel buttons
				profile.gyro[cfg_i] 
					= min(PHYS_BUTTONS_NUM - 1, profile.gyro[cfg_i] + 1);*/
			break;
		case SCE_CTRL_LEFT:
			if (profile.gyro[id]) 	
				profile.gyro[id]--;
			else {
				if (id < 6) //Sens & deadzone
					profile.gyro[id] = 199;
				else if (id == 6) // deadband
					profile.gyro[id] = max(0, profile.gyro[id] - 1);
				else if (id == 7) //Wheel mode
					profile.gyro[id] = 1;
				/*else if (idx < 10)  // Reset wheel btns
					profile.gyro[idx] = max(0, profile.gyro[idx] - 1);*/
			}
			break;
		case SCE_CTRL_SQUARE:
			profile.gyro[id] = profile_def.gyro[id];
			break;
		case SCE_CTRL_START: profile_resetGyro(); break;
		default: onButton_generic(btn);
	}
}
void onButton_controller(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			if (id == 1)
				profile.controller[id] = min(5, profile.controller[id] + 1);
			else
				profile.controller[id] = !profile.controller[id];
			break;
		case SCE_CTRL_LEFT:
			if (id == 1)
				profile.controller[id] = max(0, profile.controller[id] - 1);
			else
				profile.controller[id] = !profile.controller[id];
			break;
		case SCE_CTRL_SQUARE:
			profile.controller[id] = profile_def.controller[id];
			break;
		case SCE_CTRL_START: profile_resetController(); break;
		default: onButton_generic(btn);
	}
}
void onButton_settings(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			if (id < 2)
				profile_settings[id] 
					= min(PHYS_BUTTONS_NUM - 1, profile_settings[id] + 1);
			else if (id == 2)
				profile_settings[id] = !profile_settings[id];
			else if (id == 3)
				profile_settings[id] 
					= min(60, profile_settings[id] + 1);
			break;
		case SCE_CTRL_LEFT:
			if (id < 2)
				profile_settings[id] 
					= max(0, profile_settings[id] - 1);
			else if (id == 2)
				profile_settings[id] = !profile_settings[id];
			else if (id == 3)
				profile_settings[id] 
					= max(0, profile_settings[id] - 1);
			break;
		case SCE_CTRL_SQUARE:
			if (id <= 2)
				profile_settings[id] = profile_settings_def[id];
			break;
		case SCE_CTRL_START: profile_resetSettings(); break;
		default: onButton_generic(btn);
	}
}
void onButton_profiles(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			switch (ui_entry->id){
				case PROFILE_GLOBAL_SAVE: profile_saveAsGlobal(); break;
				case PROFILE_GLOABL_LOAD: profile_loadGlobal(); break;
				case PROFILE_GLOBAL_DELETE: profile_resetGlobal(); break;
				case PROFILE_LOCAL_SAVE: profile_save(titleid); break;
				case PROFILE_LOCAL_LOAD: profile_load(titleid); break;
				case PROFILE_LOCAL_DELETE: profile_delete(titleid); break;
				default: break;
			}
			break;
		default: onButton_generic(btn);
	}
}

void onButton_pickButton(uint32_t btn){
	uint32_t* btnP = (uint32_t *)ui_menu->data;
	switch (btn) {
		case SCE_CTRL_SQUARE:
			btn_toggle(btnP, HW_BUTTONS[ui_entry->id]);
			break;
		case SCE_CTRL_CROSS:
			if (!*btnP) break;
			if (ui_menu->next == MENU_REMAP_ID)
				profile_addRemapRule(ui_ruleEdited);
			ui_openMenuNext();
			break;
		case SCE_CTRL_CIRCLE: ui_openMenuPrev(); break;
		default: onButton_generic(btn);
	}
}
void onButton_pickAnalog(uint32_t btn){
	enum REMAP_ACTION* actn = (enum REMAP_ACTION*)ui_menu->data;
	switch (btn) {
		case SCE_CTRL_CROSS:
			*actn = ui_entry->id;
			if (ui_menu->next == MENU_REMAP_ID)
				profile_addRemapRule(ui_ruleEdited);
			ui_openMenuNext();
			break;
		case SCE_CTRL_CIRCLE: ui_openMenuPrev();
		default: onButton_generic(btn);
	}
}
void onButton_pickTouchPoint(uint32_t btn){
	RemapAction* ra = (RemapAction*)ui_menu->data;
	uint8_t isFront = ra->type == REMAP_TYPE_FRONT_TOUCH_POINT;
	switch (btn) {
		case SCE_CTRL_CROSS:
			if(ui_menu->next == MENU_REMAP_ID)
				profile_addRemapRule(ui_ruleEdited);
			ui_openMenuNext();
		case SCE_CTRL_RIGHT:
			switch (ui_entry->id){
				case 0: ra->param.touch.x = min(ra->param.touch.x + 1, isFront ? TOUCH_SIZE[0] : TOUCH_SIZE[2]); break;
				case 1: ra->param.touch.y = min(ra->param.touch.y + 1, isFront ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); break;
			}
			break;
		case SCE_CTRL_LEFT:
			switch (ui_entry->id){
				case 0: ra->param.touch.x = max(ra->param.touch.x - 1, 0); break;
				case 1: ra->param.touch.y = max(ra->param.touch.y - 1, 0); break;
			}
			break;
		case SCE_CTRL_CIRCLE: ui_openMenuPrev();
		default: onButton_generic(btn);
	}
}
void onButton_pickTouchZone(uint32_t btn){
	RemapAction* ra = (RemapAction*)ui_menu->data;
	uint8_t isFront = ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE;
	switch (btn) {
		case SCE_CTRL_CROSS:
			if(ui_menu->next == MENU_REMAP_ID)
				profile_addRemapRule(ui_ruleEdited);
			ui_openMenuNext();
		case SCE_CTRL_RIGHT:
			switch (ui_entry->id){
				case 0: ra->param.zone.a.x = min(ra->param.zone.a.x + 1, isFront ? TOUCH_SIZE[0] : TOUCH_SIZE[2]); break;
				case 1: ra->param.zone.a.y = min(ra->param.zone.a.y + 1, isFront ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); break;
				case 2: ra->param.zone.b.x = min(ra->param.zone.b.x + 1, isFront ? TOUCH_SIZE[0] : TOUCH_SIZE[2]); break;
				case 3: ra->param.zone.b.y = min(ra->param.zone.b.y + 1, isFront ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); break;
			}
			break;
		case SCE_CTRL_LEFT:
			switch (ui_entry->id){
				case 0: ra->param.zone.a.x = max(ra->param.zone.a.x - 1, 0); break;
				case 1: ra->param.zone.a.y = max(ra->param.zone.a.y - 1, 0); break;
				case 2: ra->param.zone.b.x = max(ra->param.zone.b.x - 1, 0); break;
				case 3: ra->param.zone.b.y = max(ra->param.zone.b.y - 1, 0); break;
			}
			break;
		case SCE_CTRL_CIRCLE: ui_openMenuPrev();
		default: onButton_generic(btn);
	}
}

void onButton_remap(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS: 
			if (ui_entry->id != NEW_RULE_IDX)
				ui_ruleEdited = profile.remaps[ui_entry->id];
			ui_openMenu(MENU_REMAP_TRIGGER_TYPE_ID); 
			break;
		case SCE_CTRL_SQUARE:
			if (ui_entry->id != NEW_RULE_IDX)
				profile.remaps[ui_entry->id].disabled = !profile.remaps[ui_entry->id].disabled;
			break;
		case SCE_CTRL_START:
			if (ui_entry->id != NEW_RULE_IDX){
				profile_removeRemapRule(ui_entry->id);
				ui_openMenu(ui_menu->id);
			}
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.type = ui_entry->id;
			switch(ui_entry->id){
				case REMAP_TYPE_BUTTON: 
				case REMAP_TYPE_COMBO: ui_openMenuSmart(MENU_PICK_ANALOG_ID, 
					ui_menu->id, MENU_REMAP_EMU_TYPE_ID, (uint32_t)&ui_ruleEdited.trigger.param.btn); break;
				case REMAP_TYPE_LEFT_ANALOG: 
				case REMAP_TYPE_RIGHT_ANALOG: ui_openMenuSmart(MENU_PICK_BUTTON_ID,
					ui_menu->id, MENU_REMAP_EMU_TYPE_ID, (uint32_t)&ui_ruleEdited.trigger.action); break;
				case REMAP_TYPE_FRONT_TOUCH_ZONE:
				case REMAP_TYPE_BACK_TOUCH_ZONE: ui_openMenu(MENU_REMAP_TRIGGER_TOUCH_ID); break;
				case REMAP_TYPE_GYROSCOPE: ui_openMenu(MENU_REMAP_TRIGGER_GYRO_ID); break;
			};
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerTouch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.action = ui_entry->id;
			if (ui_entry->id == REMAP_TOUCH_CUSTOM){
				ui_openMenuSmart(MENU_PICK_TOUCH_ZONE_ID, ui_menu->id, MENU_REMAP_EMU_TYPE_ID, 
					(uint32_t)&ui_ruleEdited.trigger);
				break;
			}
			//ToDo Set custom touch zones here
			ui_openMenu(MENU_REMAP_EMU_TYPE_ID);
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerGyro(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.action = ui_entry->id;
			ui_openMenu(MENU_REMAP_EMU_TYPE_ID);
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.type = ui_entry->id;
			switch(ui_entry->id){
				case REMAP_TYPE_BUTTON: 
				case REMAP_TYPE_COMBO: 
					ui_openMenuSmart(MENU_PICK_ANALOG_ID, 
						ui_menu->id, MENU_REMAP_ID, (uint32_t)&ui_ruleEdited.emu.param.btn); 
					break;
				case REMAP_TYPE_LEFT_ANALOG:
				case REMAP_TYPE_LEFT_ANALOG_DIGITAL:
				case REMAP_TYPE_RIGHT_ANALOG:
				case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
					ui_openMenuSmart(MENU_PICK_BUTTON_ID,
						ui_menu->id, MENU_REMAP_ID, (uint32_t)&ui_ruleEdited.emu.action); 
					break;
				case REMAP_TYPE_FRONT_TOUCH_POINT:
				case REMAP_TYPE_BACK_TOUCH_POINT: ui_openMenu(MENU_REMAP_EMU_TOUCH_ID); break;
			};
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuTouch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.action = ui_entry->id;
			if (ui_entry->id == REMAP_TOUCH_CUSTOM){
				ui_openMenuSmart(MENU_PICK_TOUCH_POINT_ID, ui_menu->id, MENU_REMAP_ID, 
					(uint32_t)&ui_ruleEdited.emu);
				break;
			}
			//ToDo Add custom touch points here
			profile_addRemapRule(ui_ruleEdited);
			ui_openMenu(MENU_REMAP_ID);
			break;
		default: onButton_generic(btn);
	}
}

void ctrl_onInput(SceCtrlData *ctrl) {
	if ((ctrl->buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl->buttons & HW_BUTTONS[profile_settings[1]]))
		return; //Menu trigger butoons should not trigger any menu actions on menu open
	if (new_frame) {
		new_frame = 0;
		
		if (ui_menu->onInput)
			ui_menu->onInput(ctrl);

		tick = ctrl->timeStamp;
		curr_buttons = ctrl->buttons;
		for (int i = 0; i < PHYS_BUTTONS_NUM; i++){
			if ((curr_buttons & HW_BUTTONS[i]) && !(old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = tick;
			else if (!(curr_buttons & HW_BUTTONS[i]) && (old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = 0;
			
			if (!isBtnActive(i))
				continue;
			if (ui_menu->onButton)
				ui_menu->onButton(HW_BUTTONS[i]);
			else 
				onButton_generic(HW_BUTTONS[i]);

		}
		old_buttons = curr_buttons;
	}
}

void ui_control_init(){
}
void ui_control_destroy(){
}