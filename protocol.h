#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <sstream> // For serializing data into a string
#include <vector>

// A structure to hold our sensor data
struct SensorData {
    double latitude = 34.0522; // Default to Los Angeles
    double longitude = -118.2437;
    double temperature = 25.0; // in Celsius

    // Converts the struct's data into a string for sending
    std::string serialize() const {
        std::stringstream ss;
        ss << "LAT:" << latitude << ";LON:" << longitude << ";TEMP:" << temperature << ";";
        return ss.str();
    }
};

#endif // PROTOCOL_H