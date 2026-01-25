#ifndef _VERSION_INC_
#define _VERSION_INC_

// Auto-generated version from build date/time
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Convert build date/time to version string format: YYYYMMDD-HHMM
// Example: 20250121-1430
inline void generateVersion(char* buffer, size_t bufferSize) {
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    char month[4], day[3], year[5], time[9];
    sscanf(BUILD_DATE, "%s %s %s", month, day, year);
    strcpy(time, BUILD_TIME);

    int monthNum = 1;
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            monthNum = i + 1;
            break;
        }
    }

    int h, m, s;
    sscanf(time, "%d:%d:%d", &h, &m, &s);

    snprintf(buffer, bufferSize, "%s%02d%02d-%02d%02d",
             year, monthNum, atoi(day), h, m);
}

#endif
