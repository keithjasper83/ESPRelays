#ifndef TELEMETRY_BUFFER_H
#define TELEMETRY_BUFFER_H

#include "telemetry_snapshot.h"
#include <Arduino.h>

namespace telemetry {

/**
 * @brief Owns any queueing for pending telemetry.
 * 
 * Stores pending telemetry.
 * No networking.
 */
class TelemetryBuffer {
public:
    TelemetryBuffer();
    
    // Add a snapshot to the buffer
    bool addSnapshot(const TelemetrySnapshot& snapshot);
    
    // Remove and return the oldest snapshot
    bool removeOldest(TelemetrySnapshot& snapshot);
    
    // Get buffer size
    size_t size() const;
    
    // Clear the buffer
    void clear();
    
    // Check if buffer is full
    bool isFull() const;
    
    // Check if buffer is empty
    bool isEmpty() const;
    
private:
    static const size_t MAX_BUFFER_SIZE = 10;
    TelemetrySnapshot buffer_[MAX_BUFFER_SIZE];
    size_t head_;
    size_t tail_;
    size_t count_;
};

} // namespace telemetry

#endif // TELEMETRY_BUFFER_H
