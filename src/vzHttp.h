#ifndef MY_HTTP_H
#define MY_HTTP_H
//
// 2023-02-14 mh
// - adapt size of uuidValue structure
// 2023-01-30 - 2023-02-02 mh
// - expansion of class
//
// (C) Copyright M. Herbert, 2022-2023.
// Licensed under the GNU General Public License v3.0

#include "config.h"
#ifndef DEBUG_TRACE
    #define DEBUG_TRACE(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout); Serial.println();}
#endif


#define N_UUID_VALUE 10          // adapt if enum is changed.
enum UuidValueName
{
    vzTEMP_CH6,
    vzINV_POWER,
    vzINV_DC_U,
    vzINV_DC_I,
    vzINV_DC_POWER,
    vzINV_ENERGY_LASTDAY,
    vzINV_ENERGY_LASTMONTH,
    vzINV_ENERGY_THISDAY,
    vzINV_HEART_BEAT,
    vzTEST
};

#define sizeOfUUID 48   // length of UUID string
struct VzHttpConfig
{
  char vzServer[64] = VZ_SERVER;
  char vzMiddleware[64] = VZ_MIDDLEWARE;
  char uuidValue[N_UUID_VALUE][sizeOfUUID] = {"0","1","2","3","4","5","6","7","8","9"};
};

class VzHttp
{
public:
    VzHttp();
    void init(VzHttpConfig &config);
    void setServerName(String serverName);
    void setMiddlewareName(String middlewareName);
    void testHttp();
    int postHttp(String vzUUID, String timeStamp, float value);
    String getTimeStamp();
    float getValue(UuidValueName select);

private:
    String _TimeStamp="0";          // ms
    String _serverName="";
    String _middlewareName="";
    char* _uuid[N_UUID_VALUE];
    float _value[N_UUID_VALUE];
};
#endif // MY_HTTP_H