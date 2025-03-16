#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <Arduino.h>
#include <math.h> // for fabs

// Structure to hold a single measurement.
struct Measurement {
    float value;
    int64_t timestamp;
};

class RingBuffer {
public:
    // Constructor: Initializes the ring buffer with the given capacity and creates the mutex.
    RingBuffer(int capacity)
      : capacity(capacity), headIndex(0), countMeasurements(0) {
        buffer = new Measurement[capacity];
        ringBufferMutex = xSemaphoreCreateMutex();
        if (ringBufferMutex == NULL) {
            Serial.println("Error: Failed to create ring buffer mutex.");
        }
    }
    
    // Destructor: Deletes the allocated buffer and the mutex.
    ~RingBuffer() {
        if (buffer != nullptr) {
            delete[] buffer;
        }
        if (ringBufferMutex != NULL) {
            vSemaphoreDelete(ringBufferMutex);
        }
    }
    
    // Adds a new measurement to the ring buffer.
    // If the buffer is full, the oldest entry is overwritten.
    void addMeasurement(const Measurement &m) {
        xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
        if (countMeasurements < capacity) {
            int tail = (headIndex + countMeasurements) % capacity;
            buffer[tail] = m;
            countMeasurements++;
        } else {
            // Buffer full: Overwrite the oldest entry.
            buffer[headIndex] = m;
            headIndex = (headIndex + 1) % capacity;
        }
        xSemaphoreGive(ringBufferMutex);
    }
    
    // Copies up to maxCount measurements from the buffer into dest.
    // Returns the actual number of measurements copied.
    int getChunk(Measurement *dest, int maxCount) {
        int count = 0;
        xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
        count = (countMeasurements < maxCount) ? countMeasurements : maxCount;
        for (int i = 0; i < count; i++) {
            int idx = (headIndex + i) % capacity;
            dest[i] = buffer[idx];
        }
        xSemaphoreGive(ringBufferMutex);
        return count;
    }
    
    // Removes measurements from the buffer that match the ones in dest.
    // It compares the timestamp and the value (with a tolerance of 0.001).
    // Returns the number of removed entries.
    int removeChunk(const Measurement *dest, int sentCount) {
        int removed = 0;
        xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
        for (int i = 0; i < sentCount && i < countMeasurements; i++) {
            int idx = (headIndex + i) % capacity;
            if (buffer[idx].timestamp == dest[i].timestamp &&
                fabs(buffer[idx].value - dest[i].value) < 0.001) {
                removed++;
            } else {
                break;
            }
        }
        headIndex = (headIndex + removed) % capacity;
        countMeasurements -= removed;
        xSemaphoreGive(ringBufferMutex);
        return removed;
    }
    
    // Returns the current number of measurements stored in the buffer.
    int getCount() {
        int count;
        xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
        count = countMeasurements;
        xSemaphoreGive(ringBufferMutex);
        return count;
    }
    
private:
    Measurement* buffer;
    int capacity;
    int headIndex;
    int countMeasurements;
    SemaphoreHandle_t ringBufferMutex;
};

#endif // RINGBUFFER_H
