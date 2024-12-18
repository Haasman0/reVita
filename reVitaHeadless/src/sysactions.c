#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <psp2kern/power.h>
#include <psp2/appmgr.h>
#include "vitasdkext.h"
#include "common.h"
#include "revita.h"
#include "main.h"
#include "fio/fio.h"
#include "fio/profile.h"
#include "fio/settings.h"
#include "sysactions.h"
#include "log.h"

#define PATH_SAVE_BACKUP    "ux0:/data/reVita/Save"
#define PATH_SAVE           "ux0:/user/00/savedata"

#define BRIGHTNESS_STEP     6
int brightnessLevel;

void sysactions_softReset(){
    kscePowerRequestSoftReset();
}
void sysactions_coldReset(){
    kscePowerRequestColdReset();
}
void sysactions_standby(){
    kscePowerRequestStandby();
}
void sysactions_suspend(){
    kscePowerRequestSuspend();
}
void sysactions_displayOff(){
    kscePowerRequestDisplayOff();
}
void sysactions_killCurrentApp() {
	if (processid == -1 || processid == shellPid) return;
	ksceAppMgrKillProcess(processid);
}

void sysactions_brightnessInc(){
    brightnessLevel = clamp(
        21 + (brightnessLevel + (0xFFFF - 21) * BRIGHTNESS_STEP / 100),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
	ksceRegMgrSetKeyInt("/CONFIG/DISPLAY", "brightness", brightnessLevel);
}
void sysactions_brightnessDec(){
    brightnessLevel = clamp(
        21 + (brightnessLevel - (0xFFFF - 21) * BRIGHTNESS_STEP / 100),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
	ksceRegMgrSetKeyInt("/CONFIG/DISPLAY", "brightness", brightnessLevel);
}

void sysactions_saveBackup(){
	//Create dir if not exists
	ksceIoMkdir(PATH_SAVE_BACKUP, 0777); 

    char src[64];
    char dest[64];
	sprintf(src, "%s/%s", PATH_SAVE, titleid);
	sprintf(dest, "%s/%s", PATH_SAVE_BACKUP, titleid);
    
    if (fio_exist(src)){
        LOG("save found\n");
        if (fio_copyDir(src, dest) == 0){
            return;
        }
    }

}

void sysactions_saveRestore(){
    char src[64];
    char dest[64];
	sprintf(src, "%s/%s", PATH_SAVE_BACKUP, titleid);
	sprintf(dest, "%s/%s", PATH_SAVE, titleid);
    
    if (fio_exist(src)){
        if (fio_copyDir(src, dest) == 0){
            return;
		}
    }

}

void sysactions_saveDelete(){
    char src[64];
	sprintf(src, "%s/%s", PATH_SAVE_BACKUP, titleid);
    
    if (fio_exist(src)){
        if (fio_deletePath(src) == 1){
            return;
        }
    }

}

void sysactions_saveDeleteAll(){
    if (fio_exist(PATH_SAVE_BACKUP)){
        if (fio_deletePath(PATH_SAVE_BACKUP) == 1){
            return;
        }
    }
}

void sysactions_calibrateMotion(){
	SceMotionState sms;
	int gyroRet = revita_sceMotionGetState(&sms);
    if (gyroRet >= 0){     
        profile.entries[PR_GY_CALIBRATION_X].v.i = clamp(
            (int)(sms.rotationMatrix.x.z * 1000), 
            profile.entries[PR_GY_CALIBRATION_X].min.i, 
            profile.entries[PR_GY_CALIBRATION_X].max.i);   
        profile.entries[PR_GY_CALIBRATION_Y].v.i = clamp(
            (int)(sms.rotationMatrix.y.z * 1000), 
            profile.entries[PR_GY_CALIBRATION_Y].min.i, 
            profile.entries[PR_GY_CALIBRATION_Y].max.i);
        profile.entries[PR_GY_CALIBRATION_Z].v.i = clamp(
            (int)(sms.acceleration.x * 1000), 
            profile.entries[PR_GY_CALIBRATION_Z].min.i, 
            profile.entries[PR_GY_CALIBRATION_Z].max.i); 
        // void __sceMotionReset();
    }
}

void sysactions_toggleSecondary(){
    isSecondaryProfileLoaded = !isSecondaryProfileLoaded;
    profile_load(titleid);
}

void sysactions_init(){
	ksceRegMgrGetKeyInt("/CONFIG/DISPLAY", "brightness", (int *)&brightnessLevel);
}