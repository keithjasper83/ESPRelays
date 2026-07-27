#include "telemetry_buffer.h"

namespace telemetry {

TelemetryBuffer::TelemetryBuffer()
    : head_(0)
    , tail_(0)
    , count_(0) {
}

bool TelemetryBuffer::addSnapshot(const TelemetrySnapshot& snapshot) {
    if (count_ >= MAX_BUFFER_SIZE) {
        return false;
    }
    
    buffer_[tail_] = snapshot;
    tail_ = (tail_ + 1) % MAX_BUFFER_SIZE;
    count_++;
    
    return true;
}

bool TelemetryBuffer::removeOldest(TelemetrySnapshot& snapshot) {
    if (count_ == 0) {
        return false;
    }
    
    snapshot = buffer_[head_];
    head_ = (head_ + 1) % MAX_BUFFER_SIZE;
    count_--;
    
    return true;
}

size_t TelemetryBuffer::size() const {
    return count_;
}

void TelemetryBuffer::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

bool TelemetryBuffer::isFull() const {
    return count_ >= MAX_BUFFER_SIZE;
}

bool TelemetryBuffer::isEmpty() const {
    return count_ == 0;
}

} // namespace telemetry
