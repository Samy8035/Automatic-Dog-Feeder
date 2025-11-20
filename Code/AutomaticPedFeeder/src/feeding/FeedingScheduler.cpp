#include "FeedingScheduler.h"

FeedingScheduler::FeedingScheduler(FeedingLogic* logic)
    : feedingLogic(logic),
      enabled(true),
      feedingIntervalHours(DEFAULT_FEEDING_INTERVAL_HOURS),
      maxFeedingsPerDay(DEFAULT_PORTIONS_PER_DAY),
      lastFeedingTime(0),
      nextFeedingTime(0),
      feedingsTodayCount(0),
      lastDay(-1),
      scheduledFeedingCallback(nullptr) {
}

void FeedingScheduler::begin() {
    scheduleNextFeeding();
}

void FeedingScheduler::update() {
    if (!enabled) return;
    
    // Verificar cambio de día
    int currentDay = TimeUtils::getCurrentDay();
    if (currentDay != lastDay) {
        resetDailyCount();
        lastDay = currentDay;
    }
    
    // Verificar si es hora de alimentar
    if (shouldFeedNow()) {
        executeFeeding();
    }
}

void FeedingScheduler::setEnabled(bool enable) {
    enabled = enable;
    if (enabled) {
        scheduleNextFeeding();
    }
}

void FeedingScheduler::setFeedingInterval(int hours) {
    if (hours > 0 && hours <= 24) {
        feedingIntervalHours = hours;
        scheduleNextFeeding();
    }
}

void FeedingScheduler::setMaxFeedingsPerDay(int max) {
    if (max > 0 && max <= 10) {
        maxFeedingsPerDay = max;
    }
}

void FeedingScheduler::resetDailyCount() {
    feedingsTodayCount = 0;
    scheduleNextFeeding();
}

void FeedingScheduler::scheduleNextFeeding() {
    if (lastFeedingTime == 0) {
        // Primera alimentación inmediata o en el próximo intervalo
        nextFeedingTime = millis() + (feedingIntervalHours * 3600000UL);
    } else {
        nextFeedingTime = lastFeedingTime + (feedingIntervalHours * 3600000UL);
    }
}

unsigned long FeedingScheduler::getTimeUntilNextFeeding() const {
    if (!enabled || nextFeedingTime == 0) {
        return 0;
    }
    
    unsigned long currentTime = millis();
    if (currentTime >= nextFeedingTime) {
        return 0;
    }
    
    return nextFeedingTime - currentTime;
}

String FeedingScheduler::getScheduleStatus() const {
    if (!enabled) {
        return "Programación: Desactivada";
    }
    
    String status = "Próxima alimentación en: ";
    status += formatTimeRemaining(getTimeUntilNextFeeding());
    status += " | Hoy: " + String(feedingsTodayCount) + "/" + String(maxFeedingsPerDay);
    
    return status;
}

bool FeedingScheduler::shouldFeedNow() const {
    if (!enabled) return false;
    if (feedingLogic->isFeedingInProgress()) return false;
    if (feedingsTodayCount >= maxFeedingsPerDay) return false;
    if (nextFeedingTime == 0) return false;
    
    return millis() >= nextFeedingTime;
}

void FeedingScheduler::executeFeeding() {
    if (feedingLogic->startFeeding()) {
        lastFeedingTime = millis();
        feedingsTodayCount++;
        scheduleNextFeeding();
        
        if (scheduledFeedingCallback) {
            scheduledFeedingCallback();
        }
    }
}

String FeedingScheduler::formatTimeRemaining(unsigned long milliseconds) const {
    if (milliseconds == 0) {
        return "Ahora";
    }
    
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    minutes %= 60;
    seconds %= 60;
    
    String result = "";
    
    if (hours > 0) {
        result += String(hours) + "h ";
    }
    if (minutes > 0) {
        result += String(minutes) + "m ";
    }
    if (hours == 0 && seconds > 0) {
        result += String(seconds) + "s";
    }
    
    return result.length() > 0 ? result : "0s";
}