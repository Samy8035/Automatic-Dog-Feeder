#ifndef FEEDING_SCHEDULER_H
#define FEEDING_SCHEDULER_H

#include <Arduino.h>
#include "../config.h"
#include "FeedingLogic.h"

class FeedingScheduler {
private:
    FeedingLogic* feedingLogic;
    
    // Configuración
    bool enabled;
    int feedingIntervalHours;
    int maxFeedingsPerDay;
    
    // Estado
    unsigned long lastFeedingTime;
    unsigned long nextFeedingTime;
    int feedingsTodayCount;
    int lastDay;
    
    // Callbacks
    void (*scheduledFeedingCallback)();
    
public:
    FeedingScheduler(FeedingLogic* logic);
    
    // Inicialización
    void begin();
    void update();
    
    // Control
    void setEnabled(bool enable);
    void setFeedingInterval(int hours);
    void setMaxFeedingsPerDay(int max);
    void resetDailyCount();
    void scheduleNextFeeding();
    
    // Estado
    bool isEnabled() const { return enabled; }
    int getFeedingInterval() const { return feedingIntervalHours; }
    unsigned long getNextFeedingTime() const { return nextFeedingTime; }
    unsigned long getTimeUntilNextFeeding() const;
    int getFeedingsTodayCount() const { return feedingsTodayCount; }
    String getScheduleStatus() const;
    
    // Callbacks
    void setScheduledFeedingCallback(void (*callback)()) { 
        scheduledFeedingCallback = callback; 
    }
    
private:
    bool shouldFeedNow() const;
    void executeFe eding();
    String formatTimeRemaining(unsigned long milliseconds) const;
};

#endif // FEEDING_SCHEDULER_H