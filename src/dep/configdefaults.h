/**
 * @file   configdefaults.h
 *
 * @brief  definitions related to config templates and defaults
 *
 *
 */

#ifndef PTPD_CONFIGDEFAULTS_H_
#define PTPD_CONFIGDEFAULTS_H_

#include "ptp_primitives.h"
#include "dep/iniparser/dictionary.h"
#include "datatypes_stub.h"

#define DEFAULT_TEMPLATE_FILE DATADIR"/"PACKAGE_NAME"/templates.conf"

typedef struct{

    UInteger8 minValue;
    UInteger8 maxValue;
    UInteger8 defaultValue;

} UInteger8_option;

typedef struct{

    Integer32  minValue;
    Integer32  maxValue;
    Integer32  defaultValue;

} Integer32_option;

typedef struct{

    UInteger32  minValue;
    UInteger32  maxValue;
    UInteger32  defaultValue;

} UInteger32_option;

typedef struct{

    Integer16  minValue;
    Integer16  maxValue;
    Integer16  defaultValue;

} Integer16_option;

typedef struct{

    UInteger16  minValue;
    UInteger16  maxValue;
    UInteger16  defaultValue;

} UInteger16_option;

typedef struct {
    char * name;
    char * value;
} TemplateOption;

typedef struct {
    char * name;
    char * description;
    TemplateOption options[100];
} ConfigTemplate;

/* Structure defining a PTP engine preset */
typedef struct {

    char* presetName;
    Boolean slaveOnly;
    Boolean noAdjust;
    UInteger8_option clockClass;

} PtpEnginePreset;

/* Preset definitions */
enum {
    PTP_PRESET_NONE,
    PTP_PRESET_SLAVEONLY,
    PTP_PRESET_MASTERSLAVE,
    PTP_PRESET_MASTERONLY,
    PTP_PRESET_MAX
};

void loadDefaultSettings( RunTimeOpts* rtOpts );
int applyConfigTemplates(dictionary *dict, char *templateNames, char *files);
PtpEnginePreset getPtpPreset(int presetNumber, RunTimeOpts* rtOpts);
void dumpConfigTemplates();

#endif /* PTPD_CONFIGDEFAULTS_H_ */
